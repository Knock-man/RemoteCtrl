﻿// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_nObjWidth = -1;
	m_nObjHeight = -1;
	m_isFull = false;

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
	ON_MESSAGE(WM_SEND_PACK_ACK,&CWatchDialog::OnSendPacketMessageAck)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPointToRemoteScreenPoint(CPoint& point,bool isScreen)
{
	
	if (!isScreen)ClientToScreen(&point);//客户区坐标转为屏幕坐标
	m_picture.ScreenToClient(&point);//屏幕坐标转为图片客户区坐标
	
	CRect clientRect;
	m_picture.GetWindowRect(clientRect);//获取图片控件的客户区域信息
	//本地坐标，到远程坐标
	int width0 = clientRect.Width();//客户区宽
	int height0 = clientRect.Height();//客户区高
	int width = m_nObjWidth, height = m_nObjHeight;//远程区宽高
	int x = point.x * width / width0;//客户区鼠标x对应远程区鼠标位置
	int y = point.y * height / height0;//客户区鼠标y对应远程区鼠标位置

	return CPoint(x,y);
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	
	//SetTimer(0, 10, NULL);//开启定时器
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

//定时器执行函数
void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//if (nIDEvent == 0)
	//{
	//	CClientController* pParent = CClientController::getInstance();
	//	if (m_isFull)
	//	{
	//		CRect rect;
	//		m_picture.GetWindowRect(rect);//m_picture 的屏幕尺寸和位置存储在 rect 对象中
	//		//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(),0,0,SRCCOPY);
	//		m_nObjWidth = m_image.GetWidth();//接收到截图的宽
	//		m_nObjHeight = m_image.GetHeight();//接收到截图的高
	//		m_image.StretchBlt(
	//			m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
	//		m_picture.InvalidateRect(NULL);
	//		m_image.Destroy();
	//		m_isFull = false;
	//	}
	//}
	CDialog::OnTimer(nIDEvent);
}
/*
typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//点击、移动、双击
	WORD nButton;//左键、右键、滚轮
	POINT ptXY;//坐标
}MOUSEEV, * PMOUSEEV;

*/


//左键双击
void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		//坐标转换
		CPoint remote = UserPointToRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 2;//双击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}

//左键按下
void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		//坐标转换
		CPoint remote = UserPointToRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//右键
		event.nAction = 2;//按下
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnLButtonDown(nFlags, point);
}

//左键弹起
void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		// 坐标转换
		CPoint remote = UserPointToRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 3;//弹起
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnLButtonUp(nFlags, point);
}

//右键双击
void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		//坐标转换
		CPoint remote = UserPointToRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 1;//双击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnRButtonDblClk(nFlags, point);
}

//右键按下
void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		//坐标转换
		CPoint remote = UserPointToRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 2;//按下
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnRButtonDown(nFlags, point);
}

//右键弹起
void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		//坐标转换
		CPoint remote = UserPointToRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;//右键
		event.nAction = 3;//弹起
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnRButtonUp(nFlags, point);
}

//鼠标移动
void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)//拿到的是客户端坐标
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		//坐标转换
		CPoint remote = UserPointToRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 8;//没有按键
		event.nAction = 0;//移动
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	}
	CDialog::OnMouseMove(nFlags, point);
}

//单击
void CWatchDialog::OnStnClickedWatch()
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint point;
		GetCursorPos(&point);//拿到屏幕坐标
		//坐标转换
		CPoint remote = UserPointToRemoteScreenPoint(point, true);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;//左键
		event.nAction = 0;//单击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	}
}
void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();
}

//点击加锁
void CWatchDialog::OnBnClickedBtnLock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),7);
}

//点击解锁
void CWatchDialog::OnBnClickedBtnUnlock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),8);
}


//接收ACK消息处理函数
LRESULT CWatchDialog::OnSendPacketMessageAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2))//错误处理
	{

	}
	else if (lParam == 1)//对方关闭了套接字
	{

	}
	else
	{
		CPacket* pPacket = (CPacket*)wParam;
		if (pPacket != NULL)
		{
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			switch (head.sCmd)
			{
			case 6://显示
			{
					CEdoyunTool::Bytes2Image(m_image, head.strData);
					CRect rect;
					m_picture.GetWindowRect(rect);//m_picture 的屏幕尺寸和位置存储在 rect 对象中
					//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(),0,0,SRCCOPY);
					m_nObjWidth = m_image.GetWidth();//接收到截图的宽
					m_nObjHeight = m_image.GetHeight();//接收到截图的高
					m_image.StretchBlt(
						m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
					m_picture.InvalidateRect(NULL);
					m_image.Destroy();
					m_isFull = false;
				break;
			}
			case 5://鼠标
			case 7://锁机
			case 8://解锁
			default:
				break;
			}
		}
	}

	return 0;
}