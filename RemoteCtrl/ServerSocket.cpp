#include "pch.h"
#include "ServerSocket.h"


//网络服务类
//构造
CServerSocket::CServerSocket() {
	m_clntsock = -1;
	if (InitSockEnv() == FALSE)
	{
		MessageBox(NULL, TEXT("无法初始化套接字错误,请检查网络设置"), TEXT("初始化错误"), MB_OK | MB_ICONERROR);
		exit(0);
	}

	m_servsock = socket(AF_INET, SOCK_STREAM, 0);
	m_buffer.resize(BUFSIZE);
	memset(m_buffer.data(), 0, BUFSIZE);
};

CServerSocket::CServerSocket(const CServerSocket& ss)
{
	m_clntsock = ss.m_clntsock;
	m_servsock = ss.m_servsock;
};

//析构
CServerSocket::~CServerSocket() {
	closesocket(m_servsock);
	WSACleanup();

};

CServerSocket* CServerSocket::getInstance()
{
	static CServerSocket server;
	return &server;
}

//套接字初始化
bool CServerSocket::InitSocket()
{
	if (m_servsock == -1)return false;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	//serv_addr.sin_port = PORT;

	if (bind(m_servsock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
	{
		return false;
	};

	if (listen(m_servsock, 1) == -1)
	{
		return false;
	};
	
	return true;
}

int CServerSocket::Run(SOCKET_CALLBACK callback, void* cmdObject)
{
	m_callback = callback;//回调函数
	m_arg_cmdObject = cmdObject;//commad对象
	//网络初始化
	bool ret  = InitSocket();
	if (ret == false)return -1;

	std::list<CPacket> listPacket;

	int count = 0;
	while (true)
	{
		//建立连接
		if (AcceptClient() == false)
		{
			if (count >= 3) {
				return -2;
			}
			count++;
		}
		//接收数据
		int ret = DealCommand();
		if (ret > 0)
		{
			//执行相应命令
			m_callback(m_arg_cmdObject, ret,listPacket,m_packet);
			while (listPacket.size() > 0) {
				Send(listPacket.front());
				listPacket.pop_front();
			}
		}
		CloseSocket();
	}
	
	return 0;
}

//接收客户端连接
bool CServerSocket::AcceptClient()
{
	sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	m_clntsock = accept(m_servsock, (sockaddr*)&client_addr, &client_addr_len);
	if (m_clntsock == -1)return false;
	return true;
}

//接收消息
int CServerSocket::DealCommand()
{
	if (m_clntsock == -1)return -1;
	char* buffer = m_buffer.data();
	static size_t index = 0;//缓冲区空闲位置指针（实际存储数据大小）
	while (true)
	{
		size_t len = recv(m_clntsock, buffer + index, BUFSIZE - index, 0);
		//TRACE("[服务器]buff=%s  buffSize=%d\r\n", buffer, index + len);
		if ((len <= 0) && (index <= 0))
		{
			return -1;
		}
		index += len;
		len = index;
		m_packet = CPacket((BYTE*)buffer, len);//len传入：buffer数据长度   传出：已解析数据长度
		if (len > 0)//解析成功
		{
			memmove(buffer, buffer + len, index - len);//剩余解析数据移到缓冲区头部
			index -= len;
			
			return m_packet.sCmd;
		}
	}
	return -1;
}

void CServerSocket::CloseSocket()
{
	if (m_clntsock != INVALID_SOCKET)
	{
		closesocket(m_clntsock);
		m_clntsock = -1;
	}
	
}

//发送
bool CServerSocket::Send(const void* pData, size_t nSize)
{
	if (m_clntsock == -1)return false;
	return send(m_clntsock, (const char*)pData, nSize, 0) > 0;
}

bool CServerSocket::Send(CPacket& pack)
{
	if (m_clntsock == -1)return false;
	int ret = send(m_clntsock, pack.Data(), pack.size(), 0);
	TRACE("[服务器]发送%d个字节\r\n", ret);
	if (ret)return ret;
	else return -1;
}

bool CServerSocket::GetFilePath(std::string& strPath)
{
	if ((m_packet.sCmd == 2)|| (m_packet.sCmd == 3)|| (m_packet.sCmd == 4)|| (m_packet.sCmd == 9))
	{
		strPath = m_packet.strDate;
		return true;
	}
	return false;
}

//获取鼠标事件
bool CServerSocket::GetMouseEvent(MOUSEEV& mouse)
{
	if (m_packet.sCmd == 5)
	{
		memcpy(&mouse, m_packet.strDate.c_str(), sizeof(MOUSEEV));
		return true;
	}
	return false;
}

CPacket& CServerSocket::GetPacket()
{
	return m_packet;
}

//网络环境初始化
BOOL  CServerSocket::InitSockEnv() {
	WSAData data;
	if (WSAStartup(MAKEWORD(1, 1), &data))
	{
		return false;
	}
	return TRUE;
}





