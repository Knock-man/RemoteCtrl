#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;

//下载文件线程函数
void CClientController::threadDownloadFile()
{
    //打开保存文件
    FILE* pFile = fopen(m_strLocal, "wb+");
    if (pFile == NULL)//打开失败
    {
        AfxMessageBox(TEXT("本地没有权限保存该文件，或者文件无法创建!!!"));
        m_StatusDlg.ShowWindow(SW_HIDE);
        m_StatusDlg.EndWaitCursor();
        return;
    }
    //下载文件(向文件里面写入数据)
    CClientSocket* pClient = CClientSocket::getInstance();
    do {
        //网络模块发送下载文件请求
        int ret = SendCommandPacket(m_remoteDlg,4, false,(BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength());
        if (ret < 0)
        {
            {
                AfxMessageBox("执行下载命令失败");
                TRACE("执行下载失败:ret=%d\r\n", ret);
                break;
            }
        }
        //接收待下载文件总长度
        long long  nlength = *(long long*)pClient->GetPacket().strData.c_str();//待下载文件长度
        if (nlength == 0)
        {
            AfxMessageBox("文件长度为0或者无法读取文件!!!");
            break;
        }

        //循环下载文件，直至文件完整下载完成
        long long nCount = 0;
        while (nCount < nlength)
        {
            ret = pClient->DealCommand();
            if (ret < 0)
            {
                AfxMessageBox("传输失败!!");
                TRACE("传输失败:ret = %d\r\n", ret);
                break;
            }
            //下载数据写入文件中
            fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
            nCount += pClient->GetPacket().strData.size();//计算已下载长度
        }
    } while (false);
    //下载完成，关闭
    fclose(pFile);//关闭文件
    pClient->CloseSocket();//关闭套接字
    m_StatusDlg.ShowWindow(SW_HIDE);//关闭状态栏
    m_remoteDlg.EndWaitCursor();//关闭沙漏
    m_remoteDlg.MessageBox(TEXT("下载完成!!"), TEXT("完成"));
    
}
//下载文件线程入口函数
void _stdcall CClientController::threadDownloadFileEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadDownloadFile();
    _endthread();
}

//监控线程函数
void CClientController::threadWatchScreen()
{
    Sleep(50);
    while (!m_isClosed)
    {
        if (m_watchDlg.isFull() == false)//更新数据到缓存
        {
            std::list<CPacket> lstPacks;
            int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), true, NULL, 0);
           //添加消息响应函数WM_SEND_PACK_ACK
            //TODO :控制发送频率
            if (ret == 6)
            {
                if (CEdoyunTool::Bytes2Image(m_watchDlg.GetImage(), 
                    lstPacks.front().strData) == 0) {//接收截图存入m_image中
                    m_watchDlg.SetImageSatus(true);
                }
                else
                {
                    TRACE("获取图片失败!\r\n");
                }
            }
            else
            {
                Sleep(1);
            }
        }
    }
}
//监控线程入口函数
void CClientController::threadWatchScreenEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadWatchScreen();
    _endthread();
}


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
    m_hThreadDown = INVALID_HANDLE_VALUE;
    m_hThread = INVALID_HANDLE_VALUE;
    m_hThreadWatch = INVALID_HANDLE_VALUE;
    m_nThreadID = -1;
    m_isClosed = true;
}

//初始化函数  开启线程，接收消息
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
    m_pMainWnd = &m_remoteDlg;
    return m_remoteDlg.DoModal();
}

//发送消息
LRESULT CClientController::SendMessage(MSG msg)
{
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//事件
    if (hEvent == NULL)return -2;
    MSGINFO info(msg);//消息结构体
    PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE,
        (WPARAM)&info, (LPARAM)hEvent);
    /*
    PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE,
        (WPARAM)&wParam, (LPARAM)&lParam);
    */
    WaitForSingleObject(hEvent, INFINITE);//回收事件
    CloseHandle(hEvent);
    return info.result;
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

