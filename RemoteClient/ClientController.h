#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "CEdoyunTool.h"
#include "resource.h"
#include "StatusDlg.h"
#include <map>

//#define WM_SEND_DATA (WM_USER+2)//发送数据
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
	//初始化操作 开启接收消息线程
	int InitController();

	//启动模态对话框
	int Invoke(CWnd*& m_pMainWnd);

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
	//关闭网络连接
	void CloseSocket()
	{
		CClientSocket::getInstance()->CloseSocket();
	}

	//发送数据  返回值是状态
	bool SendCommandPacket(HWND hWnd,//数据包收到后需要应答的窗口
		int nCmd, //控制命令
		bool bAutoClose=true,//长短连接 
		BYTE* pData=NULL, //数据(未打包)
		size_t nLength=0,//数据长度
		WPARAM wParam = 0)//附加参数
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		bool ret = pClient->SendPacket(hWnd,CPacket(nCmd, pData, nLength),bAutoClose, wParam);
		return ret;
	}

	//获得图片  网络端接收到的数据 转为图片格式返回
	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		return CEdoyunTool::Bytes2Image(image, pClient->GetPacket().strData);
	}

	//下载文件 弹出保存文件对话框  发出下载文件请求 弹出下载状态对话框
	int DownFile(CString strPath);//strPath为远程下载文件路径
	
	//获取网络当前缓存数据包
	CPacket GetPacket() const
	{
		return CClientSocket::getInstance()->GetPacket();
	}

	//下载结束 回收对话框 弹出下载完成提示
	void DownloadEnd()
	{
		m_StatusDlg.ShowWindow(SW_HIDE);//回收下载状态对话框
		m_remoteDlg.EndWaitCursor();//回收沙漏
		m_remoteDlg.MessageBox(TEXT("下载完成!!"), TEXT("完成"));
	}

	//开启监控线程 打开监视对话框 阻塞等待
	void StartWatchScreen();
	
protected:
	//监控线程 以一定频率发送监控请求
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

	//回收单例对象
	static void releaseInstance()
	{
		if (m_instance == NULL)
		{
			delete m_instance;
			m_instance = NULL;
		}
	}

	//消息处理函数  目前还没有用到
	LRESULT OnSendStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);

//自定义消息结构体
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

	//函数指针 指向消息函数
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	//信号-消息函数 映射表
	static std::map<UINT, MSGFUNC> m_mapFunc;//<信号，执行函数>
	CWatchDialog m_watchDlg;//监控对话框
	CRemoteClientDlg m_remoteDlg;//远程控制对话框
	CStatusDlg m_StatusDlg;//下载状态栏对话框
	bool m_isClosed;//远程监控是否关闭

	//线程相关
	HANDLE m_hThread;
	unsigned int m_nThreadID;
	HANDLE m_hThreadWatch;//监控线程句柄

	//下载文件的远程路径
	CString m_strRemote;
	//下载文件的本地保存路径
	CString m_strLocal;
	
	static CClientController* m_instance;//单例对象  暂时没用到
};