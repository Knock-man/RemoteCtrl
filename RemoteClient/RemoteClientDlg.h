
// RemoteClientDlg.h: 头文件
//

#pragma once
#include"StatusDlg.h"
#include "ClientSocket.h"
#ifndef WM_SEND_PACK_ACK
#define WM_SEND_PACK_ACK (WM_USER+2)
#endif

// CRemoteClientDlg 主对话框
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
	//不同nCmd调用不同结果处理函数 strData:结果数据 lParam：附带参数(树节点句柄/文件指针)
	void DealCommand(WORD nCmd,const std::string& strData, LPARAM lParam);
	//初始化主对话框UI界面
	void InitUIData();
	//获取树节点 hTree  完整的绝对路径 D:\C\xbj\shixi
	CString Getpath(HTREEITEM hTree);
	//删除hTree节点的所有孩子节点
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	//将二进制转换为16进制输出
	void Dump(BYTE* pData, size_t nSize);
	//请求鼠标当前鼠标指向的树节点下面的所有文件和目录
	void LoadFileInfo();
	//加载当前被选中句柄的全部文件(使用在删除文件之后重新加载所有文件到显示列表中)
	void LoadFileCurrent();
	//drivers[]={'C','D'}  分区信息插入到树(tree)中
	void Str2Tree(const std::string& drivers, CTreeCtrl& tree);
	//文件名finfo.szFileName，插到树节点hParent下，或者列表list中(使用双击树节点展开文件时)
	void UpdateFIleInfo(const FILEINFO& finfo, HTREEITEM hParent);
	//将内容strData 写入pFile文件中
	void UpdateDownloadFile(const std::string& strData, FILE* pFile);
// 实现
protected:
	DECLARE_MESSAGE_MAP()//声明消息宏
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;//下载状态对话框(非模态)
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	
public:
	//IP 端口
	DWORD IP_Address;
	CString IP_PORT;
	// 显示文件
	CListCtrl m_List;
	CTreeCtrl m_Tree;

public:
	afx_msg void OnBnClickedBtnTest();//点击测试连接按钮 发送测试连接请求  cmd=1
	afx_msg void OnBnClickedBtnFileinfo();//点击查看文件信息按钮 发送查看所有磁盘分区请求  cmd=1981
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);//双击树控件  发送查看选中节点下所有文件目录请求 cmd=2
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);//单击树控件	   发送查看选中节点下所有文件目录请求 cmd=2
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);//右键单击列表  弹出子菜单(下载文件 删除文件 运行文件)
	afx_msg void OnDeleteFile();//点击删除文件菜单项 发送删除选中文件请求 cmd=9
	afx_msg void OnRunfile();//点击运行文件菜单项 发送运行选中文件请求 cmd=3
	afx_msg void OnDownloadFile();//点击下载文件菜单项 发送下载选中文件请求 cmd=4
	afx_msg void OnBnClickedBtnStartWatch();//点击监控按钮，发送监控请求 cmd=6
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);//IP输入框改变 更新IP PORT
	afx_msg void OnEnChangeEditPort();//PORT输入框改变 更新IP PORT
	afx_msg LRESULT OnSendPacketAck(WPARAM wParam, LPARAM lParam);//接收结果消息 wParam:数据包  lParam:附带参数（(树节点句柄/文件指针)）
};
