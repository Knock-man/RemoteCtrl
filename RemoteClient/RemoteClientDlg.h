
// RemoteClientDlg.h: 头文件
//

#pragma once
#include"StatusDlg.h"
#include "ClientSocket.h"
#define WM_SEND_PACKET (WM_USER+1) //发送数据包消息

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


public:
	bool isFull()const
	{
		return m_isFull;
	}
	CImage& GetImage() {
		return m_image;
	}
	void SetImageSatus(bool isFull = false)
	{
		m_isFull = isFull;
	}
private:
	CImage m_image;//缓存
	bool m_isFull;//缓存是否有数据 true表示有缓存数据 false表示没有缓存数据
	bool m_isClosed;//监视是否关闭
private:
	int SendCommandPacket(int nCmd, bool bAutoClose=true, BYTE* pData=nullptr, size_t nLength=0);
	CString Getpath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void Dump(BYTE* pData, size_t nSize);
	void LoadFileInfo();
	void LoadFileCurrent();
	void threadDownFile();

	//线程函数
	static void threadEntryForDownFile(void* arg);//下载文件线程函数 调用 threadDownFile()
	static void threadEntryForWatch(void*);
	void threadWatchData();


// 实现
protected:
	DECLARE_MESSAGE_MAP()//声明宏
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;//下载状态对话框
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD IP_Address;
	CString IP_PORT;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);//双击树控件
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);//右键单击列表控件
	afx_msg void OnDeleteFile();
	afx_msg void OnRunfile();
	afx_msg void OnDownloadFile();
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
};
