#pragma once
#include <MSWSock.h>
#include "EdoyunThread.h"
#include "CEdoyunQueue.h"
#include <map>
#include "CEdoyunTool.h"

//��������
enum EdoyunOperator {
	ENone,
	EAccept,//����
	ERecv,//��������
	ESend,//��������
	EError//����
};

class EdoyunServer;
class EdoyunClient;
typedef std::shared_ptr<EdoyunClient> PCLIENT;//�ͻ��˶���ָ��

//Overlapped����
class EdoyunOverlapped :public ThreadFuncBase {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;//���� �μ�EdoyunOperator
	std::vector<char> m_buffer;//������
	ThreadWorker m_worker;//��������
	EdoyunServer* m_server;//����������
	EdoyunClient* m_client;//��Ӧ�Ŀͻ���
	WSABUF m_wsabuffer;
	virtual ~EdoyunOverlapped() {
		m_client = NULL;
	}
};
template<EdoyunOperator>class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;
template<EdoyunOperator>class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;
template<EdoyunOperator>class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;

//�ͻ�����
class EdoyunClient :public ThreadFuncBase {
public:
	EdoyunClient();

	~EdoyunClient();

	void SetOverlappedClient(EdoyunClient* ptr);
	operator SOCKET() {
		return m_sock;
	}
	operator PVOID() {
		return (PVOID)m_buffer.data();
	}
	operator LPOVERLAPPED();

	operator LPDWORD() {
		return &m_received;
	}
	LPWSABUF RecvWSABuffer();
	LPOVERLAPPED RecvOverlapped();
	LPWSABUF SendWSABuffer();
	LPOVERLAPPED SendOverlapped();
	DWORD& flags() { return m_flags; }
	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
	size_t GetBufferSize()const { return m_buffer.size(); }
	int Recv();
	int Send(void* buffer, size_t nSize);
	int SendData(std::vector<char>& data);
private:
	SOCKET m_sock;//�ͻ����׽���
	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::shared_ptr<RECVOVERLAPPED> m_recv;
	std::shared_ptr<SENDOVERLAPPED> m_send;
	std::vector<char> m_buffer;
	size_t m_used;//�Ѿ�ʹ�õĻ�������С
	sockaddr_in m_laddr;//���ص�ַ
	sockaddr_in m_raddr;//Զ�̵�ַ
	bool m_isbusy;
	EdoyunSendQueue<std::vector<char>> m_vecSend;//�������ݶ���
};

template<EdoyunOperator>
class AcceptOverlapped :public EdoyunOverlapped
{
public:
	AcceptOverlapped();
	virtual ~AcceptOverlapped() {}
	int AcceptWorker();
};


template<EdoyunOperator>
class RecvOverlapped :public EdoyunOverlapped
{
public:
	RecvOverlapped();
	virtual ~RecvOverlapped() {}
	int RecvWorker() {
		int ret = m_client->Recv();
		return ret;
	}

};

template<EdoyunOperator>
class SendOverlapped :public EdoyunOverlapped
{
public:
	SendOverlapped();
	virtual ~SendOverlapped() {}
	int SendWorker() {
		//TODO:
		/*
		* 1 Send���ܲ����������
		*/
		return -1;
	}
};
typedef SendOverlapped<ESend> SENDOVERLAPPED;

template<EdoyunOperator>
class ErrorOverlapped :public EdoyunOverlapped
{
public:
	ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	virtual ~ErrorOverlapped() {}
	int ErrorWorker() {
		//TODO:
		return -1;
	}
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;


//������
class EdoyunServer :
	public ThreadFuncBase
{
public:
	EdoyunServer(const std::string& ip = "0.0.0.0", short port = 9527) :m_pool(10) {
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	~EdoyunServer();
	bool StartService();
	//����һ���ͻ��������׽���
	bool NewAccept() {
		//PCLIENT pClient(new EdoyunClient());
		EdoyunClient* pClient = new EdoyunClient();
		pClient->SetOverlappedClient(pClient);
		m_client.insert(std::pair<SOCKET, EdoyunClient*>(*pClient, pClient));
		if (!AcceptEx(m_sock,
			*pClient,
			*pClient,
			0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			*pClient, *pClient))
		{
			if (WSAGetLastError() != ERROR_SUCCESS && (WSAGetLastError() != WSA_IO_PENDING)) {
				TRACE("����ʧ�ܣ�%d %s\r\n", WSAGetLastError(), CEdoyunTool::GetErrInfo(WSAGetLastError()).c_str());
				closesocket(m_sock);
				m_sock = INVALID_SOCKET;
				m_hIOCP = INVALID_HANDLE_VALUE;
				return false;
			}
		}
		return true;
	}
	//����IOCP��sock �׽���
	void BindNewSocket(SOCKET sock, ULONG_PTR nKey);
private:
	//�����������׽���
	void CreateSocket() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
			return;
		}
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}
	//IOCP�߳�
	int threadIocp();
private:
	EdoyunThreadPool m_pool;//�̳߳�
	HANDLE m_hIOCP;//��ȫ�˿�
	SOCKET m_sock;//�������׽���
	sockaddr_in m_addr;//��������ַ
	std::map<SOCKET, EdoyunClient*> m_client;//���ӵĿͻ���
};

