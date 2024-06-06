#pragma once
#include"framework.h"
#include"pch.h"
#include"list"
#include<vector>
#include"Packet.h"

#define BUFSIZE 409600
#define PORT 9527




typedef void(*SOCKET_CALLBACK)(void* arg, int status,std::list<CPacket>&, CPacket& inPacket);//回调函数

class CServerSocket
{
public:
	//获取单例实例接口
	static CServerSocket* getInstance();


	int Run(SOCKET_CALLBACK callback, void* arg);

private:

	//套接字初始化
	bool InitSocket();

	
	//建立连接
	bool AcceptClient();

	//接收消息
	int DealCommand();

	//发送消息
	bool Send(const void* pData, size_t nSize);
	bool Send(CPacket& pack);

	//获取文件列表
	bool GetFilePath(std::string& strPath);

	//获取鼠标事件
	bool GetMouseEvent(MOUSEEV& mouse);

	//获取数据包
	CPacket& GetPacket();

	//关闭客户端连接
	void CloseSocket();
	
private:
	SOCKET_CALLBACK m_callback;//回调函数
	void* m_arg_cmdObject;

	//套接字
	SOCKET m_servsock;
	SOCKET m_clntsock;

	//数据包
	CPacket m_packet;

	std::vector<char> m_buffer;

private:
	//构造  初始化网络环境，分配服务器套接字
	CServerSocket();

	//析构
	~CServerSocket();

	CServerSocket(const CServerSocket&);
	CServerSocket& operator=(const CServerSocket&) {};

	//初始化网络环境
	BOOL InitSockEnv();
};

