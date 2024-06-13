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

bool CClientSocket::SendPacket(HWND hWnd,const CPacket& pack, bool isAutoClosed,WPARAM wParam)
{
	//��ͨ���̷߳�����Ϣ
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
bool CClientSocket::SendPacket(const CPacket& pack,std::list<CPacket>& lstPacks,bool isAutoClosed)//lstPacks������
{
	//����ͨ���߳�
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE)
	{
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
	}
	m_lock.lock();
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));//����Map
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent,isAutoClosed));//�������ӱ��
	m_lstSend.push_back(pack);//���뵽���Ͷ���
	m_lock.unlock();

	WaitForSingleObject(pack.hEvent, INFINITE);//����������ֱ��������(���ݽ�����ɣ�m_hThread�̻߳ỽ��)

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


//����
bool CClientSocket::Send(const void* pData, size_t nSize)
{
	if (m_sock == -1)return false;
	return send(m_sock, (const char*)pData, nSize, 0) > 0;
}

bool CClientSocket::Send(const CPacket& pack)
{
	if (m_sock == -1)return false;
	//TRACE("[�ͻ���]׼����������%d\r\n", pack.sCmd);
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

//��ȡ����¼�
bool CClientSocket::GetMouseEvent(MOUSEEV& mouse)
{
	if (m_packet.sCmd == 5)
	{
		memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
		return true;
	}
	return false;
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
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

//ͨ���߳�
/*
void CClientSocket::threadFunc()
{
	std::string strBuffer;
	strBuffer.resize(BUFSIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;

	InitSocket();//��ʼ������
	while (m_sock != INVALID_SOCKET)
	{
		if (m_lstSend.size() > 0)//�ȴ����Ͷ��в�Ϊ��
		{
			TRACE("lstSend size:%d\r\n", m_lstSend.size());
			m_lock.lock();
			CPacket& head = m_lstSend.front();//ȡ��������ж�ͷ���ݰ�
			m_lock.unlock();
			if (Send(head) == false)
			{
				TRACE("����ʧ��!\r\n");
				continue;
			}
			//���ͳɹ�
			std::map<HANDLE, std::list<CPacket>&>::iterator it = m_mapAck.find(head.hEvent);//�ҵ����������ڵ�key
			if (it != m_mapAck.end())
			{
				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);//ȡ���糤�����ӱ��
				do
				{
					//�ȴ�Ӧ��
					int len = recv(m_sock, pBuffer + index, BUFSIZE - index, 0);
					if ((len > 0) || (index > 0))
					{
						index += len;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);
						if (size > 0)//�ɹ����
						{
							//�����յ��İ����浽�¼�key��Ӧ��value�б���(list<CPacket>)
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it0->second)//�¼��Ƕ�����(������¼����鿴�����¼����յ�һ��������Ϳ��Թر���)
							{
								SetEvent(head.hEvent);//��������һ��������֪ͨ���߳̽������
								break;
							}
						}
					}
					else if (len <= 0 && index <= 0)//�Զ�����رգ�������Ҳ��ȡ���
					{
						CloseSocket();
						SetEvent(head.hEvent);//���������а��������/�������ر�����֮��,��֪ͨ���߳̽������
						if (it0 != m_mapAutoClosed.end())
						{
							TRACE("SetEvent %d %d\r\n", head.sCmd, it0->second);
						}
						else
						{
							TRACE("�쳣�����û�ж�Ӧ��pair\r\n");
						}
						break;
					}
				} while (it0->second == false);//��֤�����Ӽ������գ����ļ����ػ��Ϊ��������͹���

				m_lock.lock();
				m_lstSend.pop_front();//������һ�����󣬴���������е�����
				m_mapAutoClosed.erase(head.hEvent);//ɾ�����̱��
				m_lock.unlock();
				if (InitSocket() == false)InitSocket();
			}
		}
		Sleep(1);
	}
	CloseSocket();

}
*/
//ͨ���߳�
void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke);//���߳��������ȴ��¼�������(���캯����)

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
//ͨ�Ŵ�����
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	//��Ϣ���ݽṹ�����ݺ����ݳ���,ģʽ��
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
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);//wParamΪ���Ӳ���
						if (data.nMode & CSM_AUTOCLOSE)//�������Զ��ر�
						{
							CloseSocket();
							return;
						}
					}
					index -= nLen;
					memmove(pBuffer, pBuffer + index, nLen);
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



//����

CPacket::CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0)
{

}
//������
CPacket::CPacket(const BYTE* pData, size_t& nSize)
{
	//�� [��ͷ2 ������4 ��������2 ������2 ��У��2]
	size_t i = 0;
	//ȡ��ͷλ
	for (; i < nSize; i++)
	{
		if ((*(WORD*)(pData + i)) == 0xFEFF)//�ҵ���ͷ
		{
			sHead = *(WORD*)(pData + i);
			i += 2;
			break;
		}
	}
	
	if ((i + 4 + 2 + 2) > nSize)//�����ݲ�ȫ ֻ�� [��ͷ ������ �������� ��У��]  û�����ݶ� ����ʧ��
	{
		nSize = 0;
		return;
	}

	//ȡ������λ
	nLength = *(DWORD*)(pData + i); i += 4;
	if (nLength + i > nSize)//��δ��ȫ���յ� nLength+sizeof(��ͷ)+sizeof(������) pData������Խ����
	{
		nSize = 0;
		return;
	}

	//ȡ����������λ
	sCmd = *(WORD*)(pData + i); i += 2;

	//�������ݶ�
	if (nLength > 4)
	{
		strData.resize(nLength-2-2);//nLength - [��������λ����] - [У��λ����]
		memcpy((void*)strData.c_str(), pData + i, nLength-4);
		i = i + nLength - 2 - 2;
	}

	//ȡ��У��λ ��У��
	sSum = *(WORD*)(pData + i); i += 2;
	WORD sum = 0;
	for (size_t j = 0; j < strData.size(); j++)
	{
		sum += BYTE(strData[j]) & 0xFF;//ֻȡ�ַ��Ͱ�λ
	}
	TRACE("[�ͻ���] sHead=%d nLength=%d sCmd=%d data=[%s]  sSum=%d  sum = %d\r\n", sHead, nLength, sCmd, strData.c_str(), sSum, sum);
	if (sum == sSum)
	{
		nSize =i;
		return;
	}
	nSize = 0;
}
//�������װ�ɰ�
CPacket::CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
{
	sHead = 0xFEFF;
	nLength = nSize + 4;
	sCmd = nCmd;


	if (nSize > 0)//�����ݶ�
	{
		//������ݶ�
		strData.resize(nSize);
		memcpy((void*)strData.c_str(), pData, nSize);
	}
	else//�����ݶ�
	{
		strData.clear();
	}

	//�������λ
	sSum = 0;
	for (size_t j = 0; j < strData.size(); j++)
	{
		sSum += BYTE(strData[j]) & 0xFF;//ֻȡ�ַ��Ͱ�λ
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

