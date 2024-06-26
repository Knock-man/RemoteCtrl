#pragma once
#include "afxdialogex.h"

#ifndef WM_SEND_PACK_ACK
#define WM_SEND_PACK_ACK (WM_USER+2)
#endif

// CWatchDialog 对话框

class CWatchDialog : public CDialog
{
	DECLARE_DYNAMIC(CWatchDialog)

public:
	CWatchDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDialog();
	virtual void OnOK();


// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_WATCH };
#endif
public:
	int m_nObjWidth;//截图宽
	int m_nObjHeight;//截图高
	CImage m_image;//图片缓冲区
protected:
	bool m_isFull;//缓存是否有数据 true表示有缓存数据 false表示没有缓存数据

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	//获得图片缓冲区
	CImage& GetImage() {
		return m_image;
	}
	//图片缓冲区是否有数据
	bool isFull()const
	{
		return m_isFull;
	}
	//设置图片缓冲区状态
	void SetImageSatus(bool isFull = false)
	{
		m_isFull = isFull;
	}

	//本地坐标转为远程屏幕坐标
	CPoint UserPointToRemoteScreenPoint(CPoint& point,bool isScreen = false);

	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);//定时取图片缓冲区数据展示 已被弃用
	CStatic m_picture;
	////接收结果消息 wParam:数据包  lParam:附带参数（(树节点句柄/文件指针)）
	afx_msg LRESULT OnSendPacketMessageAck(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);//左键双击
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);//左键按下
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);//左键弹起
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);//右键双击
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);//右键按下
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);//右键弹起
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);//鼠标移动
	afx_msg void OnStnClickedWatch();//单击
	afx_msg void OnBnClickedBtnLock();//加锁
	afx_msg void OnBnClickedBtnUnlock();//解锁
};
