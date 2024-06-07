#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "resource.h"
#include "StatusDlg.h"
#include <map>

#define WM_SEND_PACK (WM_USER+1)//���Ͱ�����
#define WM_SEND_DATA (WM_USER+2)//��������
#define WM_SHOW_STATUS (WM_USER+3)//չʾ״̬
#define WM_SHOW_WATCH (WM_USER+4)//Զ�̼��
#define WM_SEND_MESSAGE (WM_USER+0x1000)//�Զ�����Ϣ����

class CClientController
{
public:
	//��ȡ����ʵ���ӿ�
	static CClientController* getInstance()
	{
		static CClientController server;
		return &server;
	}
	//��ʼ������
	int InitController();

	//����
	int Invoke(CWnd*& m_pMainWnd);

	//������Ϣ
	LRESULT SendMessage(MSG msg);
protected:
	CClientController();

	~CClientController()
	{
		//�����߳�
		WaitForSingleObject(m_hThread, 100);

	}

	//�̺߳���
	void threadFunc();
	static unsigned __stdcall ThreadEntry(void* arg);

	//ɾ������
	static void releaseInstance()
	{
		if (m_instance == NULL)
		{
			delete m_instance;
			m_instance = NULL;
		}

	}

	//��Ϣ������
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);
private:
	typedef struct MsgInfo
	{
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m)//�вι���
		{
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));

		}

		MsgInfo(const MsgInfo& m)//��������
		{
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));

		}

		MsgInfo& operator=(const MsgInfo& m)
		{
			if (this != &m)
			{
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));

			}
			return *this;
		}
	}MSGINFO;

	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC> m_mapFunc;//<�źţ�ִ�к���>
	CWatchDialog m_watchDlg;//��ضԻ���
	CRemoteClientDlg m_remoteDlg;//Զ�̿��ƶԻ���
	CStatusDlg m_StatusDlg;//״̬���Ի���
	HANDLE m_hThread;
	unsigned int m_nThreadID;
	static CClientController* m_instance;
};