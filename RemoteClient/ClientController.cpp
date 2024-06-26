#include "pch.h"
#include "ClientController.h"


//构造函数
CClientController::CClientController() :m_StatusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg)//设置父窗口
{
    m_nThreadID = -1;
    m_hThreadWatch = INVALID_HANDLE_VALUE;

    m_isClosed = true;
}

//初始化函数  创建状态对话框
int CClientController::InitController()
{
    m_StatusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
    return 0;
}

//启动模态对话框
int CClientController::Invoke(CWnd*& m_pMainWnd)
{
    m_pMainWnd = &m_remoteDlg;//设置主窗口
    return m_remoteDlg.DoModal();//启动主窗口为模态对话框
}

void  CClientController::StartWatchScreen()
{
    m_isClosed = false;
    m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry, 0, this);
    m_watchDlg.DoModal();//调用模态对话框，阻塞直到对话框被关闭
    m_isClosed = true;//会让监控线程自动走向死亡 监控线程while(true)
    //阻塞当前线程，等待m_hThreadWatch线程完成工作，最多等500毫秒
    WaitForSingleObject(m_hThreadWatch, 500);
}

//监控线程入口函数
void CClientController::threadWatchScreenEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadWatchScreen();
    _endthread();//回收线程
}

//监控线程函数
void CClientController::threadWatchScreen()
{
    Sleep(50);
    ULONGLONG nTick = GetTickCount64();
    while (!m_isClosed)
    {
        if (m_watchDlg.isFull() == false)//更新数据到缓存
        {
            if (GetTickCount64() - nTick < 200)//控制发送请求频率
            {
                Sleep(200 - DWORD(GetTickCount64() - nTick));
            }
            nTick = GetTickCount64();

            //发送监控请求
            int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6,true, NULL, 0);
            if (ret == 1)
            {
               // TRACE("成功发送请求图片命令!\r\n");
            }
            else
            {
                TRACE("获取图片失败!\r\n");
            }
        }
        Sleep(1);
    }
}

int CClientController::DownFile(CString strPath)//strPath为远程下载文件路径
{
    //弹出下载对话框
    CFileDialog dlg(FALSE, NULL,
        strPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY
        , NULL, &m_remoteDlg);
    if (dlg.DoModal() == IDOK)//点击确认下载
    {
        m_strRemote = strPath;//远程文件路径名
        m_strLocal = dlg.GetPathName();//本地保存文件的文件名
        //打开保存文件
        FILE* pFile = fopen(m_strLocal, "wb+");
        if (pFile == NULL)//打开失败
        {
            AfxMessageBox(TEXT("本地没有权限保存该文件，或者文件无法创建!!!"));
            return -1;
        }

        //发送下载请求
        SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);

        m_remoteDlg.BeginWaitCursor();//开启沙漏
        m_StatusDlg.m_info.SetWindowText(TEXT("命令正在执行中"));
        m_StatusDlg.ShowWindow(SW_SHOW);//显示下载状态对话框
        m_StatusDlg.CenterWindow(&m_remoteDlg);//对话框居中
        m_StatusDlg.SetActiveWindow();//设置为顶层窗口
    }
    return 0;


}
