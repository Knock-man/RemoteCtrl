/*
控制层
负责发送接收数据 实现业务具体逻辑

监控实现：
	开启线程 隔一段频率向服务器发送截图请求(图片缓存为空时)
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
	//获取单例实例接口
	static CClientController* getInstance()
	{
		static CClientController instance;
		return &instance;
	}

	CClientController();
	~CClientController() {};

	//初始化操作 创建状态对话框
	int InitController();

	//设置RemoteClientDlg为主窗口，且为模态对话框
	int Invoke(CWnd*& m_pMainWnd);

	//发送数据接口 供视图层使用 调用网络层发送数据接口  
	//返回值是发送状态
	bool SendCommandPacket(HWND hWnd,//数据包收到后需要应答的窗口
		int nCmd, //控制命令
		bool bAutoClose = true,//长短连接 
		BYTE* pData = NULL, //数据(未打包)
		size_t nLength = 0,//数据长度
		WPARAM wParam = 0)//附加参数
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		bool ret = pClient->SendPacketMessage(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
		return ret;
	}

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

//自定义消息结构体
private:
	CWatchDialog m_watchDlg;//监控对话框
	CRemoteClientDlg m_remoteDlg;//远程控制对话框
	CStatusDlg m_StatusDlg;//下载状态栏对话框
	bool m_isClosed;//远程监控是否关闭

	//线程相关
	unsigned int m_nThreadID;
	HANDLE m_hThreadWatch;//监控线程句柄

	//下载文件的远程路径
	CString m_strRemote;
	//下载文件的本地保存路径
	CString m_strLocal;
};