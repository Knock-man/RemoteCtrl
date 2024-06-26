/*
���Ʋ�
�����ͽ������� ʵ��ҵ������߼�

���ʵ�֣�
	�����߳� ��һ��Ƶ������������ͽ�ͼ����(ͼƬ����Ϊ��ʱ)
*/

#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "CEdoyunTool.h"
#include "resource.h"
#include "StatusDlg.h"
#include <map>

class CClientController
{
public:
	//��ȡ����ʵ���ӿ�
	static CClientController* getInstance()
	{
		static CClientController instance;
		return &instance;
	}

	CClientController();
	~CClientController() {};

	//��ʼ������ ����״̬�Ի���
	int InitController();

	//����RemoteClientDlgΪ�����ڣ���Ϊģ̬�Ի���
	int Invoke(CWnd*& m_pMainWnd);

	//�������ݽӿ� ����ͼ��ʹ�� ��������㷢�����ݽӿ�  
	//����ֵ�Ƿ���״̬
	bool SendCommandPacket(HWND hWnd,//���ݰ��յ�����ҪӦ��Ĵ���
		int nCmd, //��������
		bool bAutoClose = true,//�������� 
		BYTE* pData = NULL, //����(δ���)
		size_t nLength = 0,//���ݳ���
		WPARAM wParam = 0)//���Ӳ���
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		bool ret = pClient->SendPacketMessage(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
		return ret;
	}

	//���������������ַ
	void UpdateAddress(int nIP, int nPort)
	{
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}

	//��������
	int DealCommand()
	{
		return CClientSocket::getInstance()->DealCommand();
	}
	//�ر���������
	void CloseSocket()
	{
		CClientSocket::getInstance()->CloseSocket();
	}

	//���ͼƬ  ����˽��յ������� תΪͼƬ��ʽ����
	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		return CEdoyunTool::Bytes2Image(image, pClient->GetPacket().strData);
	}

	//�����ļ� ���������ļ��Ի���  ���������ļ����� ��������״̬�Ի���
	int DownFile(CString strPath);//strPathΪԶ�������ļ�·��
	
	//��ȡ���統ǰ�������ݰ�
	CPacket GetPacket() const
	{
		return CClientSocket::getInstance()->GetPacket();
	}

	//���ؽ��� ���նԻ��� �������������ʾ
	void DownloadEnd()
	{
		m_StatusDlg.ShowWindow(SW_HIDE);//��������״̬�Ի���
		m_remoteDlg.EndWaitCursor();//����ɳ©
		m_remoteDlg.MessageBox(TEXT("�������!!"), TEXT("���"));
	}

	//��������߳� �򿪼��ӶԻ��� �����ȴ�
	void StartWatchScreen();
	
protected:
	//����߳� ��һ��Ƶ�ʷ��ͼ������
	void threadWatchScreen();
	static void threadWatchScreenEntry(void* arg);

//�Զ�����Ϣ�ṹ��
private:
	CWatchDialog m_watchDlg;//��ضԻ���
	CRemoteClientDlg m_remoteDlg;//Զ�̿��ƶԻ���
	CStatusDlg m_StatusDlg;//����״̬���Ի���
	bool m_isClosed;//Զ�̼���Ƿ�ر�

	//�߳����
	unsigned int m_nThreadID;
	HANDLE m_hThreadWatch;//����߳̾��

	//�����ļ���Զ��·��
	CString m_strRemote;
	//�����ļ��ı��ر���·��
	CString m_strLocal;
};