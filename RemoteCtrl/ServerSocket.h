#pragma once
#include"framework.h"
#include"pch.h"
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

	//����
	CServerSocket();

	//����
	~CServerSocket();

	CServerSocket(const CServerSocket&) = delete;
	CServerSocket& operator=(const CServerSocket&) = delete;

	//��ʼ�����绷��
	BOOL InitSockEnv();
};