#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;

//构造函数
CClientController::CClientController() :m_StatusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg)//设置父窗口
{
    //if (CClientController::getInstance() == NULL)
    //{
        //CClientController::m_instance = CClientController::getInstance();

        //注册消息map<信号,执行函数>
    struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] =
    {
        //{WM_SEND_PACK,&CClientController::OnSendPack},
        //{WM_SEND_DATA,&CClientController::OnSendData},
        {WM_SHOW_STATUS,&CClientController::OnSendStatus},
        {WM_SHOW_WATCH,&CClientController::OnSendWatcher},
        {(UINT)-1,NULL}
    };
    for (int i = 0; MsgFuncs[i].func != NULL; i++)
    {
        m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs
            [i].nMsg, MsgFuncs[i].func));
    }
    //}

    //初始化线程
    m_hThread = INVALID_HANDLE_VALUE;
    m_nThreadID = -1;
    m_hThreadWatch = INVALID_HANDLE_VALUE;

    m_isClosed = true;
}

//初始化函数  开启接收消息线程
int CClientController::InitController()
{
    //开启线程
    m_hThread = (HANDLE)_beginthreadex(
        NULL, 0,
        &CClientController::ThreadEntry, this, 0, &m_nThreadID);//可以返回线程IDm_nThreadID
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
    m_isClosed = true;//会让监控线程自动走向死亡
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






//消息线程函数入口
unsigned __stdcall CClientController::ThreadEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadFunc();
    _endthreadex(0);
    return 0;
}
void CClientController::threadFunc()
{
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_SEND_MESSAGE)//通过SendMessage()发送
        {
            MSGINFO* pmsg = (MSGINFO*)msg.wParam;
            HANDLE hEvent = (HANDLE)msg.lParam;
            std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
            if (it != m_mapFunc.end())
            {
                //执行对应消息函数
                pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);

            }
            else
            {
                pmsg->result = -1;
            }
            SetEvent(hEvent);
        }
        else//普通消息发送
        {
            std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
            if (it != m_mapFunc.end())
            {
                //执行对应消息函数
                (this->*it->second)(msg.message, msg.wParam, msg.lParam);
            }
        }

    }

}

//发送包
//LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//    CClientSocket* pClient = CClientSocket::getInstance();
//    CPacket* pPacket = (CPacket*)wParam;
//    return pClient->Send(*pPacket);
//}
// 
//发送数据
//LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//    CClientSocket* pClient = CClientSocket::getInstance();
//    char* pBuffer = (char*)wParam;
//    return pClient->Send(pBuffer,(int)lParam);
//}
//显示状态框
LRESULT CClientController::OnSendStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_StatusDlg.ShowWindow(SW_SHOW);//以非模态方式显示对话框
}
//显示监控对话框
LRESULT CClientController::OnSendWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_watchDlg.DoModal();//以模态方式显示对话框
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
