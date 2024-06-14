#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "CEdoyunTool.h"
#include "resource.h"
#include "StatusDlg.h"
#include <map>

//#define WM_SEND_DATA (WM_USER+2)//��������
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
	//��ʼ������ ����������Ϣ�߳�
	int InitController();

	//����ģ̬�Ի���
	int Invoke(CWnd*& m_pMainWnd);

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

	//��������  ����ֵ��״̬
	bool SendCommandPacket(HWND hWnd,//���ݰ��յ�����ҪӦ��Ĵ���
		int nCmd, //��������
		bool bAutoClose=true,//�������� 
		BYTE* pData=NULL, //����(δ���)
		size_t nLength=0,//���ݳ���
		WPARAM wParam = 0)//���Ӳ���
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		bool ret = pClient->SendPacket(hWnd,CPacket(nCmd, pData, nLength),bAutoClose, wParam);
		return ret;
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

	CClientController();
	~CClientController()
	{
		//�����߳�
		WaitForSingleObject(m_hThread, 100);
	}

	//�̺߳���
	void threadFunc();
	static unsigned __stdcall ThreadEntry(void* arg);

	//���յ�������
	static void releaseInstance()
	{
		if (m_instance == NULL)
		{
			delete m_instance;
			m_instance = NULL;
		}
	}

	//��Ϣ������  Ŀǰ��û���õ�
	LRESULT OnSendStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);

//�Զ�����Ϣ�ṹ��
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

	//����ָ�� ָ����Ϣ����
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	//�ź�-��Ϣ���� ӳ���
	static std::map<UINT, MSGFUNC> m_mapFunc;//<�źţ�ִ�к���>
	CWatchDialog m_watchDlg;//��ضԻ���
	CRemoteClientDlg m_remoteDlg;//Զ�̿��ƶԻ���
	CStatusDlg m_StatusDlg;//����״̬���Ի���
	bool m_isClosed;//Զ�̼���Ƿ�ر�

	//�߳����
	HANDLE m_hThread;
	unsigned int m_nThreadID;
	HANDLE m_hThreadWatch;//����߳̾��

	//�����ļ���Զ��·��
	CString m_strRemote;
	//�����ļ��ı��ر���·��
	CString m_strLocal;
	
	static CClientController* m_instance;//��������  ��ʱû�õ�
};