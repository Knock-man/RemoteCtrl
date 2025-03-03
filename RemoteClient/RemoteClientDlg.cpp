﻿
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientController.h"
#include "ClientSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "CWatchDialog.h"


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	//DECLARE_MESSAGE_MAP()
public:
	
	DECLARE_MESSAGE_MAP()
	
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


// CRemoteClientDlg 对话框

CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, IP_Address(0)
	, IP_PORT(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, IP_Address);
	DDX_Text(pDX, IDC_EDIT_PORT, IP_PORT);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUNFILE, &CRemoteClientDlg::OnRunfile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPacketMessageAck)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}
	// TODO: 在此添加额外的初始化代码
	InitUIData();//初始化主对话框UI界面（自己写的）
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); //用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//点击测试连接按钮
void CRemoteClientDlg::OnBnClickedBtnTest()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),1981);
}

//点击查看文件信息按钮
void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	//发送查看磁盘分区请求
	bool ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),1,true,NULL,0);
	if (ret == 0)
	{
		AfxMessageBox(TEXT("命令处理失败"));
		return;
	}
	
}
//加载当前被选中句柄的全部文件(使用在删除文件之后重新加载所有文件到显示列表中)
void CRemoteClientDlg::LoadFileCurrent()
{
	//获取文件路径
	HTREEITEM hTree = m_Tree.GetSelectedItem();//获取树控件中被选中的项
	CString strPath = Getpath(hTree);//获得完整文件路径	 D:\\xbj\shixi
	//TRACE("path=%s\r\n", strPath);
	//清空列表
	m_List.DeleteAllItems();

	//发送查看目录下所有文件请求
	int nCmd =CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	
	//接收文件信息 PFILEINFO为文件结构体
	PFILEINFO pInfo = (PFILEINFO)CClientController::getInstance()->GetPacket().strData.c_str();
	while (pInfo->HasNext) {//是否为最后一个文件
		if (!pInfo->IsDirectory)//是文件
		{
			//List中显示文件
			m_List.InsertItem(0, pInfo->szFileName);
		}

		int cmd = CClientController::getInstance()->DealCommand();//继续接收
		if (cmd < 0)break;
		pInfo = (PFILEINFO)CClientController::getInstance()->GetPacket().strData.c_str();
	};
}

//磁盘分区插入到树中
void CRemoteClientDlg::Str2Tree(const std::string& drivers, CTreeCtrl& tree)
{
	std::string dr;
	tree.DeleteAllItems();//清空树
	//"C,D"
	for (size_t i = 0; i < drivers.size() + 1; i++)
	{
		if (drivers[i] == ',' || i == (drivers.size()))
		{
			dr += ":";
			HTREEITEM hTmp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);//树中插入一项
			tree.InsertItem("", hTmp, TVI_LAST);// // 在新插入的项下插入一个空项
			dr.clear();
			continue;
			//TVI_LAST：表示将新项插入到其同级项的末尾。
		}
		dr += drivers[i];
		//TRACE("%s\r\n", dr.c_str());
	}
}
void CRemoteClientDlg::UpdateFIleInfo(const FILEINFO& finfo,HTREEITEM hParent)
{
	if (finfo.HasNext == FALSE)return;//没有后续文件，结束
	if (finfo.IsDirectory)//是目录
	{
		if (CString(finfo.szFileName) == "." || CString(finfo.szFileName) == "..")
		{//防止死递归,忽略
			return;
		}
		//树中显示目录
		HTREEITEM hTemp = m_Tree.InsertItem(finfo.szFileName, hParent, TVI_LAST);
		m_Tree.InsertItem("", hTemp, TVI_LAST);
		m_Tree.Expand(hParent, TVE_EXPAND);//展开选项
	}
	else
	{
		//List列表中显示文件
		m_List.InsertItem(0, finfo.szFileName);
	}
}
void CRemoteClientDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile)
{
	static LONGLONG length = 0, index = 0;//length文件长度   index 已经写入长度
	//length index为静态变量因为文件可以分为多个包发送
	if (length == 0)//文件长度为0 当收到第一个包 文件长度包时
	{
		length = *(long long*)strData.c_str();
		if (length == 0 && strData.size() != 0 )
		{
			AfxMessageBox("文件长度为零或者无法读取文件!!!");
			CClientController::getInstance()->DownloadEnd();//结束下载
			return;
		}
	}
	else if (length > 0 && (index >= length))//文件全部写入完成
	{
		fclose(pFile);//关闭文件
		length = 0;
		index = 0;
		CClientController::getInstance()->DownloadEnd();//结束下载 弹出结束下载弹窗，回收资源
	}
	else//写入文件
	{
		fwrite(strData.c_str(), 1, strData.size(), pFile);
		index += strData.size();
		if (index >= length)//文件全部下载结束
		{
			fclose(pFile);//关闭文件
			length = 0;
			index = 0;
			CClientController::getInstance()->DownloadEnd();//结束下载 弹出结束下载弹窗，回收资源
		}
	}
}
//请求鼠标当前鼠标指向的树节点下面的所有文件和目录
void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;//鼠标指针
	GetCursorPos(&ptMouse);//获取鼠标指针坐标
	m_Tree.ScreenToClient(&ptMouse);//屏幕坐标转为树控件中的坐标

	//获得鼠标点击坐标对应的树节点句柄
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)return;

	//先删除所有孩子节点防止重复点击，导致文件重复
	DeleteTreeChildrenItem(hTreeSelected);
	//清空文件列表
	m_List.DeleteAllItems();

	//获得完整的路径信息 D:\C\xbj\shixi
	CString strPath = Getpath(hTreeSelected);
	//TRACE("path=%s\r\n", strPath);

	//TRACE("发送路径:[%s]",strPath);
	//发送查看目录下所有文件请求
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength(), (WPARAM)hTreeSelected);
}

//不同nCmd调用不同结果处理函数 strData:结果数据 lParam：附带参数(树节点句柄/文件指针)
void CRemoteClientDlg::DealCommand(WORD nCmd,const std::string& strData, LPARAM lParam)
{
	switch (nCmd)
	{
	case 1://获取驱动信息(盘符)
		Str2Tree(strData, m_Tree);//磁盘分区插入到树中
		break;
	case 2://目录文件信息
		UpdateFIleInfo(*(PFILEINFO)strData.c_str(), (HTREEITEM)lParam);//插入树和列表中  lParam：树节点句柄
		break;
	case 3://运行文件
		MessageBox("打开文件完成!", "操作完成", MB_ICONINFORMATION);
		break;
	case 4://下载文件
		UpdateDownloadFile(strData, (FILE*)lParam);//lParam:文件指针
		break;
	case 9://删除文件
		MessageBox("删除文件完成!", "操作完成", MB_ICONINFORMATION);
		break;
	case 1981://测试连接
		MessageBox("连接测试成功!","连接成功",MB_ICONINFORMATION);
		break;
	default:
		TRACE("unknow data received!%d\r\n", nCmd);
		break;
	}
}

void CRemoteClientDlg::InitUIData()
{
	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	UpdateData();//从对话框的控件中检索数据到成员变量
	//默认显示的端口和IP
	IP_PORT = TEXT("9527");//端口	//默认窗口的地址
	IP_Address = 0xC0A83487;//ip

	//刷新IP PORT
	CClientController* pController = CClientController::getInstance();//控制层单例对象
	pController->UpdateAddress(IP_Address, atoi((LPCTSTR)IP_PORT));//通过控制层设置网络层通信的IP 和 PORT
	UpdateData(FALSE);//将数据从成员变量推送到对话框的控件中

	//非模态创建
	m_dlgStatus.Create(IDD_DLG_STATUS, this);//创建下载状态对话框  IDD_DLG_STATUS为下载对话框ID
	m_dlgStatus.ShowWindow(SW_HIDE);//隐藏下载状态对话框
}

//获得hTree节点 完整路径信息 D:\C\xbj\shixi
CString CRemoteClientDlg::Getpath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	do{
		strTmp = m_Tree.GetItemText(hTree);//获得树节点当前的文本
		strRet  = strTmp + '\\' + strRet;//拼接
		hTree = m_Tree.GetParentItem(hTree);//得到父节点句柄
	} while (hTree != NULL);//找到了根节点
	return strRet;
}

//删除hTree节点的所有孩子节点
void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL)m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);
}

//树控件收到双击消息
void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();//显示目录和文件
}

//树控件收到单击消息
void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


//右键单击列表控件
void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	//获取选中列表项句柄
	CPoint ptMouse,ptList;
	GetCursorPos(&ptMouse);//获取鼠标指针坐标
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);//转为列表控件坐标
	int ListSelected = m_List.HitTest(ptList);//选中列表项句柄
	if (ListSelected < 0)return;

	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);//加载菜单控件
	CMenu* pPupup = menu.GetSubMenu(0);//取第一个子菜单
	if (pPupup != NULL)
	{
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);//弹出菜单
		/*
		TPM_LEFTALIGN 是一个标志，表示菜单项应该左对齐显示。
		TPM_RIGHTBUTTON 是一个标志，表示菜单是由鼠标右键触发的。这通常用于确保菜单在显示时不会与可能的上下文菜单冲突。
		ptMouse.x 和 ptMouse.y 是你想要显示菜单的屏幕坐标。
		this 指针指向当前窗口或对话框对象，它是菜单的父窗口。
		*/
	}
}

//删除文件	点击删除文件菜单被触发
void CRemoteClientDlg::OnDeleteFile()
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hselected = m_Tree.GetSelectedItem();//树中被选择的路径
	CString strPath = Getpath(hselected);//完整目录

	int nSelected = m_List.GetSelectionMark();//列表中选中标记
	CString strFile = m_List.GetItemText(nSelected, 0);//拿到文件名

	strFile = strPath + strFile;//路径

	//发送删除文件请求

	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox(TEXT("删除文件命令执行失败!!!"));
	}

	LoadFileCurrent();//删除文件之后需要重新加载当前目录下的所有文件
}

//运行文件	点击打开菜单触发
void CRemoteClientDlg::OnRunfile()
{
	HTREEITEM hselected = m_Tree.GetSelectedItem();//树中被选择的路径
	CString strPath = Getpath(hselected);

	int nSelected = m_List.GetSelectionMark();//列表中选中标记
	CString strFile = m_List.GetItemText(nSelected, 0);//拿到文件名

	strFile = strPath + strFile;//路径

	//发送打开文件请求
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox(TEXT("打开文件命令执行失败!!!"));
	}
}

//下载文件 点击下载文件菜单执行
void CRemoteClientDlg::OnDownloadFile()
{
	//待下载文件名
	int nListSelected = m_List.GetSelectionMark();//选中标记
	CString strFile = m_List.GetItemText(nListSelected, 0);

	//待下载的路径名
	HTREEITEM hselected = m_Tree.GetSelectedItem();//树中被选择的路径
	strFile = Getpath(hselected) + strFile;//完整路径名
	int ret = CClientController::getInstance()->DownFile(strFile);
	if (ret != 0)
	{
		MessageBox(TEXT("下载失败！"));
		TRACE("下载失败 ret=%d\r\n", ret);
	}
	
}

//远程控制
void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWatchScreen();
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	
END_MESSAGE_MAP()


void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(IP_Address, atoi((LPCTSTR)IP_PORT));

}


void CRemoteClientDlg::OnEnChangeEditPort()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(IP_Address, atoi((LPCTSTR)IP_PORT));
}

LRESULT CRemoteClientDlg::OnSendPacketMessageAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2))//错误处理
	{
		TRACE("socket is error %d\r\n", lParam);
	}
	else if (lParam == 1)//对方关闭了套接字
	{
		TRACE("socket is closed!\r\n");
	}
	else//正常处理
	{
		if (wParam != NULL)
		{
			CPacket pack = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			DealCommand(pack.sCmd, pack.strData, lParam);
		}
	}
	return 0;
}
