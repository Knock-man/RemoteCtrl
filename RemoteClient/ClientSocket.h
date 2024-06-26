/*
网络层
消息机制：
1.开启一个线程执行消息循环
2.当有数据发送时，向消息队列投送发送数据消息请求(SendPacketMessage)，消息循环接收事件，调用发送数据的回调函数(SendPack）
3.在发送数据回调函数中执行 连接服务器 向服务器发送数据 接收服务器数据 向指定窗口发送ACK消息
*/
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

	//发送数据消息请求
	bool SendPacketMessage(HWND hWnd, const CPacket& pack, bool isAutoClosed=true, WPARAM wParam = 0);

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
	/*
		创建线程 注册<消息,消息处理函数映射表>
	*/
	CClientSocket();

	//析构
	~CClientSocket();

	CClientSocket(const CClientSocket&);
	CClientSocket& operator=(const CClientSocket&) = default;

	//初始化网络环境
	BOOL InitSockEnv();

	//发送消息
	bool Send(const CPacket& pack);

	//消息循环 等待有通信事件发生 调取相应的消息处理函数
	static unsigned _stdcall threadEntry(void* arg);
	void threadFunc();

	//消息处理函数
	//建立网络连接，向服务器发送数据，并且在收到服务器数据后，向指定窗口发送消息
	void SendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/);
	
	
};