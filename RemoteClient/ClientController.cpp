#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;

//���캯��
CClientController::CClientController() :m_StatusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg)//���ø�����
{
    //if (CClientController::getInstance() == NULL)
    //{
        //CClientController::m_instance = CClientController::getInstance();

        //ע����Ϣmap<�ź�,ִ�к���>
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

    //��ʼ���߳�
    m_hThread = INVALID_HANDLE_VALUE;
    m_nThreadID = -1;
    m_hThreadWatch = INVALID_HANDLE_VALUE;

    m_isClosed = true;
}

//��ʼ������  ����������Ϣ�߳�
int CClientController::InitController()
{
    //�����߳�
    m_hThread = (HANDLE)_beginthreadex(
        NULL, 0,
        &CClientController::ThreadEntry, this, 0, &m_nThreadID);//���Է����߳�IDm_nThreadID
    m_StatusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);

    return 0;
}

//����ģ̬�Ի���
int CClientController::Invoke(CWnd*& m_pMainWnd)
{
    m_pMainWnd = &m_remoteDlg;//����������
    return m_remoteDlg.DoModal();//����������Ϊģ̬�Ի���
}

void  CClientController::StartWatchScreen()
{
    m_isClosed = false;
    m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry, 0, this);
    m_watchDlg.DoModal();//����ģ̬�Ի�������ֱ���Ի��򱻹ر�
    m_isClosed = true;//���ü���߳��Զ���������
    //������ǰ�̣߳��ȴ�m_hThreadWatch�߳���ɹ���������500����
    WaitForSingleObject(m_hThreadWatch, 500);
}

//����߳���ں���
void CClientController::threadWatchScreenEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadWatchScreen();
    _endthread();//�����߳�
}

//����̺߳���
void CClientController::threadWatchScreen()
{
    Sleep(50);
    ULONGLONG nTick = GetTickCount64();
    while (!m_isClosed)
    {
        if (m_watchDlg.isFull() == false)//�������ݵ�����
        {
            if (GetTickCount64() - nTick < 200)//���Ʒ�������Ƶ��
            {
                Sleep(200 - DWORD(GetTickCount64() - nTick));
            }
            nTick = GetTickCount64();

            //���ͼ������
            int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6,true, NULL, 0);
            if (ret == 1)
            {
               // TRACE("�ɹ���������ͼƬ����!\r\n");
            }
            else
            {
                TRACE("��ȡͼƬʧ��!\r\n");
            }
        }
        Sleep(1);
    }
}






//��Ϣ�̺߳������
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
        if (msg.message == WM_SEND_MESSAGE)//ͨ��SendMessage()����
        {
            MSGINFO* pmsg = (MSGINFO*)msg.wParam;
            HANDLE hEvent = (HANDLE)msg.lParam;
            std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
            if (it != m_mapFunc.end())
            {
                //ִ�ж�Ӧ��Ϣ����
                pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);

            }
            else
            {
                pmsg->result = -1;
            }
            SetEvent(hEvent);
        }
        else//��ͨ��Ϣ����
        {
            std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
            if (it != m_mapFunc.end())
            {
                //ִ�ж�Ӧ��Ϣ����
                (this->*it->second)(msg.message, msg.wParam, msg.lParam);
            }
        }

    }

}

//���Ͱ�
//LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//    CClientSocket* pClient = CClientSocket::getInstance();
//    CPacket* pPacket = (CPacket*)wParam;
//    return pClient->Send(*pPacket);
//}
// 
//��������
//LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//    CClientSocket* pClient = CClientSocket::getInstance();
//    char* pBuffer = (char*)wParam;
//    return pClient->Send(pBuffer,(int)lParam);
//}
//��ʾ״̬��
LRESULT CClientController::OnSendStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_StatusDlg.ShowWindow(SW_SHOW);//�Է�ģ̬��ʽ��ʾ�Ի���
}
//��ʾ��ضԻ���
LRESULT CClientController::OnSendWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_watchDlg.DoModal();//��ģ̬��ʽ��ʾ�Ի���
}

int CClientController::DownFile(CString strPath)//strPathΪԶ�������ļ�·��
{
    //�������ضԻ���
    CFileDialog dlg(FALSE, NULL,
        strPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY
        , NULL, &m_remoteDlg);
    if (dlg.DoModal() == IDOK)//���ȷ������
    {
        m_strRemote = strPath;//Զ���ļ�·����
        m_strLocal = dlg.GetPathName();//���ر����ļ����ļ���
        //�򿪱����ļ�
        FILE* pFile = fopen(m_strLocal, "wb+");
        if (pFile == NULL)//��ʧ��
        {
            AfxMessageBox(TEXT("����û��Ȩ�ޱ�����ļ��������ļ��޷�����!!!"));
            return -1;
        }

        //������������
        SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);

        m_remoteDlg.BeginWaitCursor();//����ɳ©
        m_StatusDlg.m_info.SetWindowText(TEXT("��������ִ����"));
        m_StatusDlg.ShowWindow(SW_SHOW);//��ʾ����״̬�Ի���
        m_StatusDlg.CenterWindow(&m_remoteDlg);//�Ի������
        m_StatusDlg.SetActiveWindow();//����Ϊ���㴰��
    }
    return 0;


}
