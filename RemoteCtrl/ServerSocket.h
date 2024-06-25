/*
网络通信模块
作用：网络初始化(socket() listen() bind() accept() send() recv() closesocket())
run()函数利用了回调机制  解耦了业务层和网络层
*/

#pragma once
#include"framework.h"
#include"pch.h"
#include"list"
#include<vector>
#include"Packet.h"

#define BUFSIZE 409600
#define PORT 9527

typedef void(*SOCKET_CALLBACK)(void* arg, int status, CPacket& inPacket, std::list<CPacket>&);//回调函数

//网络通信类
class CServerSocket
{
public:
	//获取单例实例接口
	static CServerSocket* getInstance();

	//执行网络通信 初始化网络 → 建立连接 → 接收数据 → 发送数据 → 关闭套接字
	int Run(void* arg,SOCKET_CALLBACK callback);

private:

	//套接字初始化 创建套接字 bind listen
	bool InitSocket();
	
	//建立连接 accept() 阻塞等待
	bool AcceptClient();

	//接收消息 存入缓冲区 
	int DealCommand();

	//发送数据 直接发送字符串
	bool Send(const void* pData, size_t nSize);
	//发送包 先把包转化为字符串再发送
	bool Send(CPacket& pack);

	//获取文件列表
	/*bool GetFilePath(std::string& strPath);*/

	//获取鼠标事件
	/*bool GetMouseEvent(MOUSEEV& mouse);*/

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

