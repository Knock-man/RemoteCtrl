#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;

//�����ļ��̺߳���
void CClientController::threadDownloadFile()
{
    //�򿪱����ļ�
    FILE* pFile = fopen(m_strLocal, "wb+");
    if (pFile == NULL)//��ʧ��
    {
        AfxMessageBox(TEXT("����û��Ȩ�ޱ�����ļ��������ļ��޷�����!!!"));
        m_StatusDlg.ShowWindow(SW_HIDE);
        m_StatusDlg.EndWaitCursor();
        return;
    }
    //�����ļ�(���ļ�����д������)
    CClientSocket* pClient = CClientSocket::getInstance();
    do {
        //����ģ�鷢�������ļ�����
        int ret = SendCommandPacket(m_remoteDlg,4, false,(BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength());
        if (ret < 0)
        {
            {
                AfxMessageBox("ִ����������ʧ��");
                TRACE("ִ������ʧ��:ret=%d\r\n", ret);
                break;
            }
        }
        //���մ������ļ��ܳ���
        long long  nlength = *(long long*)pClient->GetPacket().strData.c_str();//�������ļ�����
        if (nlength == 0)
        {
            AfxMessageBox("�ļ�����Ϊ0�����޷���ȡ�ļ�!!!");
            break;
        }

        //ѭ�������ļ���ֱ���ļ������������
        long long nCount = 0;
        while (nCount < nlength)
        {
            ret = pClient->DealCommand();
            if (ret < 0)
            {
                AfxMessageBox("����ʧ��!!");
                TRACE("����ʧ��:ret = %d\r\n", ret);
                break;
            }
            //��������д���ļ���
            fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
            nCount += pClient->GetPacket().strData.size();//���������س���
        }
    } while (false);
    //������ɣ��ر�
    fclose(pFile);//�ر��ļ�
    pClient->CloseSocket();//�ر��׽���
    m_StatusDlg.ShowWindow(SW_HIDE);//�ر�״̬��
    m_remoteDlg.EndWaitCursor();//�ر�ɳ©
    m_remoteDlg.MessageBox(TEXT("�������!!"), TEXT("���"));
    
}
//�����ļ��߳���ں���
void _stdcall CClientController::threadDownloadFileEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadDownloadFile();
    _endthread();
}

//����̺߳���
void CClientController::threadWatchScreen()
{
    Sleep(50);
    while (!m_isClosed)
    {
        if (m_watchDlg.isFull() == false)//�������ݵ�����
        {
            std::list<CPacket> lstPacks;
            int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), true, NULL, 0);
           //�����Ϣ��Ӧ����WM_SEND_PACK_ACK
            //TODO :���Ʒ���Ƶ��
            if (ret == 6)
            {
                if (CEdoyunTool::Bytes2Image(m_watchDlg.GetImage(), 
                    lstPacks.front().strData) == 0) {//���ս�ͼ����m_image��
                    m_watchDlg.SetImageSatus(true);
                }
                else
                {
                    TRACE("��ȡͼƬʧ��!\r\n");
                }
            }
            else
            {
                Sleep(1);
            }
        }
    }
}
//����߳���ں���
void CClientController::threadWatchScreenEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadWatchScreen();
    _endthread();
}


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
    m_hThreadDown = INVALID_HANDLE_VALUE;
    m_hThread = INVALID_HANDLE_VALUE;
    m_hThreadWatch = INVALID_HANDLE_VALUE;
    m_nThreadID = -1;
    m_isClosed = true;
}

//��ʼ������  �����̣߳�������Ϣ
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
    m_pMainWnd = &m_remoteDlg;
    return m_remoteDlg.DoModal();
}

//������Ϣ
LRESULT CClientController::SendMessage(MSG msg)
{
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);//�¼�
    if (hEvent == NULL)return -2;
    MSGINFO info(msg);//��Ϣ�ṹ��
    PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE,
        (WPARAM)&info, (LPARAM)hEvent);
    /*
    PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE,
        (WPARAM)&wParam, (LPARAM)&lParam);
    */
    WaitForSingleObject(hEvent, INFINITE);//�����¼�
    CloseHandle(hEvent);
    return info.result;
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

