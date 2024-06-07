#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;

//构造函数
CClientController::CClientController() :m_StatusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg)//设置父窗口
{
    if (CClientController::getInstance() == NULL)
    {
        CClientController::m_instance = CClientController::getInstance();

        //注册消息map<信号,执行函数>
        struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] =
        {
            {WM_SEND_PACK,&CClientController::OnSendPack},
            {WM_SEND_DATA,&CClientController::OnSendData},
            {WM_SHOW_STATUS,&CClientController::OnSendStatus},
            {WM_SHOW_WATCH,&CClientController::OnSendWatcher},
            {(UINT)-1,NULL}
        };
        for (int i = 0; MsgFuncs[i].func != NULL; i++)
        {
            m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs
                [i].nMsg, MsgFuncs[i].func));
        }
    }

    //初始化线程
    m_hThread = INVALID_HANDLE_VALUE;
    m_nThreadID = -1;
}

//初始化函数  开启线程
int CClientController::InitController()
{
    //开启线程
    m_hThread = (HANDLE)_beginthreadex(
        NULL, 0,
        &CClientController::ThreadEntry, this, 0, &m_nThreadID);
    m_StatusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);

    return 0;
}

//启动模态对话框
int CClientController::Invoke(CWnd*& m_pMainWnd)
{
    m_pMainWnd = &m_remoteDlg;
    return m_remoteDlg.DoModal();
}

//发送消息
LRESULT CClientController::SendMessage(MSG msg)
{
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent == NULL)return -2;
    MSGINFO info(msg);
    PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE,
        (WPARAM)&info, (LPARAM)hEvent);
    /*
    PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE,
        (WPARAM)&wParam, (LPARAM)&lParam);
    */
    WaitForSingleObject(hEvent, -1);
    return info.result;
}


//线程函数
unsigned __stdcall CClientController::ThreadEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadFunc();
    _endthreadex(0);
    return 0;
}

LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return LRESULT();
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return LRESULT();
}

LRESULT CClientController::OnSendStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_StatusDlg.ShowWindow(SW_SHOW);//以非模态方式显示对话框
}

LRESULT CClientController::OnSendWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_watchDlg.DoModal();//以模态方式显示对话框
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