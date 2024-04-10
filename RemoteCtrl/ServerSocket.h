#pragma once
#include"framework.h"
#include"pch.h"
class CServerSocket
{
public:
	//获取单例实例接口
	static CServerSocket* getInstance();


	//套接字初始化
	bool InitSocket();

	//建立连接
	bool AcceptClient();

	//接收消息
	int DealCommand();

	//发送消息
	bool Send(const void* pData, size_t nSize);

private:
	//套接字
	SOCKET m_servsock;
	SOCKET m_clntsock;

	//构造
	CServerSocket();

	//析构
	~CServerSocket();

	CServerSocket(const CServerSocket&) = delete;
	CServerSocket& operator=(const CServerSocket&) = delete;

	//初始化网络环境
	BOOL InitSockEnv();
};