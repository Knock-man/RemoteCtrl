#pragma once
#include "framework.h"
#include "pch.h"
#include <string>
#include "Packet.h"
#include<list>
#include<map>
#include <vector>
#include<mutex>
#define BUFSIZE 4096000
#define PORT 9527
#define WM_SEND_PACK (WM_USER+1) //���Ͱ����� ��Ϣ
#define WM_SEND_PACK_ACK (WM_USER+2) //���Ͱ�����Ӧ�� ��Ϣ

//����ͨ����
class CClientSocket
{
public:
	//��ȡ����ʵ���ӿ�
	static CClientSocket* getInstance();

	//�׽��ֳ�ʼ��
	bool CClientSocket::InitSocket();

	//�ر�
	void CloseSocket();

	//������Ϣ
	int DealCommand();

	//bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks,bool isAutoClosed=true);
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed=true, WPARAM wParam = 0);

	//��ȡ���ݰ�
	CPacket& GetPacket();

	//���������ַ
	void UpdateAddress(int nIP, int nPort)
	{
		if ((m_nIP != nIP) || (m_nPort != nPort))//�޸��� IP �˿�
		{
			m_nIP = nIP;
			m_nPort = nPort;
		}		
	}
private:
	//����ָ�� ָ����Ϣ����
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);

	//����
	std::map<UINT, MSGFUNC> m_mapFunc;
	std::map<HANDLE, bool>m_mapAutoClosed;//�¼���������ӳ���
	std::map<HANDLE, std::list<CPacket>&> m_mapAck;//���ս��ӳ���
	std::list<CPacket>m_lstSend;//���Ͷ���

	HANDLE m_eventInvoke;//�����¼�
	UINT m_nThreadID;
	HANDLE m_hThread;
	
	bool m_bAutoClose;//��������
	std::mutex m_lock;
	
	
	int m_nIP;//��ַ
	int m_nPort;//�˿�
private:
	//�׽���
	SOCKET m_sock;

	//���ݰ�
	CPacket m_packet;

	//������
	std::vector<char> m_buffer;

	//����
	CClientSocket();

	//����
	~CClientSocket();

	CClientSocket(const CClientSocket&);
	CClientSocket& operator=(const CClientSocket&) {};

	//��ʼ�����绷��
	BOOL InitSockEnv();

	//������Ϣ
	//bool Send(const void* pData, size_t nSize);
	bool Send(const CPacket& pack);

	//void threadFunc();
	static unsigned _stdcall threadEntry(void* arg);
	void threadFunc2();//��Ϣѭ��

	//��Ϣ����  WM_SEND_PACK��Ϣ����
	void SendPack(UINT nMsg, WPARAM wParam/*��������ֵ*/, LPARAM lParam/*�������ĳ���*/);
	
	
};