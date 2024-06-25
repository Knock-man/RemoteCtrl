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
	//初始化地址结构
	if (m_servsock == -1)return false;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//监听本机所有地址
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	//bind listen
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

int CServerSocket::Run(void* cmdObject,SOCKET_CALLBACK callback)
{
	m_callback = callback;//业务层回调函数
	m_arg_cmdObject = cmdObject;//commad对象
	//网络初始化
	bool ret  = InitSocket();
	if (ret == false)return -1;

	std::list<CPacket> listPacket;//调取业务层处理结果

	int count = 0;
	while (true)
	{
		//建立连接
		if (AcceptClient() == false)
		{
			if (count >= 3) {
				return -2;
			}
			count++;//三次连接机会
			continue;
		}
		//接收数据
		int rcmd = DealCommand();//ret为操作类型
		if (rcmd > 0)
		{
			//执行相应命令
			m_callback(m_arg_cmdObject,rcmd,m_packet,listPacket);
			while (listPacket.size() > 0) {//处理结果全部发送出去
				Send(listPacket.front());
				listPacket.pop_front();
			}
		}
		CloseSocket();//关闭套接字
	}
	
	return 0;
}

//接收客户端连接
bool CServerSocket::AcceptClient()
{
	sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	m_clntsock = accept(m_servsock, (sockaddr*)&client_addr, &client_addr_len);//阻塞
	if (m_clntsock == -1)return false;
	return true;
}

//接收消息	拆包 返回值：控制命令
int CServerSocket::DealCommand()
{
	if (m_clntsock == -1)return -1;
	char* buffer = m_buffer.data();
	static size_t index = 0;//缓冲区空闲位置指针（实际存储数据大小）
	while (true)
	{
		size_t len = recv(m_clntsock, buffer + index, BUFSIZE - index, 0);
		//TRACE("[服务器]buff=%s  buffSize=%d\r\n", buffer, index + len);
		if ((len <= 0) && (index <= 0))//断开连接/读到末尾
		{
			return -1;
		}
		index += len;
		len = index;
		m_packet = CPacket((BYTE*)buffer, len);//len传入：buffer数据长度   len传出：成功解析数据长度
		if (len > 0)//解析成功
		{
			memmove(buffer, buffer + len, index - len);//缓冲区剩余解析数据移到缓冲区头部
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

//发送数据 直接发送字符串
bool CServerSocket::Send(const void* pData, size_t nSize)
{
	if (m_clntsock == -1)return false;
	return send(m_clntsock, (const char*)pData, nSize, 0) > 0;
}

//发送包 先把包转换为字符串 再发送
bool CServerSocket::Send(CPacket& pack)
{
	if (m_clntsock == -1)return false;
	int ret = send(m_clntsock, pack.Data(), pack.size(), 0);
	//TRACE("[服务器]发送%d个字节\r\n", ret);
	if (ret)return ret;
	else return false;
}

//bool CServerSocket::GetFilePath(std::string& strPath)
//{
//	if ((m_packet.sCmd == 2)|| (m_packet.sCmd == 3)|| (m_packet.sCmd == 4)|| (m_packet.sCmd == 9))
//	{
//		strPath = m_packet.strData;
//		return true;
//	}
//	return false;
//}

//获取鼠标事件
//bool CServerSocket::GetMouseEvent(MOUSEEV& mouse)
//{
//	if (m_packet.sCmd == 5)
//	{
//		memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
//		return true;
//	}
//	return false;
//}

CPacket& CServerSocket::GetPacket()
{
	return m_packet;
}

//网络环境初始化
BOOL  CServerSocket::InitSockEnv() {
	WSAData data;
	if (WSAStartup(MAKEWORD(2,0), &data))
	{
		return FALSE;
	}
	return TRUE;
}





