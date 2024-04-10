#pragma once
#include"framework.h"
#include"pch.h"

#define BUFSIZE 4096

//�� [��ͷ ������ �������� ������ ��У��]
class CPacket
{
public:
	CPacket();
	CPacket(const BYTE* pData, size_t& nSize);
	CPacket(const CPacket& pack);
	CPacket& operator=(const CPacket& pack);
	~CPacket();

public:
	//WORD:unsiged short(2�ֽ�)		DWORD:unsigned long(4�ֽ�)
	WORD sHead;//��ͷ FEFF  
	DWORD nLength;//�����ȣ��ӿ������ʼ������У������� 
	WORD sCmd;//��������
	std::string strDate;//������
	WORD sSum;//��У��
};

class CServerSocket
{
public:
	//��ȡ����ʵ���ӿ�
	static CServerSocket* getInstance();


	//�׽��ֳ�ʼ��
	bool InitSocket();

	//��������
	bool AcceptClient();

	//������Ϣ
	int DealCommand();

	//������Ϣ
	bool Send(const void* pData, size_t nSize);

private:
	//�׽���
	SOCKET m_servsock;
	SOCKET m_clntsock;

	//���ݰ�
	CPacket m_packet;

	//����
	CServerSocket();

	//����
	~CServerSocket();

	CServerSocket(const CServerSocket&) = delete;
	CServerSocket& operator=(const CServerSocket&) = delete;

	//��ʼ�����绷��
	BOOL InitSockEnv();
};

