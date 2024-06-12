#pragma once
#include"framework.h"
#include"pch.h"
#include "string"
#include<list>
#include<map>
#include <vector>
#define BUFSIZE 2048000
#define PORT 9527

#pragma pack(push)
#pragma pack(1)
//�� [��ͷ2 ������4 ��������2 ������ ��У��2]
class CPacket
{
public:
	CPacket();
	CPacket(const BYTE* pData, size_t& nSize);//���
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize, HANDLE hEvent);//���
	CPacket(const CPacket& pack);
	CPacket& operator=(const CPacket& pack);
	~CPacket();

	int size();//����С
	const char* CPacket::Data(std::string& strOut) const;//��

public:
	//WORD:unsiged short(2�ֽ�)		DWORD:unsigned long(4�ֽ�)
	WORD sHead;//��ͷ FEFF  
	DWORD nLength;//�����ȣ��ӿ������ʼ������У������� 
	WORD sCmd;//��������
	std::string strData;//������
	WORD sSum;//��У��
	HANDLE hEvent;
};
#pragma pack(pop)
//�ļ���Ϣ�ṹ��
typedef struct file_info
{
	file_info()
	{
		IsInvalid = false;//Ĭ��Ϊ��Ч�ļ�
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//�Ƿ���Ч
	BOOL IsDirectory;//�ļ����� 0�ļ� 1Ŀ¼
	BOOL HasNext;//�Ƿ��к��� 0û�� 1��
	char szFileName[256];//�ļ���
}FILEINFO, * PFILEINFO;
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
	bool CClientSocket::InitSocket();

	void CloseSocket();

	//������Ϣ
	int DealCommand();

	//������Ϣ
	bool Send(const void* pData, size_t nSize);
	bool Send(const CPacket& pack);

	//��ȡ�ļ��б�
	bool GetFilePath(std::string& strPath);

	bool GetMouseEvent(MOUSEEV& mouse);

	CPacket& GetPacket();

	void UpdateAddress(int nIP, int nPort)
	{
		m_nIP = nIP;
		m_nPort = nPort;
	}
private:
	std::list<CPacket>m_lstSend;
	std::map<HANDLE, std::list<CPacket>> m_mapAck;
	int m_nIP;//��ַ
	int m_nPort;//�˿�
private:
	//�׽���
	SOCKET m_sock;

	//���ݰ�
	CPacket m_packet;

	std::vector<char> m_buffer;

	//����
	CClientSocket();

	//����
	~CClientSocket();

	CClientSocket(const CClientSocket&);
	CClientSocket& operator=(const CClientSocket&) {};

	//��ʼ�����绷��
	BOOL InitSockEnv();

	static void threadEntry(void* arg);
	void threadFunc();

};