#include "pch.h"
#include "ClientSocket.h"


//网络服务类
//构造
CClientSocket::CClientSocket() :


	m_nIP(INADDR_ANY), m_nPort(0),
	m_sock(INVALID_SOCKET),
	m_bAutoClose(true),
	m_hThread(INVALID_HANDLE_VALUE)
{
	if (InitSockEnv() == FALSE)
	{
		MessageBox(NULL, TEXT("无法初始化套接字错误,请检查网络设置"), TEXT("初始化错误"), MB_OK | MB_ICONERROR);
		exit(0);
	}

	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_eventInvoke == NULL) {
		TRACE("创建事件失败!\r\n");
	}
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);//启动通信线程
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT)
	{
		TRACE("网络消息处理线程启动失败了!\r\n");
	}
	CloseHandle(m_eventInvoke);

	m_buffer.resize(BUFSIZE);
	memset(m_buffer.data(), 0, BUFSIZE);
	
	//初始化消息和消息函数对应的映射表
	struct {
		UINT message;
		MSGFUNC func;
	}funcs[] = {
		{WM_SEND_PACK,&CClientSocket::SendPack},
		{0,NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++)
	{
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false)
		{
			TRACE("插入失败,消息值：%d 函数值%08X 序号:%d\r\n", funcs[i].message, funcs[i].func, i);
		}
	}
	
};

//析构
CClientSocket::~CClientSocket() {
	closesocket(m_sock);
	m_sock = INVALID_SOCKET;
	WSACleanup();

}
CClientSocket::CClientSocket(const CClientSocket&  ss)
{
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClose = ss.m_bAutoClose;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != ss.m_mapFunc.end(); it++)
	{
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
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

//发送消息 负责PostThreadMessage()发送消息  将数据+数据大小+长短连接+附加参数 打包成WPARAM传递给消息
bool CClientSocket::SendPacketMessage(HWND hWnd,const CPacket& pack, bool isAutoClosed, WPARAM AttParam)
{
	//向通信线程发送消息
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);//pack格式化为字符串
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, AttParam);
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData,(LPARAM)hWnd);
	if (ret == false)
	{
		delete pData;
	}
	return ret;
}

bool CClientSocket::Send(const CPacket& pack)
{
	if (m_sock == -1)return false;
	//TRACE("[客户端]准备发送数据%d\r\n", pack.sCmd);
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
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


unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

//通信线程
void CClientSocket::threadFunc()
{
	SetEvent(m_eventInvoke);//主线程在阻塞等待事件被唤醒(构造函数中)

	//消息循环 等待发送包事件
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end())
		{
			//调用消息处理成员函数
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}
//通信处理函数 wParam:数据  lParam:回调窗口句柄
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	//消息数据结构（数据,模式，附加参数）
	//回调消息数据结构（HWND）
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam;
	HWND hWnd = (HWND)lParam;
	
	if (InitSocket() == true)
	{	
		//发送数据
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0)
		{
			//接收数据
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFSIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET)
			{
				int len = recv(m_sock, pBuffer+index, BUFSIZE-index, 0);
				if ((len > 0)||(index>0))
				{
					index += (size_t)len;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0)//解包成功
					{
						//将收到的包通过消息发送到指定窗口 
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.AttParam);//wParam为附加参数
						if (data.nMode & CSM_AUTOCLOSE)//短连接自动关闭
						{
							CloseSocket();
							return;
						}
						index -= nLen;
						memmove(pBuffer, pBuffer + nLen, index);
					}
					
				}
				else//关闭套接字或者网络异常
				{
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
				}
			}
		}
		else
		{
			CloseSocket();
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else
	{
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
}