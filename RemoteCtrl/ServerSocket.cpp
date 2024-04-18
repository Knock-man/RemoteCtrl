#include "pch.h"
#include "ServerSocket.h"


//���������
//����
CServerSocket::CServerSocket() {
	m_clntsock = -1;
	if (InitSockEnv() == FALSE)
	{
		MessageBox(NULL, TEXT("�޷���ʼ���׽��ִ���,������������"), TEXT("��ʼ������"), MB_OK | MB_ICONERROR);
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

//���տͻ�������
bool CServerSocket::AcceptClient()
{
	sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	m_clntsock = accept(m_servsock, (sockaddr*)&client_addr, &client_addr_len);
	if (m_clntsock == -1)return false;
	return true;
}

//������Ϣ
int CServerSocket::DealCommand()
{
	if (m_clntsock == -1)return -1;
	char* buffer = m_buffer.data();
	static size_t index = 0;//����������λ��ָ�루ʵ�ʴ洢���ݴ�С��
	while (true)
	{
		size_t len = recv(m_clntsock, buffer + index, BUFSIZE - index, 0);
		//TRACE("[������]buff=%s  buffSize=%d\r\n", buffer, index + len);
		if ((len <= 0) && (index <= 0))
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

void CServerSocket::CloseSocket()
{
	closesocket(m_clntsock);
	m_clntsock = -1;
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
	int ret = send(m_clntsock, pack.Data(), pack.size(), 0);
	//TRACE("[������]����%d���ֽ�\r\n", ret);
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

//��ȡ����¼�
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
// ������
CPacket::CPacket(const BYTE * pData, size_t & nSize)
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
		strDate.resize(nLength - 2 - 2);//nLength - [��������λ����] - [У��λ����]
		memcpy((void*)strDate.c_str(), pData + i, nLength - 4);
		i = i + nLength - 2 - 2;
	}

	//ȡ��У��λ ��У��
	sSum = *(WORD*)(pData + i); i += 2;
	WORD sum = 0;
	for (size_t j = 0; j < strDate.size(); j++)
	{
		sum += BYTE(strDate[j]) & 0xFF;//ֻȡ�ַ��Ͱ�λ
	}
	//TRACE("[�ͻ���] sHead=%d nLength=%d data=[%s]  sSum=%d  sum = %d\r\n", sHead, nLength, strDate.c_str(), sSum, sum);
	if (sum == sSum)
	{
		nSize = i;
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
	for (size_t j = 0; j < strDate.size(); j++)
	{
		sSum += BYTE(strDate[j]) & 0xFF;//ֻȡ�ַ��Ͱ�λ
	}
	//TRACE("[������] sHead=%d nLength=%d data=[%s]  sSum=%d\r\n", sHead, nLength, strDate.c_str(), sSum);
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

//����תΪ�ַ�������
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

