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
	//��ʼ����ַ�ṹ
	if (m_servsock == -1)return false;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//�����������е�ַ
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
	m_callback = callback;//ҵ���ص�����
	m_arg_cmdObject = cmdObject;//commad����
	//�����ʼ��
	bool ret  = InitSocket();
	if (ret == false)return -1;

	std::list<CPacket> listPacket;//��ȡҵ��㴦����

	int count = 0;
	while (true)
	{
		//��������
		if (AcceptClient() == false)
		{
			if (count >= 3) {
				return -2;
			}
			count++;//�������ӻ���
			continue;
		}
		//��������
		int rcmd = DealCommand();//retΪ��������
		if (rcmd > 0)
		{
			//ִ����Ӧ����
			m_callback(m_arg_cmdObject,rcmd,m_packet,listPacket);
			while (listPacket.size() > 0) {//������ȫ�����ͳ�ȥ
				Send(listPacket.front());
				listPacket.pop_front();
			}
		}
		CloseSocket();//�ر��׽���
	}
	
	return 0;
}

//���տͻ�������
bool CServerSocket::AcceptClient()
{
	sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	m_clntsock = accept(m_servsock, (sockaddr*)&client_addr, &client_addr_len);//����
	if (m_clntsock == -1)return false;
	return true;
}

//������Ϣ	��� ����ֵ����������
int CServerSocket::DealCommand()
{
	if (m_clntsock == -1)return -1;
	char* buffer = m_buffer.data();
	static size_t index = 0;//����������λ��ָ�루ʵ�ʴ洢���ݴ�С��
	while (true)
	{
		size_t len = recv(m_clntsock, buffer + index, BUFSIZE - index, 0);
		//TRACE("[������]buff=%s  buffSize=%d\r\n", buffer, index + len);
		if ((len <= 0) && (index <= 0))//�Ͽ�����/����ĩβ
		{
			return -1;
		}
		index += len;
		len = index;
		m_packet = CPacket((BYTE*)buffer, len);//len���룺buffer���ݳ���   len�������ɹ��������ݳ���
		if (len > 0)//�����ɹ�
		{
			memmove(buffer, buffer + len, index - len);//������ʣ����������Ƶ�������ͷ��
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

//�������� ֱ�ӷ����ַ���
bool CServerSocket::Send(const void* pData, size_t nSize)
{
	if (m_clntsock == -1)return false;
	return send(m_clntsock, (const char*)pData, nSize, 0) > 0;
}

//���Ͱ� �ȰѰ�ת��Ϊ�ַ��� �ٷ���
bool CServerSocket::Send(CPacket& pack)
{
	if (m_clntsock == -1)return false;
	int ret = send(m_clntsock, pack.Data(), pack.size(), 0);
	//TRACE("[������]����%d���ֽ�\r\n", ret);
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

//��ȡ����¼�
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

//���绷����ʼ��
BOOL  CServerSocket::InitSockEnv() {
	WSAData data;
	if (WSAStartup(MAKEWORD(2,0), &data))
	{
		return FALSE;
	}
	return TRUE;
}





