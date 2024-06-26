#include "pch.h"
#include "Command.h"

//��Ҫ��<cmd,������>����map��
CCommand::CCommand() :threadid(0)
{
	struct {
		int nCmd;//��������
		CMDFUNC func;//����ָ��
	}data[] = {
		{1,&CCommand::MakeDriverInfo},//�鿴���̷���
		{2,&CCommand::MakeDirectoryInfo},//�鿴Ŀ¼�µ������ļ�
		{3,&CCommand::RunFile},//�����ļ�
		{4,&CCommand::DownloadFile},//�����ļ�
		{5,&CCommand::MouseEvent},//����¼�
		{6,&CCommand::SendScreen},//���ͽ�ͼ
		{7,&CCommand::LockMachine},//����
		{8,&CCommand::UnlockMachine},//����
		{9,&CCommand::DeleteLocalFile},//ɾ���ļ�
		{1981,&CCommand::TestConnect},//����
		{-1,NULL},
	};

	//<����,����ָ��>  ����map
	for (int i = 0; data[i].nCmd != -1; i++)
	{
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}

}

//ִ�к��� �����ص�    ���ó�Ա����ExcuteCommand����
void CCommand::RunCommand(void* cmdObject, int nCmd, CPacket& inPacket, std::list<CPacket>& listPacket)//listPacket����
{
    CCommand* thiz = (CCommand*)cmdObject;
    if (nCmd > 0)
    {
        int ret = thiz->ExcuteCommand(nCmd, inPacket,listPacket);//ִ�к���
        if (ret != 0)
        {
            TRACE("ִ������ʧ�ܣ�%d ret = %d\r\n", nCmd, ret);
        }
    }
    else
    {
        MessageBox(NULL, TEXT("�޷����������û����Զ�����"), TEXT("�ͻ�������ʧ��"), MB_OK | MB_ICONERROR);
    }

}

//ִ�г�Ա���������������Ӧ�ĺ���
int CCommand::ExcuteCommand(int nCmd, CPacket& inPacket,std::list<CPacket>& listPacket)
{
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it != m_mapFunction.end())
	{
		return (this->*it->second)(listPacket, inPacket);//ִ���������
	}
	return 0;
};

//��ѯ���̷���
int CCommand::MakeDriverInfo(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    std::string result;//�������д��ڵ��̷�

    // ���� A �� Z ��������
    for (int i = 1; i <= 26; i++) {
        char driveLetter = 'A' + i - 1;//�̷�
        std::string drivePath = std::string(1, driveLetter) + ":\\";

        // ���Խ���ǰ����Ŀ¼����Ϊ��ǰ������
        if (SetCurrentDirectoryA(drivePath.c_str())) {//�ж�����Щ����,���ĳɹ�˵���̷�����
            if (!result.empty()) {
                result += ',';
            }
            result += driveLetter;//����������
        }
    }
    listPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
    return 0;
}


/*����Ŀ¼����
    struct _finddata_t c_file;//�ļ��ṹ��
    long hFile; //�ļ����
    if ((hFile = _findfirst("C:\\path\\to\\directory\\*.txt", &c_file)) != -1L) {
          do {
              printf("%s\n", c_file.name);
          } while (_findnext(hFile, &c_file) == 0);
          _findclose(hFile);
     }

*/
//��ѯָ��Ŀ¼�µ������ļ�
int CCommand::MakeDirectoryInfo(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    std::string strpath = inPacket.strData;
    //����Ϊ��ǰ����Ŀ¼
    if (!SetCurrentDirectoryA(strpath.c_str()))
    {
        //����ʧ�ܣ�
        FILEINFO finfo;
        finfo.HasNext = FALSE;//û�к����ļ�
        listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        OutputDebugString(TEXT("û��Ȩ�޷���Ŀ¼"));
        return -2;
    }

    //���õ�ǰ����Ŀ¼�ɹ�
    _finddata_t c_file; //�ļ��ṹ��
    int hFile = 0;//�ļ����
    if ((hFile = _findfirst("*", &c_file)) == -1)//�ҹ���Ŀ¼��ƥ��ĵ�һ���ļ�  ��һ������ʹ��ͨ��������ļ�����
    {
        
        //���ͽ����ļ�
        FILEINFO finfo;//�ļ���Ϣ�ṹ��
        finfo.HasNext = FALSE;//���ý����ļ����
        listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        OutputDebugString(TEXT("û���ҵ��κ��ļ�"));
        return -3;
    }
    //int Count = 0;//�ļ���Ŀ

    //����������Ч�ļ����ͻ���
    do
    {
        FILEINFO finfo;
        finfo.IsDirectory = ((c_file.attrib & _A_SUBDIR) != 0);
        memcpy(finfo.szFileName, c_file.name, strlen(c_file.name));
        listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));      
       // Count++;  
        //TRACE("[������]�����ļ���[%s]\r\n", finfo.szFileName);     
    } while (!_findnext(hFile, &c_file));//���ҹ���Ŀ¼ƥ�����һ���ļ�

    //���ͽ����ļ�
    FILEINFO finfo;
    finfo.HasNext = false;//���������һ���ļ�
    listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
    //TRACE("Count=%d\r\n", Count);
    return 0;
}

//�����ļ�ָ���ļ�
int CCommand::RunFile(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    std::string strPath = inPacket.strData;
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//����˫���ļ� ����
    listPacket.push_back(CPacket(3, NULL, 0));
    return 0;
}

//�������ļ����ϴ����ͻ��ˣ�
int CCommand::DownloadFile(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    //���ļ�
    std::string strPath = inPacket.strData;
    long long dataSize = 0;
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    if (err != 0)//�ļ���ʧ��
    {
        listPacket.push_back(CPacket(4, (BYTE*)&dataSize, 8));
        return -1;
    }

    //�ļ��򿪳ɹ�
    if (pFile != NULL)
    {
        //��ͻ��˷����ļ��ܴ�С
        fseek(pFile, 0, SEEK_END);//pFileָ���Ƶ��ļ�ĩβ��ƫ����Ϊ0
        dataSize = _ftelli64(pFile);//��ȡ�ļ�ָ�뵱ǰλ��    ���ļ���С
        listPacket.push_back(CPacket(4, (BYTE*)&dataSize, 8));


        fseek(pFile, 0, SEEK_SET);//pFileָ���Ƶ��ļ���ͷ��ƫ����Ϊ0
        //1KB 1KB��ͻ��˷���
        char buffer[1024] = "";
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);//ÿ�ζ�ȡ1024��char�ַ�  pFIleָ�������ƶ�
            listPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
        } while (rlen >= 1024);//ֻ�ж�ȡ���������ڻ�������Сʱ�Ż�û�ж���
        fclose(pFile);
    }
    //���ͽ������
    listPacket.push_back(CPacket(4, NULL, 0));
    return 0;

}

int CCommand::DeleteLocalFile(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    std::string strPath = inPacket.strData;
    DeleteFileA(strPath.c_str());

    //���ͽ������
    listPacket.push_back(CPacket(9, NULL, 0));
    return 0;
}

//����¼�
int CCommand::MouseEvent(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    MOUSEEV mouse;
    memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));

    DWORD nFlags = 0;
    switch (mouse.nButton)////������Ҽ�������
    {
    case 0://���             0000 0001
        nFlags = 1;
        break;
    case 1://�Ҽ�             0000 0010
        nFlags = 2; 
        break;
    case 2://����             0000 0100
        nFlags = 4;
        break;
    case 4://û�а���         0000 1000
        nFlags = 8;
        break;
    }
    if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);//�ƶ���굽ָ����Ļָ��λ��

    //����    �������ƶ���˫��
    switch (mouse.nAction)
    {
    case 0://����             0001 0000
        nFlags |= 0x10; 
        break;
    case 1://˫��             0010 0000
        nFlags |= 0x20;
        break;
    case 2://����             0100 0000
        nFlags |= 0x40;
        break;
    case 3://�ſ�             1000 0000
        nFlags |= 0x80;
        break;
    default:
        break;
    }

    switch (nFlags)
    {
    case 0x21://���˫��
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        //û��break�����ִ��һ�ε���
    case 0x11://�������
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        break;
    case 0x41://�������
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
        break;
    case 0x81://����ſ�
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        break;
    case 0x22://�Ҽ�˫��
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
    case 0x12://�Ҽ�����
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        break;
    case 0x42://�Ҽ�����
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
        break;
    case 0x82://�Ҽ��ſ�
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        break;
    case 0x24://�н�˫��
        mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
        mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
    case 0x14://�м�����
        mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
        mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        break;
    case 0x44://�м�����
        mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
        break;
    case 0x84://�м��ſ�
        mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        break;
    case 0x08://��������ƶ�
        mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
        break;
    }

    //���ͽ�����־
    listPacket.push_back(CPacket(5, NULL, 0));
    return 0;
}

//���ͽ�ͼ
int CCommand::SendScreen(std::list<CPacket>& listPacket, CPacket& inPacket)
{
   
    HDC hScreen = ::GetDC(NULL);//��ȡ��Ļ������
    /*
        HDC����һ�������Handle��������һ���豸�����ġ��豸��������һ��Windows���ݽṹ���������˻���ͼ���������Ϣ��
        ���豸���͡���ͼ��ɫ�������������ʽ�ȡ�
        
        ::GetDC�����ڻ�ȡָ�����ڣ���������Ļ�����豸������
    */

    //��ȡ��Ļ�����ĵ���ɫλ�� ��͸�
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//ÿ�����ص���ɫλ��(��ɫ���)
    /*
    nBitPerPixel=32;
    ��ζ��ÿ������ʹ��32λ����ʾ��ɫ
    ͨ������24λ���ں졢�̡�����RGB����ɫͨ����ÿ��ͨ��8λ��
    �Լ������8λ��������alphaͨ����͸���ȣ�������Ŀ�ģ���������Ļ����ʾʱ��������8λͨ������ʹ�ã���Ϊ��Ļ��֧��͸���ȣ���
    */
    //TRACE("���ص�λ��:%d", nBitPerPixel);
    int nWidth = GetDeviceCaps(hScreen, HORZRES);//��Ļ��
    int nHeight = GetDeviceCaps(hScreen, VERTRES);//��Ļ��

    //����һ��ͼ�����(������Ļ�����Ŀ����ɫλ��)  ����Ļ��λͼ���Ƶ�ͼ������е�λͼ��
    CImage screen;
    screen.Create(nWidth, nHeight, nBitPerPixel);
    BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);//��hScreenͼ���Ƶ�screenͼ����
    /*
          
          ��һ���豸�����ģ�DC����ָ��������λͼ��һ��Ŀ���豸�����ĵ�ָ������
          BOOL BitBlt(  
          HDC     hdcDest,          // Ŀ���豸�����ľ��  
          int     nXDest,           // Ŀ��������Ͻǵ�X����  
          int     nYDest,           // Ŀ��������Ͻǵ�Y����  
          int     nWidth,           // ��������Ŀ��  
          int     nHeight,          // ��������ĸ߶�  
          HDC     hdcSource,        // Դ�豸�����ľ��  
          int     nXSrc,            // Դ�������Ͻǵ�X����  
          int     nYSrc,            // Դ�������Ͻǵ�Y����  
          DWORD   rop               // ��դ������  
            );
    
    */
    
    ReleaseDC(NULL, hScreen);//�ͷ���Ļ������

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//����һ�����ƶ����ڴ��
    if (hMem == NULL)return -1;
    IStream* pStream = NULL;//����һ��ָ��IStream�ӿڵ�ָ��
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//�����ڴ�鴴��һ���ڴ�����pStreamָ��������
    if (ret == S_OK)
    {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);//��ͼƬ���浽�ڴ�����(��ָ����ƶ�)
        LARGE_INTEGER bg = { 0 };//ƫ����
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);//����ָ���Ƶ�������ʼλ��
        PBYTE pData = (PBYTE)GlobalLock(hMem);//�����ڴ�飬ת��Ϊ�ֽ���ָ�룬��ȡ�ڴ�����ʼ��ַ
        //Globallock�������������ڴ���ָ�����ڴ�飬������һ����ֵַ���õ�ֵַָ���ڴ�����ʼ����
        SIZE_T nSize = GlobalSize(hMem);//��ȡ�����ڴ���С
        listPacket.push_back(CPacket(6, pData, nSize));
        GlobalUnlock(hMem);//�ڴ�����
    }


    /*
    //����PNG��JPEGCPU��ĶԱ�
    DWORD tick = GetTickCount64();
    screen.Save(TEXT("test2020.png"), Gdiplus::ImageFormatPNG);
    TRACE("pnd %d\n", GetTickCount64() - tick);
    tick = GetTickCount64();
    screen.Save(TEXT("test2020.jpg"), Gdiplus::ImageFormatJPEG);
    TRACE("JPEG %d\n", GetTickCount64() - tick);
    */
    pStream->Release();//�ͷ���
    GlobalFree(hMem);//�ͷ��ڴ��
    screen.ReleaseDC();

    return 0;
}

//����
int CCommand::LockMachine(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE))
    {
        //�����̴߳��ڱο�
        _beginthreadex(NULL, 0, CCommand::threadLockDlg, this, 0, &threadid);

    }
    //����Ӧ����Ϣ
    listPacket.push_back(CPacket(7, NULL, 0));
    return 0;
}
//�ڱο��̺߳���
unsigned __stdcall CCommand::threadLockDlg(void* arg)
{
    CCommand* thiz = (CCommand*)arg;
    thiz->threadLockDlgMain();
    _endthreadex(0);
    return 0;
}
void CCommand::threadLockDlgMain()
{
    //���������Ի���
    dlg.Create(IDD_DIALOG_INFO, NULL);//�����Ի���
    dlg.ShowWindow(SW_SHOW);//��ʾ�Ի���

    //�����ڱο��С
    CRect rect;//������������
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);//�����Ļ�Ŀ��
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN) + 70;//�����Ļ�ĸ߶�
    dlg.MoveWindow(rect);//�ƶ������Ի���Ĵ�С(�ʹ���һ����)

    //��������λ�ô�С
    CWnd* pText = dlg.GetDlgItem(IDC_STATIC);//��ȡ���ֿؼ�ָ��   IDC_STATIC�ǿؼ�ID
    if (pText)
    {
        CRect rtText;
        pText->GetWindowRect(rtText);//��ǰ���ֿؼ��������
        int nWindth = rtText.Width();//�޸Ĵ�С
        int x = (rect.right - nWindth) / 2;
        int nHeight = rtText.Height();
        int y = (rect.bottom - nHeight) / 2;
        pText->MoveWindow(x, y, rtText.Width(), rtText.Height());//���ֿؼ��ƶ����Ի����м�
    }

    //�������ڶ���
    dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    //�������
    ShowCursor(false);
    //����������
    ::ShowWindow(::FindWindow(TEXT("shell_TrayWnd"), NULL), SW_HIDE);

    //���������Χ
    dlg.GetWindowRect(rect);
    rect.left = 0;//�������ֻ�������Ͻ�һ�����ص��ƶ�
    rect.top = 0;
    rect.right = 1;
    rect.bottom = 1;
    ClipCursor(rect);//�����������

    //��Ϣѭ��
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        //�������˳���Ϣѭ��
        if (msg.message == WM_KEYDOWN)//���̰���
        {
            if (msg.wParam == 0x1B)break;//��Esc���˳�
        }
    }
    ClipCursor(NULL);//�ָ���귶Χ
    //�ָ�������
    ::ShowWindow(::FindWindow(TEXT("shell_TrayWnd"), NULL), SW_SHOW);
    //�ָ����
    ShowCursor(true);
    dlg.DestroyWindow();

}

//����
int CCommand::UnlockMachine(std::list<CPacket>& listPacket, CPacket& inPacket) {

    //��ָ���̷߳�����ϢEsc������Ϣ
    PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0);//���̷߳�����Ϣ
    listPacket.push_back(CPacket(8, NULL, 0));
    return 0;
}

int CCommand::TestConnect(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    listPacket.push_back(CPacket(1981, NULL, 0));
    return 0;
}
