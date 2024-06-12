#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "CEdoyunTool.h"
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
		static CClientController instance;
		return &instance;
	}
	//��ʼ������
	int InitController();

	//����
	int Invoke(CWnd*& m_pMainWnd);

	//������Ϣ
	LRESULT SendMessage(MSG msg);

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
	void CloseSocket()
	{
		CClientSocket::getInstance()->CloseSocket();
	}

	//��������
	int SendCommandPacket(int nCmd, bool bAutoClose=true, BYTE* pData=NULL, size_t nLength=0, std::list<CPacket>* plstPacks=NULL)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		//if (pClient->InitSocket() == false)return false;
		HANDLE hEVent = CreateEvent(NULL, TRUE, FALSE, NULL);
		//��Ӧ��ֱ�ӷ��ͣ�����Ͷ�����
		std::list<CPacket> lstPacks;//��ʱӦ������
		if (plstPacks == NULL)
		{
			plstPacks = &lstPacks;//����Ҫ���ط���ֵ
		}
		pClient->SendPacket(CPacket(nCmd, pData, nLength, hEVent),*plstPacks,bAutoClose);
		CloseHandle(hEVent);//�����¼������ֹ��Դ�ľ�
		if (plstPacks->size() > 0)
		{	
			return plstPacks->front().sCmd;
		}
		return -1;
	}

	//���ͼƬ
	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		return CEdoyunTool::Bytes2Image(image, pClient->GetPacket().strData);
	}
	int DownFile(CString strPath)//strPathΪԶ�������ļ�·��
	{
		//�������ضԻ���
		CFileDialog dlg(FALSE, NULL,
			strPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY
			, NULL, &m_remoteDlg);
		if (dlg.DoModal() == IDOK)//���ȷ������
		{
			m_strRemote = strPath;//Զ���ļ�·����
			m_strLocal = dlg.GetPathName();//���ر����ļ����ļ���
			//�����߳�����
			m_hThreadDown = (HANDLE)_beginthread(&CClientController::threadDownloadFileEntry, 0, this);//�����߳�
			if (WaitForSingleObject(m_hThreadDown, 0) != WAIT_TIMEOUT)//�߳̿���ʧ��
			{
				return -1;
			}
			m_remoteDlg.BeginWaitCursor();//����ɳ©
			m_StatusDlg.m_info.SetWindowText(TEXT("��������ִ����"));
			m_StatusDlg.ShowWindow(SW_SHOW);//��ʾ�Ի���
			m_StatusDlg.CenterWindow(&m_remoteDlg);//�Ի������
			m_StatusDlg.SetActiveWindow();//����Ϊ���㴰��
		}
		return 0;
		
		
	}

	void StartWatchScreen()
	{
		m_isClosed = false;
		//m_watchDlg.SetParent(&m_remoteDlg);
		m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry, 0, this);
		m_watchDlg.DoModal();//����ģ̬�Ի�������ɺ󷵻ء�
		m_isClosed = true;
		//������ǰ�̣߳��ȴ�m_hThreadWatch�߳̽���������500����
		WaitForSingleObject(m_hThreadWatch, 500);
	}
protected:
	//�����ļ��߳�
	void threadDownloadFile();
	static void threadDownloadFileEntry(void* arg);

	//����߳�
	void threadWatchScreen();
	static void threadWatchScreenEntry(void* arg);

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
	//LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	//LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
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
	bool m_isClosed;//�����Ƿ�ر�

	//�߳����
	HANDLE m_hThread;
	unsigned int m_nThreadID;
	HANDLE m_hThreadDown;
	HANDLE m_hThreadWatch;

	//�����ļ���Զ��·��
	CString m_strRemote;
	//�����ļ��ı��ر���·��
	CString m_strLocal;
	
	static CClientController* m_instance;
};