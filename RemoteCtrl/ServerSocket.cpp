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
	closesocket(m_clntsock);
	m_clntsock = -1;
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
	//TRACE("[服务器]发送%d个字节\r\n", ret);
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



//包类

CPacket::CPacket():sHead(0),nLength(0),sCmd(0),sSum(0)
{

}
// 解析包
CPacket::CPacket(const BYTE * pData, size_t & nSize)
{
	//包 [包头2 包长度4 控制命令2 包数据2 和校验2]
	size_t i = 0;
	//取包头位
	for (; i < nSize; i++)
	{
		if ((*(WORD*)(pData + i)) == 0xFEFF)//找到包头
		{
			sHead = *(WORD*)(pData + i);
			i += 2;
			break;
		}
	}

	if ((i + 4 + 2 + 2) > nSize)//包数据不全 只有 [包头 包长度 控制命令 和校验]  没有数据段 解析失败
	{
		nSize = 0;
		return;
	}

	//取包长度位
	nLength = *(DWORD*)(pData + i); i += 4;
	if (nLength + i > nSize)//包未完全接收到 nLength+sizeof(包头)+sizeof(包长度) pData缓冲区越界了
	{
		nSize = 0;
		return;
	}

	//取出控制命令位
	sCmd = *(WORD*)(pData + i); i += 2;

	//保存数据段
	if (nLength > 4)
	{
		strDate.resize(nLength - 2 - 2);//nLength - [控制命令位长度] - [校验位长度]
		memcpy((void*)strDate.c_str(), pData + i, nLength - 4);
		i = i + nLength - 2 - 2;
	}

	//取出校验位 并校验
	sSum = *(WORD*)(pData + i); i += 2;
	WORD sum = 0;
	for (size_t j = 0; j < strDate.size(); j++)
	{
		sum += BYTE(strDate[j]) & 0xFF;//只取字符低八位
	}
	//TRACE("[客户端] sHead=%d nLength=%d data=[%s]  sSum=%d  sum = %d\r\n", sHead, nLength, strDate.c_str(), sSum, sum);
	if (sum == sSum)
	{
		nSize = i;
		return;
	}
	nSize = 0;
}
//打包：封装成包
CPacket::CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
{
	sHead = 0xFEFF;
	nLength = nSize + 4;
	sCmd = nCmd;


	if (nSize > 0)//有数据段
	{
		//打包数据段
		strDate.resize(nSize);
		memcpy((void*)strDate.c_str(), pData, nSize);
	}
	else//无数据段
	{
		strDate.clear();
	}
	
	//打包检验位
	sSum = 0;
	for (size_t j = 0; j < strDate.size(); j++)
	{
		sSum += BYTE(strDate[j]) & 0xFF;//只取字符低八位
	}
	//TRACE("[服务器] sHead=%d nLength=%d data=[%s]  sSum=%d\r\n", sHead, nLength, strDate.c_str(), sSum);
}
CPacket::CPacket(const CPacket& pack)
{
	sHead = pack.sHead;
	nLength = pack.nLength;
	sCmd = pack.sCmd;
	strDate = pack.strDate;
	sSum = pack.sSum;
}
CPacket& CPacket::operator=(const CPacket& pack)
{
	if (this != &pack)
	{
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strDate = pack.strDate;
		sSum = pack.sSum;
	}
	return *this;
	
}
CPacket::~CPacket()
{

}

int CPacket::size()
{
	return nLength+6;
}

//将包转为字符串类型
const char* CPacket::Data()
{
	strOut.resize(nLength + 6);
	BYTE* pData = (BYTE*)strOut.c_str();
	*(WORD*)pData = sHead;
	*(WORD*)(pData+2) = nLength;
	*(WORD*)(pData + 2 +4) = sCmd;
	memcpy(pData + 2 + 4 + 2, strDate.c_str(), strDate.size());
	*(WORD*)(pData + 2 + 4+2+ strDate.size()) = sSum;
	return strOut.c_str();
}

