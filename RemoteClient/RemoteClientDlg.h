
// RemoteClientDlg.h: 头文件
//

#pragma once
#include"StatusDlg.h"
#include "ClientSocket.h"
#ifndef WM_SEND_PACK_ACK
#define WM_SEND_PACK_ACK (WM_USER+2)
#endif

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
	

	
private:

	
private:
	void DealCommand(WORD nCmd,const std::string& strData, LPARAM lParam);
	void InitUIData();
	CString Getpath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void Dump(BYTE* pData, size_t nSize);
	void LoadFileInfo();
	void LoadFileCurrent();
	void Str2Tree(const std::string& driver, CTreeCtrl& tree);
	void UpdateFIleInfo(const FILEINFO& finfo, HTREEITEM hParent);
	void UpdateDownloadFile(const std::string& strData, FILE* pFile);
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
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditPort();
	afx_msg LRESULT OnSendPacketAck(WPARAM wParam, LPARAM lParam);
};
