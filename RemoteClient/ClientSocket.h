#pragma once
#include "framework.h"
#include "pch.h"
#include <string>
#include "Packet.h"
#include<list>
#include<map>
#include <vector>
#include<mutex>
#define BUFSIZE 4096000
#define PORT 9527
#define WM_SEND_PACK (WM_USER+1) //发送包数据 消息
#define WM_SEND_PACK_ACK (WM_USER+2) //发送包数据应答 消息

//网络通信类
class CClientSocket
{
public:
	//获取单例实例接口
	static CClientSocket* getInstance();

	//套接字初始化
	bool CClientSocket::InitSocket();

	//关闭
	void CloseSocket();

	//接收消息
	int DealCommand();

	//bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks,bool isAutoClosed=true);
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed=true, WPARAM wParam = 0);

	//获取数据包
	CPacket& GetPacket();

	//更新网络地址
	void UpdateAddress(int nIP, int nPort)
	{
		if ((m_nIP != nIP) || (m_nPort != nPort))//修改了 IP 端口
		{
			m_nIP = nIP;
			m_nPort = nPort;
		}		
	}
private:
	//函数指针 指向消息函数
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);

	//容器
	std::map<UINT, MSGFUNC> m_mapFunc;
	std::map<HANDLE, bool>m_mapAutoClosed;//事件长短连接映射表
	std::map<HANDLE, std::list<CPacket>&> m_mapAck;//接收结果映射表
	std::list<CPacket>m_lstSend;//发送队列

	HANDLE m_eventInvoke;//启动事件
	UINT m_nThreadID;
	HANDLE m_hThread;
	
	bool m_bAutoClose;//长短连接
	std::mutex m_lock;
	
	
	int m_nIP;//地址
	int m_nPort;//端口
private:
	//套接字
	SOCKET m_sock;

	//数据包
	CPacket m_packet;

	//缓冲区
	std::vector<char> m_buffer;

	//构造
	CClientSocket();

	//析构
	~CClientSocket();

	CClientSocket(const CClientSocket&);
	CClientSocket& operator=(const CClientSocket&) {};

	//初始化网络环境
	BOOL InitSockEnv();

	//发送消息
	//bool Send(const void* pData, size_t nSize);
	bool Send(const CPacket& pack);

	//void threadFunc();
	static unsigned _stdcall threadEntry(void* arg);
	void threadFunc2();//消息循环

	//消息函数  WM_SEND_PACK消息激活
	void SendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/);
	
	
};