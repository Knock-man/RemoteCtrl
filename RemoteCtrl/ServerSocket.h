#pragma once
#include"framework.h"
#include"pch.h"

#define BUFSIZE 4096

//包 [包头 包长度 控制命令 包数据 和校验]
class CPacket
{
public:
	CPacket();
	CPacket(const BYTE* pData, size_t& nSize);
	CPacket(const CPacket& pack);
	CPacket& operator=(const CPacket& pack);
	~CPacket();

public:
	//WORD:unsiged short(2字节)		DWORD:unsigned long(4字节)
	WORD sHead;//包头 FEFF  
	DWORD nLength;//包长度（从控制命令开始，到和校验结束） 
	WORD sCmd;//控制命令
	std::string strDate;//包数据
	WORD sSum;//和校验
};

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

	//数据包
	CPacket m_packet;

	//构造
	CServerSocket();

	//析构
	~CServerSocket();

	CServerSocket(const CServerSocket&) = delete;
	CServerSocket& operator=(const CServerSocket&) = delete;

	//初始化网络环境
	BOOL InitSockEnv();
};

