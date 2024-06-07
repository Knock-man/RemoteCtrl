#include "pch.h"
#include "ClientSocket.h"


//网络服务类
//构造
CClientSocket::CClientSocket():m_nIP(INADDR_ANY),m_nPort(0) {
	if (InitSockEnv() == FALSE)
	{
		MessageBox(NULL, TEXT("无法初始化套接字错误,请检查网络设置"), TEXT("初始化错误"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_buffer.resize(BUFSIZE);
	memset(m_buffer.data(), 0, BUFSIZE);
	//m_sock = socket(AF_INET, SOCK_STREAM, 0);
};

//析构
CClientSocket::~CClientSocket() {
	closesocket(m_sock);
	WSACleanup();

}
CClientSocket::CClientSocket(const CClientSocket&  ss)
{
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
};

CClientSocket* CClientSocket::getInstance()
{
	static CClientSocket server;
	return &server;
}

std::string GetErrorInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID IpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&IpMsgBuf, 0, NULL);
	ret = (char*)IpMsgBuf;
	LocalFree(IpMsgBuf);
	return ret;
}

//套接字初始化
bool CClientSocket::InitSocket()
{
	//不能在构造的时候初始化套接字，因为单例模式的对象生命周期是和程序一样的，所以程序只要没有关闭，上一次的套接字依旧存在，不能再重新分配套接字
	if (m_sock != -1)CloseSocket();//保证分配的套接字是新的套接字
	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sock == -1)return false;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_addr.s_addr = htonl(m_nIP);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(m_nPort);
	if (serv_addr.sin_addr.s_addr == INADDR_ANY)
	{
		AfxMessageBox("指定的IP地址不存在");
		return false;
	}
	int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == -1)
	{
		AfxMessageBox("连接失败");
		TRACE("连接失败,%d %s\r\n", WSAGetLastError(),GetErrorInfo(WSAGetLastError()).c_str());
		return false;
	}
	return true;
}

void CClientSocket::CloseSocket()
{
	closesocket(m_sock);
	m_sock = -1;
}

CPacket& CClientSocket::GetPacket()
{
	return m_packet;
}

//接收消息
int CClientSocket::DealCommand()
{
	if (m_sock == -1)return -1;
	char* buffer = m_buffer.data();
	static size_t index = 0;//缓冲区空闲位置指针（实际存储数据大小）
	while (true)
	{
		size_t len = recv(m_sock, buffer + index, BUFSIZE - index, 0);
		TRACE("[客户端]len=%d buff=%s  buffSize=%d\r\n", len,buffer,index+len);
		if (((int)len <= 0)&&((int)index<=0))
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

//发送
bool CClientSocket::Send(const void* pData, size_t nSize)
{
	if (m_sock == -1)return false;
	return send(m_sock, (const char*)pData, nSize, 0) > 0;
}

bool CClientSocket::Send(const CPacket& pack)
{
	if (m_sock == -1)return false;
	//TRACE("[客户端]准备发送数据%d\r\n", pack.sCmd);
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}

bool CClientSocket::GetFilePath(std::string& strPath)
{
	if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4))
	{
		strPath = m_packet.strData;
		return true;
	}
	return false;
}

//获取鼠标事件
bool CClientSocket::GetMouseEvent(MOUSEEV& mouse)
{
	if (m_packet.sCmd == 5)
	{
		memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
		return true;
	}
	return false;
}

//网络环境初始化
BOOL  CClientSocket::InitSockEnv() {
	WSAData data;
	if (WSAStartup(MAKEWORD(1, 1), &data))
	{
		return false;
	}
	return TRUE;
}



//包类

CPacket::CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0)
{

}
//解析包
CPacket::CPacket(const BYTE* pData, size_t& nSize)
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
		strData.resize(nLength-2-2);//nLength - [控制命令位长度] - [校验位长度]
		memcpy((void*)strData.c_str(), pData + i, nLength-4);
		i = i + nLength - 2 - 2;
	}

	//取出校验位 并校验
	sSum = *(WORD*)(pData + i); i += 2;
	WORD sum = 0;
	for (size_t j = 0; j < strData.size(); j++)
	{
		sum += BYTE(strData[j]) & 0xFF;//只取字符低八位
	}
	TRACE("[客户端] sHead=%d nLength=%d data=[%s]  sSum=%d  sum = %d\r\n", sHead, nLength, strData.c_str(), sSum, sum);
	if (sum == sSum)
	{
		nSize =i;
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
		strData.resize(nSize);
		memcpy((void*)strData.c_str(), pData, nSize);
	}
	else//无数据段
	{
		strData.clear();
	}

	//打包检验位
	sSum = 0;
	for (size_t j = 0; j < strData.size(); j++)
	{
		sSum += BYTE(strData[j]) & 0xFF;//只取字符低八位
	}
}
CPacket::CPacket(const CPacket& pack)
{
	sHead = pack.sHead;
	nLength = pack.nLength;
	sCmd = pack.sCmd;
	strData = pack.strData;
	sSum = pack.sSum;
}
CPacket& CPacket::operator=(const CPacket& pack)
{
	if (this != &pack)
	{
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	return *this;

}
CPacket::~CPacket()
{

}

int CPacket::size()
{
	return nLength + 6;
}

const char* CPacket::Data(std::string& strOut) const
{
	strOut.resize(nLength + 6);
	BYTE* pData = (BYTE*)strOut.c_str();
	*(WORD*)pData = sHead;
	*(WORD*)(pData + 2) = nLength;
	*(WORD*)(pData + 2 + 4) = sCmd;
	memcpy(pData + 2 + 4 + 2, strData.c_str(), strData.size());
	*(WORD*)(pData + 2 + 4 + 2 + strData.size()) = sSum;
	return strOut.c_str();
}

