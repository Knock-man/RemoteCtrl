#pragma once
#include "Thread.h"
#include <map>
#include <MSWSock.h>
#include "CEdoyunQueue.h"

//操作类型
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

//Overlapped基类
class EdoyunOverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;//操作 参照EdoyunOperator枚举
	std::vector<char>m_buffer;//缓冲区
	ThreadWorker m_worker;//处理函数
	EdoyunServer* m_server;//服务器对象
};

template<EdoyunOperator>class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;
//客户端类
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
	SOCKET m_sock;//客户端套接字
	std::vector<char>m_buffer;//通信缓冲区
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;//连接OVERLAPP
	DWORD m_received;
	sockaddr_in m_laddr;//本地地址
	sockaddr_in m_raddr;//远程地址
	bool m_isbusy;//客户端是否忙碌
};

//接收连接Overlapped
template<EdoyunOperator>
class AcceptOverlapped :public EdoyunOverlapped,ThreadFuncBase
{
public:
	AcceptOverlapped();

	int AcceptWorker();

public:
	PCLIENT m_client;
};

//发送数据Overlapped 
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

//接收数据Overlapped
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

//发生错误Overlapped
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




//服务器
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

	//启动服务器
	bool StartService() {
		CreateSocket();
		
		//绑定
		if (bind(m_servsock, (sockaddr*)&m_servaddr, sizeof(m_servaddr)) == -1)
		{
			closesocket(m_servsock);
			m_servsock = INVALID_SOCKET;
			return false;
		}

		//监听
		if (listen(m_servsock, 3) == -1) {
			closesocket(m_servsock);
			m_servsock = INVALID_SOCKET;
			return false;
		}

		//创建完全端口
		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
		if (m_hIOCP == NULL) {
			closesocket(m_servsock);
			m_servsock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}

		//套接字添加到完全端口上
		CreateIoCompletionPort((HANDLE)m_servsock, m_hIOCP, (ULONG_PTR)this, 0);

		//开启线程池
		m_pool.Invoke();
		//启动Iocp线程
		m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPPE)&EdoyunServer::threadIocp));

		//投递一个连接请求
		if (NewAccept()); return false;
		return true;
	}
	//投递一个连接请求
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
		//创建套接字 
		m_servsock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		//设置端口复用
		int  opt = 1;
		setsockopt(m_servsock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}
	
	//Iocp线程
	int threadIocp() {
		DWORD transferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* lpOverlapped = NULL;

		if (GetQueuedCompletionStatus(
			m_hIOCP,
			&transferred,//内核修改数据的字节数
			&CompletionKey,
			&lpOverlapped, INFINITE))
		{
			
			if (transferred > 0 && (CompletionKey != 0)) {
				//从一个子结构中获取包含该子结构成员的更大的结构体指针
				EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped);
				switch(pOverlapped->m_operator){
					case EAccept://接收连接
					{
						ACCEPTOVERLAPPED* pAccept = (ACCEPTOVERLAPPED*)pOverlapped;
						m_pool.DispatchWorker(pAccept->m_worker);
					}
					break;
					case ERecv://接收消息
					{
						RECVOVERLAPPED* pRecv = (RECVOVERLAPPED*)pOverlapped;
						m_pool.DispatchWorker(pRecv->m_worker);
					}
					break;
					case ESend://发送消息
					{
						SENDOVERLAPPED* pSend = (SENDOVERLAPPED*)pOverlapped;
						m_pool.DispatchWorker(pSend->m_worker);
					}
					break;
					case EError://错误消息
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

