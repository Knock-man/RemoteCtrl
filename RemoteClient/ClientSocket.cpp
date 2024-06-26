#include "pch.h"
#include "ClientSocket.h"


//���������
//����
CClientSocket::CClientSocket() :


	m_nIP(INADDR_ANY), m_nPort(0),
	m_sock(INVALID_SOCKET),
	m_bAutoClose(true),
	m_hThread(INVALID_HANDLE_VALUE)
{
	if (InitSockEnv() == FALSE)
	{
		MessageBox(NULL, TEXT("�޷���ʼ���׽��ִ���,������������"), TEXT("��ʼ������"), MB_OK | MB_ICONERROR);
		exit(0);
	}

	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_eventInvoke == NULL) {
		TRACE("�����¼�ʧ��!\r\n");
	}
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);//����ͨ���߳�
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT)
	{
		TRACE("������Ϣ�����߳�����ʧ����!\r\n");
	}
	CloseHandle(m_eventInvoke);

	m_buffer.resize(BUFSIZE);
	memset(m_buffer.data(), 0, BUFSIZE);
	
	//��ʼ����Ϣ����Ϣ������Ӧ��ӳ���
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
			TRACE("����ʧ��,��Ϣֵ��%d ����ֵ%08X ���:%d\r\n", funcs[i].message, funcs[i].func, i);
		}
	}
	
};

//����
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

//�׽��ֳ�ʼ��
bool CClientSocket::InitSocket()
{
	//�����ڹ����ʱ���ʼ���׽��֣���Ϊ����ģʽ�Ķ������������Ǻͳ���һ���ģ����Գ���ֻҪû�йرգ���һ�ε��׽������ɴ��ڣ����������·����׽���
	if (m_sock != -1)CloseSocket();//��֤������׽������µ��׽���
	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sock == -1)return false;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_addr.s_addr = htonl(m_nIP);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(m_nPort);
	if (serv_addr.sin_addr.s_addr == INADDR_ANY)
	{
		AfxMessageBox("ָ����IP��ַ������");
		return false;
	}
	int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == -1)
	{
		AfxMessageBox("����ʧ��");
		TRACE("����ʧ��,%d %s\r\n", WSAGetLastError(),GetErrorInfo(WSAGetLastError()).c_str());
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

//������Ϣ
int CClientSocket::DealCommand()
{
	if (m_sock == -1)return -1;
	char* buffer = m_buffer.data();
	static size_t index = 0;//����������λ��ָ�루ʵ�ʴ洢���ݴ�С��
	while (true)
	{
		size_t len = recv(m_sock, buffer + index, BUFSIZE - index, 0);
		TRACE("[�ͻ���]len=%d buff=%s  buffSize=%d\r\n", len,buffer,index+len);
		if (((int)len <= 0)&&((int)index<=0))
		{
			return -1;
		}
		index += len;
		len = index;
		m_packet = CPacket((BYTE*)buffer, len);//len���룺buffer���ݳ���   �������ѽ������ݳ���
		if (len > 0)//�����ɹ�
		{
			memmove(buffer, buffer + len, index - len);//ʣ����������Ƶ�������ͷ��
			index -= len;
			return m_packet.sCmd;
		}
	}
	return -1;
}

//������Ϣ ����PostThreadMessage()������Ϣ  ������+���ݴ�С+��������+���Ӳ��� �����WPARAM���ݸ���Ϣ
bool CClientSocket::SendPacketMessage(HWND hWnd,const CPacket& pack, bool isAutoClosed, WPARAM AttParam)
{
	//��ͨ���̷߳�����Ϣ
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);//pack��ʽ��Ϊ�ַ���
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
	//TRACE("[�ͻ���]׼����������%d\r\n", pack.sCmd);
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}


//���绷����ʼ��
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

//ͨ���߳�
void CClientSocket::threadFunc()
{
	SetEvent(m_eventInvoke);//���߳��������ȴ��¼�������(���캯����)

	//��Ϣѭ�� �ȴ����Ͱ��¼�
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end())
		{
			//������Ϣ�����Ա����
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}
//ͨ�Ŵ����� wParam:����  lParam:�ص����ھ��
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	//��Ϣ���ݽṹ������,ģʽ�����Ӳ�����
	//�ص���Ϣ���ݽṹ��HWND��
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam;
	HWND hWnd = (HWND)lParam;
	
	if (InitSocket() == true)
	{	
		//��������
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0)
		{
			//��������
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
					if (nLen > 0)//����ɹ�
					{
						//���յ��İ�ͨ����Ϣ���͵�ָ������ 
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.AttParam);//wParamΪ���Ӳ���
						if (data.nMode & CSM_AUTOCLOSE)//�������Զ��ر�
						{
							CloseSocket();
							return;
						}
						index -= nLen;
						memmove(pBuffer, pBuffer + nLen, index);
					}
					
				}
				else//�ر��׽��ֻ��������쳣
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