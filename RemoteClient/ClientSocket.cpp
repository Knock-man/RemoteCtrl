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

bool CClientSocket::SendPacket(HWND hWnd,const CPacket& pack, bool isAutoClosed,WPARAM wParam)
{
	//向通信线程发送消息
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData,(LPARAM)hWnd);
	if (ret == false)
	{
		delete pData;
	}
	return ret;
}

/*
bool CClientSocket::SendPacket(const CPacket& pack,std::list<CPacket>& lstPacks,bool isAutoClosed)//lstPacks储存结果
{
	//开启通信线程
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE)
	{
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
	}
	m_lock.lock();
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));//接收Map
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent,isAutoClosed));//长短连接标记
	m_lstSend.push_back(pack);//加入到发送队列
	m_lock.unlock();

	WaitForSingleObject(pack.hEvent, INFINITE);//无限阻塞，直至被唤醒(数据接收完成，m_hThread线程会唤醒)

	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.hEvent);
	if (it != m_mapAck.end())
	{
		m_lock.lock();
		m_mapAck.erase(it);
		m_lock.unlock();
		return true;
	} 
	return false;
}
*/


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


unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

//通信线程
/*
void CClientSocket::threadFunc()
{
	std::string strBuffer;
	strBuffer.resize(BUFSIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;

	InitSocket();//初始化网络
	while (m_sock != INVALID_SOCKET)
	{
		if (m_lstSend.size() > 0)//等待发送队列不为空
		{
			TRACE("lstSend size:%d\r\n", m_lstSend.size());
			m_lock.lock();
			CPacket& head = m_lstSend.front();//取出请求队列队头数据包
			m_lock.unlock();
			if (Send(head) == false)
			{
				TRACE("发送失败!\r\n");
				continue;
			}
			//发送成功
			std::map<HANDLE, std::list<CPacket>&>::iterator it = m_mapAck.find(head.hEvent);//找到储存结果对于的key
			if (it != m_mapAck.end())
			{
				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);//取网络长短连接标记
				do
				{
					//等待应答
					int len = recv(m_sock, pBuffer + index, BUFSIZE - index, 0);
					if ((len > 0) || (index > 0))
					{
						index += len;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);
						if (size > 0)//成功解包
						{
							//将接收到的包储存到事件key对应的value列表中(list<CPacket>)
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it0->second)//事件是短连接(如鼠标事件，查看锁机事件，收到一个包网络就可以关闭了)
							{
								SetEvent(head.hEvent);//短连接收一个包立马通知主线程接收完成
								break;
							}
						}
					}
					else if (len <= 0 && index <= 0)//对端网络关闭，缓冲区也读取完毕
					{
						CloseSocket();
						SetEvent(head.hEvent);//长连接所有包接收完成/服务器关闭命令之后,再通知主线程接收完成
						if (it0 != m_mapAutoClosed.end())
						{
							TRACE("SetEvent %d %d\r\n", head.sCmd, it0->second);
						}
						else
						{
							TRACE("异常情况，没有对应的pair\r\n");
						}
						break;
					}
				} while (it0->second == false);//保证长连接继续接收，如文件下载会分为多个包发送过来

				m_lock.lock();
				m_lstSend.pop_front();//处理完一个请求，从请求队列中弹出来
				m_mapAutoClosed.erase(head.hEvent);//删除长短标记
				m_lock.unlock();
				if (InitSocket() == false)InitSocket();
			}
		}
		Sleep(1);
	}
	CloseSocket();

}
*/
//通信线程
void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke);//主线程在阻塞等待事件被唤醒(构造函数中)

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
//通信处理函数
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	//消息数据结构（数据和数据长度,模式）
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
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);//wParam为附加参数
						if (data.nMode & CSM_AUTOCLOSE)//短连接自动关闭
						{
							CloseSocket();
							return;
						}
					}
					index -= nLen;
					memmove(pBuffer, pBuffer + index, nLen);
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
	TRACE("[客户端] sHead=%d nLength=%d sCmd=%d data=[%s]  sSum=%d  sum = %d\r\n", sHead, nLength, sCmd, strData.c_str(), sSum, sum);
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

