#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
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
		static CClientController server;
		return &server;
	}
	//初始化操作
	int InitController();

	//启动
	int Invoke(CWnd*& m_pMainWnd);

	//发送消息
	LRESULT SendMessage(MSG msg);
protected:
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
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
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
	HANDLE m_hThread;
	unsigned int m_nThreadID;
	static CClientController* m_instance;
};