#pragma once
#include"framework.h"
#include"pch.h"
#include "string"
#include<list>
#include<map>
#include <vector>
#define BUFSIZE 2048000
#define PORT 9527

#pragma pack(push)
#pragma pack(1)
//包 [包头2 包长度4 控制命令2 包数据 和校验2]
class CPacket
{
public:
	CPacket();
	CPacket(const BYTE* pData, size_t& nSize);//解包
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize, HANDLE hEvent);//打包
	CPacket(const CPacket& pack);
	CPacket& operator=(const CPacket& pack);
	~CPacket();

	int size();//包大小
	const char* CPacket::Data(std::string& strOut) const;//包

public:
	//WORD:unsiged short(2字节)		DWORD:unsigned long(4字节)
	WORD sHead;//包头 FEFF  
	DWORD nLength;//包长度（从控制命令开始，到和校验结束） 
	WORD sCmd;//控制命令
	std::string strData;//包数据
	WORD sSum;//和校验
	HANDLE hEvent;
};
#pragma pack(pop)
//文件信息结构体
typedef struct file_info
{
	file_info()
	{
		IsInvalid = false;//默认为有效文件
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//是否有效
	BOOL IsDirectory;//文件类型 0文件 1目录
	BOOL HasNext;//是否还有后续 0没有 1有
	char szFileName[256];//文件名
}FILEINFO, * PFILEINFO;
//鼠标结构体
typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//点击、移动、双击
	WORD nButton;//左键、右键、滚轮
	POINT ptXY;//坐标
}MOUSEEV, * PMOUSEEV;

class CClientSocket
{
public:
	//获取单例实例接口
	static CClientSocket* getInstance();


	//套接字初始化
	bool CClientSocket::InitSocket();

	void CloseSocket();

	//接收消息
	int DealCommand();

	//发送消息
	bool Send(const void* pData, size_t nSize);
	bool Send(const CPacket& pack);

	//获取文件列表
	bool GetFilePath(std::string& strPath);

	bool GetMouseEvent(MOUSEEV& mouse);

	CPacket& GetPacket();

	void UpdateAddress(int nIP, int nPort)
	{
		m_nIP = nIP;
		m_nPort = nPort;
	}
private:
	std::list<CPacket>m_lstSend;
	std::map<HANDLE, std::list<CPacket>> m_mapAck;
	int m_nIP;//地址
	int m_nPort;//端口
private:
	//套接字
	SOCKET m_sock;

	//数据包
	CPacket m_packet;

	std::vector<char> m_buffer;

	//构造
	CClientSocket();

	//析构
	~CClientSocket();

	CClientSocket(const CClientSocket&);
	CClientSocket& operator=(const CClientSocket&) {};

	//初始化网络环境
	BOOL InitSockEnv();

	static void threadEntry(void* arg);
	void threadFunc();

};