#include "pch.h"
#include "ServerSocket.h"


//���������
//����
CServerSocket::CServerSocket() {
	if (InitSockEnv() == FALSE)
	{
		MessageBox(NULL, TEXT("�޷���ʼ���׽��ִ���,������������"), TEXT("��ʼ������"), MB_OK | MB_ICONERROR);
		exit(0);
	}

	m_servsock = socket(AF_INET, SOCK_STREAM, 0);
	m_clntsock = -1;
};

//����
CServerSocket::~CServerSocket() {
	closesocket(m_servsock);
	WSACleanup();

};

CServerSocket* CServerSocket::getInstance()
{
	static CServerSocket server;
	return &server;
}

//�׽��ֳ�ʼ��
bool CServerSocket::InitSocket()
{
	if (m_servsock == -1)return false;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

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

bool CServerSocket::AcceptClient()
{
	sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	m_clntsock = accept(m_servsock, (sockaddr*)&client_addr, &client_addr_len);
	if (m_clntsock == -1)return false;
	return true;
}

//����
int CServerSocket::DealCommand()
{
	if (m_clntsock == -1)return -1;
	char* buffer = new char[BUFSIZE];
	memset(buffer, 0, BUFSIZE);
	size_t index = 0;//����������λ��ָ�루ʵ�ʴ洢���ݴ�С��
	while (true)
	{
		size_t len = recv(m_clntsock, buffer+index, BUFSIZE-index, 0);
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
bool CServerSocket::Send(const void* pData, size_t nSize)
{
	if (m_clntsock == -1)return false;
	return send(m_clntsock, (const char*)pData, nSize, 0) > 0;
}

bool CServerSocket::Send(CPacket& pack)
{
	if (m_clntsock == -1)return false;
	return send(m_clntsock, pack.Data(), pack.size(), 0) > 0;
}

//���绷����ʼ��
BOOL  CServerSocket::InitSockEnv() {
	WSAData data;
	if (WSAStartup(MAKEWORD(1, 1), &data))
	{
		return false;
	}
	return TRUE;
}



//����

CPacket::CPacket():sHead(0),nLength(0),sCmd(0),sSum(0)
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

	if ((i+2+4+2+2) > nSize)//�����ݲ�ȫ ֻ�� [��ͷ ������ �������� ��У��]  û�����ݶ� ����ʧ��
	{
		nSize = 0;
		return;
	}

	//ȡ������λ
	nLength = *(DWORD*)(pData + i+2);
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
	sSum = *(pData + i + 2 + 4 + 2+ dataLength);

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
	strDate.resize(nSize);
	//������ݶ�
	memcpy((void*)strDate.c_str(), pData, nSize);
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
	return nLength+6;
}

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

