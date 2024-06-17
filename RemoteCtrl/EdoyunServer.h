#pragma once
#include "Thread.h"
#include <map>
#include <MSWSock.h>
#include "CEdoyunQueue.h"

//��������
enum EdoyunOperator{
	ENone,
	EAccept,
	ERecv,
	ESend,
	EError
};
class EdoyunServer;
class EdoyunClient;
typedef std::shared_ptr<EdoyunClient>PCLIENT;

//Overlapped����
class EdoyunOverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;//���� ����EdoyunOperatorö��
	std::vector<char>m_buffer;//������
	ThreadWorker m_worker;//������
	EdoyunServer* m_server;//����������
};

template<EdoyunOperator>class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;
//�ͻ�����
class EdoyunClient {
public:
	EdoyunClient();
	~EdoyunClient() {
		closesocket(m_sock);
	}
	void SetOverlappedClient(PCLIENT& ptr);
	operator SOCKET() {
		return m_sock;
	}

	operator PVOID() {
		return &m_buffer[0];
	}

	operator LPOVERLAPPED();

	operator LPDWORD() {
		return &m_received;
	}
	sockaddr_in* GetLocalAddr() { return &m_laddr; };
	sockaddr_in* GetRemoteAddr() { return &m_raddr; };

private:
	SOCKET m_sock;//�ͻ����׽���
	std::vector<char>m_buffer;//ͨ�Ż�����
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;//����OVERLAPP
	DWORD m_received;
	sockaddr_in m_laddr;//���ص�ַ
	sockaddr_in m_raddr;//Զ�̵�ַ
	bool m_isbusy;//�ͻ����Ƿ�æµ
};

//��������Overlapped
template<EdoyunOperator>
class AcceptOverlapped :public EdoyunOverlapped,ThreadFuncBase
{
public:
	AcceptOverlapped();

	int AcceptWorker();

public:
	PCLIENT m_client;
};

//��������Overlapped 
template<EdoyunOperator>
class SendOverlapped :public EdoyunOverlapped, ThreadFuncBase
{
public:
	SendOverlapped() :m_operator(ESend), m_worker(this, &SendOverlapped::SendWorker) {
		memset((void*)m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int SendWorker() {

	}
};
typedef SendOverlapped<ESend>SENDOVERLAPPED;

//��������Overlapped
template<EdoyunOperator>
class RecvOverlapped :public EdoyunOverlapped, ThreadFuncBase
{
public:
	RecvOverlapped() :m_operator(ERecv), m_worker(this, &RecvOverlapped::RecvWorker) {
		memset((void*)m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int  RecvWorker() {

	}
};
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;

//��������Overlapped
template<EdoyunOperator>
class ErrorOverlapped :public EdoyunOverlapped, ThreadFuncBase
{
public:
	ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped:ErrorWorker) {
		memset(m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int ErrorWorker() {

	}
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;




//������
class EdoyunServer : public ThreadFuncBase
{
public:
	EdoyunServer(const std::string& ip =  "0.0.0.0",short port =  9527) :m_pool(10) {
		HANDLE m_hIOCP = INVALID_HANDLE_VALUE;
		m_servsock = INVALID_SOCKET;
		m_servaddr.sin_family = AF_INET;
		m_servaddr.sin_addr.s_addr = inet_addr(ip.c_str());
		m_servaddr.sin_port = htons(port);
	};
	~EdoyunServer() {};

	//����������
	bool StartService() {
		CreateSocket();
		
		//��
		if (bind(m_servsock, (sockaddr*)&m_servaddr, sizeof(m_servaddr)) == -1)
		{
			closesocket(m_servsock);
			m_servsock = INVALID_SOCKET;
			return false;
		}

		//����
		if (listen(m_servsock, 3) == -1) {
			closesocket(m_servsock);
			m_servsock = INVALID_SOCKET;
			return false;
		}

		//������ȫ�˿�
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
		if (m_hIOCP == NULL) {
			closesocket(m_servsock);
			m_servsock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}

		//�׽�����ӵ���ȫ�˿���
		CreateIoCompletionPort((HANDLE)m_servsock, m_hIOCP, (ULONG_PTR)this, 0);

		//�����̳߳�
		m_pool.Invoke();
		//����Iocp�߳�
		m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPPE)&EdoyunServer::threadIocp));

		//Ͷ��һ����������
		if (NewAccept()); return false;
		return true;
	}
	//Ͷ��һ����������
	bool NewAccept() {
		PCLIENT pClient(new EdoyunClient());
		pClient->SetOverlappedClient(pClient);
		m_clientMap.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
		//AcceptEx(m_sock, client, overlapped.m_buffer, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &received, &overlapped.m_overlapped)
		if (!AcceptEx(m_servsock, *pClient,
			*pClient, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			*pClient, *pClient))
		{
			closesocket(m_servsock);
			m_servsock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	
private:
	void CreateSocket() {
		//�����׽��� 
		m_servsock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		//���ö˿ڸ���
		int  opt = 1;
		setsockopt(m_servsock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}
	
	//Iocp�߳�
	int threadIocp() {
		DWORD transferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* lpOverlapped = NULL;

		if (GetQueuedCompletionStatus(
			m_hIOCP,
			&transferred,//�ں��޸����ݵ��ֽ���
			&CompletionKey,
			&lpOverlapped, INFINITE))
		{
			
			if (transferred > 0 && (CompletionKey != 0)) {
				//��һ���ӽṹ�л�ȡ�������ӽṹ��Ա�ĸ���Ľṹ��ָ��
				EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped);
				switch(pOverlapped->m_operator){
					case EAccept://��������
					{
						ACCEPTOVERLAPPED* pAccept = (ACCEPTOVERLAPPED*)pOverlapped;
						m_pool.DispatchWorker(pAccept->m_worker);
					}
					break;
					case ERecv://������Ϣ
					{
						RECVOVERLAPPED* pRecv = (RECVOVERLAPPED*)pOverlapped;
						m_pool.DispatchWorker(pRecv->m_worker);
					}
					break;
					case ESend://������Ϣ
					{
						SENDOVERLAPPED* pSend = (SENDOVERLAPPED*)pOverlapped;
						m_pool.DispatchWorker(pSend->m_worker);
					}
					break;
					case EError://������Ϣ
					{
						ERROROVERLAPPED* pError = (ERROROVERLAPPED*)pOverlapped;
						m_pool.DispatchWorker(pError->m_worker);
					}
					break;
				}
			}
			else
			{
				return -1;
			}
		}
		return 0;
	}
private:
	ThreadPool m_pool;
	HANDLE  m_hIOCP;
	SOCKET   m_servsock;
	sockaddr_in m_servaddr;
	std::map<SOCKET, std::shared_ptr<EdoyunClient>>  m_clientMap;
};			

