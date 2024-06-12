#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "CEdoyunTool.h"
#include "resource.h"
#include "StatusDlg.h"
#include <map>

#define WM_SEND_PACK (WM_USER+1)//发送包数据
#define WM_SEND_DATA (WM_USER+2)//发送数据
#define WM_SHOW_STATUS (WM_USER+3)//展示状态
#define WM_SHOW_WATCH (WM_USER+4)//远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000)//自定义消息处理

class CClientController
{
public:
	//获取单例实例接口
	static CClientController* getInstance()
	{
		static CClientController instance;
		return &instance;
	}
	//初始化操作
	int InitController();

	//启动
	int Invoke(CWnd*& m_pMainWnd);

	//发送消息
	LRESULT SendMessage(MSG msg);

	//更新网络服务器地址
	void UpdateAddress(int nIP, int nPort)
	{
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}
	//接收数据
	int DealCommand()
	{
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket()
	{
		CClientSocket::getInstance()->CloseSocket();
	}

	//发送数据
	int SendCommandPacket(int nCmd, bool bAutoClose=true, BYTE* pData=NULL, size_t nLength=0, std::list<CPacket>* plstPacks=NULL)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		//if (pClient->InitSocket() == false)return false;
		HANDLE hEVent = CreateEvent(NULL, TRUE, FALSE, NULL);
		//不应该直接发送，而是投入队列
		std::list<CPacket> lstPacks;//临时应答结果包
		if (plstPacks == NULL)
		{
			plstPacks = &lstPacks;//不需要传回返回值
		}
		pClient->SendPacket(CPacket(nCmd, pData, nLength, hEVent),*plstPacks,bAutoClose);
		CloseHandle(hEVent);//回收事件句柄防止资源耗尽
		if (plstPacks->size() > 0)
		{	
			return plstPacks->front().sCmd;
		}
		return -1;
	}

	//获得图片
	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		return CEdoyunTool::Bytes2Image(image, pClient->GetPacket().strData);
	}
	int DownFile(CString strPath)//strPath为远程下载文件路径
	{
		//弹出下载对话框
		CFileDialog dlg(FALSE, NULL,
			strPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY
			, NULL, &m_remoteDlg);
		if (dlg.DoModal() == IDOK)//点击确认下载
		{
			m_strRemote = strPath;//远程文件路径名
			m_strLocal = dlg.GetPathName();//本地保存文件的文件名
			//开启线程下载
			m_hThreadDown = (HANDLE)_beginthread(&CClientController::threadDownloadFileEntry, 0, this);//开启线程
			if (WaitForSingleObject(m_hThreadDown, 0) != WAIT_TIMEOUT)//线程开启失败
			{
				return -1;
			}
			m_remoteDlg.BeginWaitCursor();//开启沙漏
			m_StatusDlg.m_info.SetWindowText(TEXT("命令正在执行中"));
			m_StatusDlg.ShowWindow(SW_SHOW);//显示对话框
			m_StatusDlg.CenterWindow(&m_remoteDlg);//对话框居中
			m_StatusDlg.SetActiveWindow();//设置为顶层窗口
		}
		return 0;
		
		
	}

	void StartWatchScreen()
	{
		m_isClosed = false;
		//m_watchDlg.SetParent(&m_remoteDlg);
		m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry, 0, this);
		m_watchDlg.DoModal();//调用模态对话框并在完成后返回。
		m_isClosed = true;
		//阻塞当前线程，等待m_hThreadWatch线程结束，最多等500毫秒
		WaitForSingleObject(m_hThreadWatch, 500);
	}
protected:
	//下载文件线程
	void threadDownloadFile();
	static void threadDownloadFileEntry(void* arg);

	//监控线程
	void threadWatchScreen();
	static void threadWatchScreenEntry(void* arg);

	CClientController();
	~CClientController()
	{
		//回收线程
		WaitForSingleObject(m_hThread, 100);
	}

	//线程函数
	void threadFunc();
	static unsigned __stdcall ThreadEntry(void* arg);

	//删除对象
	static void releaseInstance()
	{
		if (m_instance == NULL)
		{
			delete m_instance;
			m_instance = NULL;
		}

	}

	//消息处理函数
	//LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	//LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);
private:
	typedef struct MsgInfo
	{
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m)//有参构造
		{
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));

		}

		MsgInfo(const MsgInfo& m)//拷贝构造
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
	static std::map<UINT, MSGFUNC> m_mapFunc;//<信号，执行函数>
	CWatchDialog m_watchDlg;//监控对话框
	CRemoteClientDlg m_remoteDlg;//远程控制对话框
	CStatusDlg m_StatusDlg;//状态栏对话框
	bool m_isClosed;//监视是否关闭

	//线程相关
	HANDLE m_hThread;
	unsigned int m_nThreadID;
	HANDLE m_hThreadDown;
	HANDLE m_hThreadWatch;

	//下载文件的远程路径
	CString m_strRemote;
	//下载文件的本地保存路径
	CString m_strLocal;
	
	static CClientController* m_instance;
};