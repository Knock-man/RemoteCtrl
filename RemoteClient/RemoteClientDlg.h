
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

private:
	int SendCommandPacket(int nCmd, bool bAutoClose=true, BYTE* pData=nullptr, size_t nLength=0);
	CString Getpath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void Dump(BYTE* pData, size_t nSize);
	void LoadFileInfo();
	void LoadFileCurrent();
	static void threadEntryForDownFile(void* arg);
	void threadDownFile();


// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD IP_Address;
	CString IP_PORT;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteFile();
	afx_msg void OnRunfile();
	afx_msg void OnDownloadFile();
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);
};
