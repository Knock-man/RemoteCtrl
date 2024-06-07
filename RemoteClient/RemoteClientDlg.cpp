
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


void CRemoteClientDlg::Dump(BYTE* pData, size_t nSize)
{
	std::string strOut;
	for (size_t i = 0; i < nSize; i++)
	{
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0))strOut += "\n";
		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}





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
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();//从对话框的控件中检索数据到成员变量
	IP_PORT = TEXT("9527");//端口	//默认窗口的地址
	IP_Address = 0xC0A83484;//ip
	//刷新IP PORT
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(IP_Address, atoi((LPCTSTR)IP_PORT));
	UpdateData(FALSE);//将数据从成员变量推送到对话框的控件中
	m_dlgStatus.Create(IDD_DLG_STATUS,this);//创建下载状态对话框  IDD_DLG_STATUS对话框ID
	m_dlgStatus.ShowWindow(SW_HIDE);//隐藏下载状态对话框
	m_isFull = false;//缓冲区是否有数据

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


//点击测试按钮
void CRemoteClientDlg::OnBnClickedBtnTest()
{
	CClientController::getInstance()->SendCommandPacket(1981);
}

//点击查看文件信息按钮
void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	//发送查看磁盘分区请求
	int ret = CClientController::getInstance()->SendCommandPacket(1);
	if (ret == -1)
	{
		AfxMessageBox(TEXT("命令处理失败"));
		return;
	}
	//获取磁盘分区名称
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->GetPacket().strData;//拿到磁盘盘符名称

	//磁盘分区插入到树中
	std::string dr;
	m_Tree.DeleteAllItems();//清空树
	//"C,D"
	for (size_t i = 0; i < drivers.size()+1; i++)
	{
		if (drivers[i] == ','||i== (drivers.size()))
		{
			dr += ":";
			HTREEITEM hTmp = m_Tree.InsertItem(dr.c_str(),TVI_ROOT,TVI_LAST);//树中插入一项
			m_Tree.InsertItem("", hTmp, TVI_LAST);// // 在新插入的项下插入一个空项
			dr.clear();
			continue;
			//TVI_LAST：表示将新项插入到其同级项的末尾。
		}
		dr += drivers[i];
		//TRACE("%s\r\n", dr.c_str());
	}
	
}

//删除文件之后重新加载文件列表
void CRemoteClientDlg::LoadFileCurrent()
{
	//获取文件路径
	HTREEITEM hTree = m_Tree.GetSelectedItem();//获取树控件中被选中的项
	CString strPath = Getpath(hTree);//获得完整文件路径	 D:\\xbj\shixi
	//TRACE("path=%s\r\n", strPath);

	//清空列表
	m_List.DeleteAllItems();

	
	//发送查看目录下所有文件请求
	int nCmd =CClientController::getInstance()->SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	

	//接收文件信息 PFILEINFO为文件结构体
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	int cout = 0;
	while (pInfo->HasNext) {//是否为最后一个文件
		//TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (!pInfo->IsDirectory)//是文件
		{
			//List中显示文件
			m_List.InsertItem(0, pInfo->szFileName);
		}

		int cmd = CClientController::getInstance()->DealCommand();//记录接收
		if (cmd < 0)break;
		cout++;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	};
	TRACE("cout= %d\r\n", cout);
	CClientController::getInstance()->CloseSocket();
}
//加载文件
void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;//鼠标指针
	GetCursorPos(&ptMouse);//获取鼠标指针坐标
	m_Tree.ScreenToClient(&ptMouse);//屏幕坐标转为树控件中的坐标

	//获得鼠标点击坐标对应的树节点句柄
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)return;
	if (m_Tree.GetChildItem(hTreeSelected) == NULL)return;//没有子节点是文件，没有子目录

	//先删除孩子节点防止重复点击
	DeleteTreeChildrenItem(hTreeSelected);
	//清空文件列表
	m_List.DeleteAllItems();

	//获得完整的路径信息 D:\C\xbj\shixi
	CString strPath = Getpath(hTreeSelected);
	TRACE("path=%s\r\n", strPath);

	//TRACE("发送路径:[%s]",strPath);
	//发送查看目录下所有文件请求
	int nCmd=CClientController::getInstance()->SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());

	//接收文件信息 PFILEINFO为文件结构体
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	int cout = 0;
	while (pInfo->HasNext) {
		//TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory)//是目录
		{
			if (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..")
			{//防止死递归，继续接收下一个文件
				int cmd = CClientController::getInstance()->DealCommand();
				//TRACE("ack:%d/r/n", cmd);
				if (cmd < 0)break;
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			//树种显示目录
			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			m_Tree.InsertItem("", hTemp, TVI_LAST);
		}
		else
		{
			//List中显示文件
			m_List.InsertItem(0,pInfo->szFileName);
		}
		
		int cmd = CClientController::getInstance()->DealCommand();//继续接收
		cout++;
		//TRACE("ack:%d/r/n", cmd);
		if (cmd < 0)break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	};
	TRACE("cout= %d\r\n", cout);

	CClientController::getInstance()->CloseSocket();
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

//删除当前节点的所有孩子节点
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

	int ret = CClientController::getInstance()->SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox(TEXT("删除文件命令执行失败!!!"));
	}
	LoadFileCurrent();
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
	int ret = CClientController::getInstance()->SendCommandPacket(3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
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
	
	//Sleep(50);
}

//远程控制
void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWatchScreen();
}


//wParam:控制命令，长短连接
LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	int ret = 0;
	int cmd = wParam >> 1;
	switch (cmd)
	{
	case 4://接收文件操作
	{
		CString strFile = (LPCSTR)lParam;
		//wParam共32个bit  前31个bit存储cmd 最后一个比特存储true/false
		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
		break;
	}
	case 5://鼠标操作
	{
		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)lParam, sizeof(MOUSEEV));
		break;
	}
	case 6://接收屏幕数据操作
	case 7://锁机
	case 8://解锁
	{
		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1);
		break;
	}
	default:
		ret = -1;
	}
	return ret;
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
