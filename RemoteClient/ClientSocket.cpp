#include "pch.h"
#include "ClientSocket.h"


//���������
//����
CClientSocket::CClientSocket() {
	if (InitSockEnv() == FALSE)
	{
		MessageBox(NULL, TEXT("�޷���ʼ���׽��ִ���,������������"), TEXT("��ʼ������"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_buffer.resize(BUFSIZE);
	//m_sock = socket(AF_INET, SOCK_STREAM, 0);
};

//����
CClientSocket::~CClientSocket() {
	closesocket(m_sock);
	WSACleanup();

}
CClientSocket::CClientSocket(const CClientSocket&  ss)
{
	m_sock = ss.m_sock;
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
bool CClientSocket::InitSocket(int nIP,int nPort)
{
	//�����ڹ����ʱ���ʼ���׽��֣���Ϊ����ģʽ�Ķ������������Ǻͳ���һ���ģ����Գ���ֻҪû�йرգ���һ�ε��׽������ɴ��ڣ����������·����׽���
	if (m_sock != -1)CloseSocket();//��֤������׽������µ��׽���
	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sock == -1)return false;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_addr.s_addr = htonl(nIP);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(nPort);
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
	memset(buffer, 0, BUFSIZE);
	size_t index = 0;//����������λ��ָ�루ʵ�ʴ洢���ݴ�С��
	while (true)
	{
		size_t len = recv(m_sock, buffer + index, BUFSIZE - index, 0);
		if (len < 0)
		{
			return -1;
		}
		index += len;
		len = index;
		m_packet = CPacket((BYTE*)buffer, len);//len���룺buffer���ݳ���   �������ѽ������ݳ���
		if (len > 0)//�����ɹ�
		{
			memmove(buffer, buffer + len, BUFSIZE - len);//ʣ����������Ƶ�������ͷ��
			index -= len;
			return m_packet.sCmd;
		}
	}
	return -1;
}

//����
bool CClientSocket::Send(const void* pData, size_t nSize)
{
	if (m_sock == -1)return false;
	return send(m_sock, (const char*)pData, nSize, 0) > 0;
}

bool CClientSocket::Send(CPacket& pack)
{
	if (m_sock == -1)return false;
	TRACE("[�ͻ���]׼����������%d\r\n", pack.sCmd);
	return send(m_sock, pack.Data(), pack.size(), 0) > 0;
}

bool CClientSocket::GetFilePath(std::string& strPath)
{
	if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4))
	{
		strPath = m_packet.strDate;
		return true;
	}
	return false;
}

//��ȡ����¼�
bool CClientSocket::GetMouseEvent(MOUSEEV& mouse)
{
	if (m_packet.sCmd == 5)
	{
		memcpy(&mouse, m_packet.strDate.c_str(), sizeof(MOUSEEV));
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



//����

CPacket::CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0)
{

}
//�����������
CPacket::CPacket(const BYTE* pData, size_t& nSize)
{

	//�� [��ͷ2 ������4 ��������2 ������2 ��У��2]
	size_t i = 0;
	//ȡ��ͷλ
	for (; i < nSize; i++)
	{
		if (*(WORD*)(pData + i) == 0xFEFF)//�ҵ���ͷ
		{
			sHead = *(WORD*)(pData + i);
			//i++;//ƫ�Ƶ���ͷĩβ
			break;
		}
	}

	if ((i + 2 + 4 + 2 + 2) > nSize)//�����ݲ�ȫ ֻ�� [��ͷ ������ �������� ��У��]  û�����ݶ� ����ʧ��
	{
		nSize = 0;
		return;
	}

	//ȡ������λ
	nLength = *(DWORD*)(pData + i + 2);
	if (nLength + 2 + 4 > nSize)//��δ��ȫ���յ� nLength+sizeof(��ͷ)+sizeof(������) pData������Խ����
	{
		nSize = 0;
		return;
	}

	//ȡ����������λ
	sCmd = *(WORD*)(pData + i + 2 + 4);

	//�������ݶ�
	int dataLength = nLength - 2 - 2;//���ݶγ���
	if (nLength > 4)
	{
		strDate.resize(dataLength);//nLength - [��������λ����] - [У��λ����]
		memcpy((void*)strDate.c_str(), pData + 8, dataLength);
	}

	//ȡ��У��λ ��У��
	sSum = *(pData + i + 2 + 4 + 2 + dataLength);

	WORD sum = 0;
	for (int j = 0; j < strDate.size(); j++)
	{
		sum += BYTE(strDate[j]) & 0xFF;//ֻȡ�ַ��Ͱ�λ
	}
	if (sum == sSum)
	{
		nSize = nLength + 2 + 4;
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
		strDate.resize(nSize);
		memcpy((void*)strDate.c_str(), pData, nSize);
	}
	else//�����ݶ�
	{
		strDate.clear();
	}

	//�������λ
	sSum = 0;
	for (int j = 0; j < strDate.size(); j++)
	{
		sSum += BYTE(strDate[j]) & 0xFF;//ֻȡ�ַ��Ͱ�λ
	}
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
	return nLength + 6;
}

const char* CPacket::Data()
{
	strOut.resize(nLength + 6);
	BYTE* pData = (BYTE*)strOut.c_str();
	*(WORD*)pData = sHead;
	*(WORD*)(pData + 2) = nLength;
	*(WORD*)(pData + 2 + 4) = sCmd;
	memcpy(pData + 2 + 4 + 2, strDate.c_str(), strDate.size());
	*(WORD*)(pData + 2 + 4 + 2 + strDate.size()) = sSum;
	return strOut.c_str();
}

