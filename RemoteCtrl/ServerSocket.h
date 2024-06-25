/*
����ͨ��ģ��
���ã������ʼ��(socket() listen() bind() accept() send() recv() closesocket())
run()���������˻ص�����  ������ҵ���������
*/

#pragma once
#include"framework.h"
#include"pch.h"
#include"list"
#include<vector>
#include"Packet.h"

#define BUFSIZE 409600
#define PORT 9527

typedef void(*SOCKET_CALLBACK)(void* arg, int status, CPacket& inPacket, std::list<CPacket>&);//�ص�����

//����ͨ����
class CServerSocket
{
public:
	//��ȡ����ʵ���ӿ�
	static CServerSocket* getInstance();

	//ִ������ͨ�� ��ʼ������ �� �������� �� �������� �� �������� �� �ر��׽���
	int Run(void* arg,SOCKET_CALLBACK callback);

private:

	//�׽��ֳ�ʼ�� �����׽��� bind listen
	bool InitSocket();
	
	//�������� accept() �����ȴ�
	bool AcceptClient();

	//������Ϣ ���뻺���� 
	int DealCommand();

	//�������� ֱ�ӷ����ַ���
	bool Send(const void* pData, size_t nSize);
	//���Ͱ� �ȰѰ�ת��Ϊ�ַ����ٷ���
	bool Send(CPacket& pack);

	//��ȡ�ļ��б�
	/*bool GetFilePath(std::string& strPath);*/

	//��ȡ����¼�
	/*bool GetMouseEvent(MOUSEEV& mouse);*/

	//��ȡ���ݰ�
	CPacket& GetPacket();

	//�رտͻ�������
	void CloseSocket();
	
private:
	SOCKET_CALLBACK m_callback;//�ص�����
	void* m_arg_cmdObject;

	//�׽���
	SOCKET m_servsock;
	SOCKET m_clntsock;

	//���ݰ�
	CPacket m_packet;

	std::vector<char> m_buffer;

private:
	//����  ��ʼ�����绷��������������׽���
	CServerSocket();

	//����
	~CServerSocket();

	CServerSocket(const CServerSocket&);
	CServerSocket& operator=(const CServerSocket&) {};

	//��ʼ�����绷��
	BOOL InitSockEnv();
};

