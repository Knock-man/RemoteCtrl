#pragma once
#include"framework.h"
#include"pch.h"
#include "string"

#define BUFSIZE 4096
#define PORT 7654

#pragma pack(push)
#pragma pack(1)
//�� [��ͷ2 ������4 ��������2 ������ ��У��2]
class CPacket
{
public:
	CPacket();
	CPacket(const BYTE* pData, size_t& nSize);//���
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize);//���
	CPacket(const CPacket& pack);
	CPacket& operator=(const CPacket& pack);
	~CPacket();

	int size();//����С
	const char* Data();//��

public:
	//WORD:unsiged short(2�ֽ�)		DWORD:unsigned long(4�ֽ�)
	WORD sHead;//��ͷ FEFF  
	DWORD nLength;//�����ȣ��ӿ������ʼ������У������� 
	WORD sCmd;//��������
	std::string strDate;//������
	WORD sSum;//��У��
	std::string strOut;//������������
};
#pragma pack(pop)
//���ṹ��
typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//������ƶ���˫��
	WORD nButton;//������Ҽ�������
	POINT ptXY;//����
}MOUSEEV, * PMOUSEEV;

class CClientSocket
{
public:
	//��ȡ����ʵ���ӿ�
	static CClientSocket* getInstance();


	//�׽��ֳ�ʼ��
	bool InitSocket(const std::string& strIPAddress);


	//������Ϣ
	int DealCommand();

	//������Ϣ
	bool Send(const void* pData, size_t nSize);
	bool Send(CPacket& pack);

	//��ȡ�ļ��б�
	bool GetFilePath(std::string& strPath);

	bool GetMouseEvent(MOUSEEV& mouse);
private:
	//�׽���
	SOCKET m_sock;

	//���ݰ�
	CPacket m_packet;

	//����
	CClientSocket();

	//����
	~CClientSocket();

	CClientSocket(const CClientSocket&);
	CClientSocket& operator=(const CClientSocket&) {};

	//��ʼ�����绷��
	BOOL InitSockEnv();
};

