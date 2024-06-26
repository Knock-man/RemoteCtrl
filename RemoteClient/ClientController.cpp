#include "pch.h"
#include "ClientController.h"


//���캯��
CClientController::CClientController() :m_StatusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg)//���ø�����
{
    m_nThreadID = -1;
    m_hThreadWatch = INVALID_HANDLE_VALUE;

    m_isClosed = true;
}

//��ʼ������  ����״̬�Ի���
int CClientController::InitController()
{
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
    m_isClosed = true;//���ü���߳��Զ��������� ����߳�while(true)
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
