#pragma once
#include<map>
#include<atlimage.h>
#include<direct.h>
#include<list>
#include<stdio.h>

#include <io.h> // ���������ļ����ҵ�ͷ�ļ�
#include"CEdoyunTool.h"
#include"ServerSocket.h"
#include "resource.h"
#include "LockDialog.h"
#include "pch.h"
#include "framework.h"

class CCommand
{
public:
	CCommand();
	 int ExcuteCommand(int nCmd);
protected:
	typedef int(CCommand::*CMDFUNC)();//��Ա����ָ��
	std::map<int, CMDFUNC> m_mapFunction;//������ŵ����ܵ�ӳ��
    CLockDialog dlg;
    unsigned threadid;

protected:

    //�ڱο��̺߳���
    static unsigned __stdcall threadLockDlg(void* arg)
    {
        CCommand* thiz = (CCommand*)arg;
        thiz->threadLockDlgMain();
        _endthreadex(0);
        return 0;
    }
    void threadLockDlgMain()
    {
        //���������Ի���
        dlg.Create(IDD_DIALOG_INFO, NULL);//�����Ի���
        dlg.ShowWindow(SW_SHOW);

        //���������Χ
        CRect rect;//������������
        rect.left = 0;
        rect.top = 0;
        rect.right = GetSystemMetrics(SM_CXFULLSCREEN);//�����Ļ���ش�С
        rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN) + 70;
        dlg.MoveWindow(rect);//�������Ƶ���������
        CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
        if (pText)
        {
            CRect rtText;
            pText->GetWindowRect(rtText);
            int nWindth = rtText.Width();
            int x = (rect.right - nWindth) / 2;
            int nHeight = rtText.Height();
            int y = (rect.bottom - nHeight) / 2;
            pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
        }
        //�������ڶ���
        dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        //�������
        ShowCursor(false);
        //����������
        ::ShowWindow(::FindWindow(TEXT("shell_TrayWnd"), NULL), SW_HIDE);
        //���������Χ
        dlg.GetWindowRect(rect);
        //�������ֻ�������Ͻ�һ�����ص��ƶ�
        rect.left = 0;
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
            if (msg.message == WM_KEYDOWN)
            {
                //if (msg.wParam == 0x1B)break;//��Esc���˳�
            }
        }
        ClipCursor(NULL);//�ָ���귶Χ
        //�ָ�������
        ::ShowWindow(::FindWindow(TEXT("shell_TrayWnd"), NULL), SW_SHOW);
        //�ָ����
        ShowCursor(true);
        dlg.DestroyWindow();
        
    }
    void Dump(BYTE* pData, size_t nSize)
    {
        std::string strOut;
        for (size_t i = 0; i < nSize; i++)
        {
            char buf[8] = "";
            if (i > 0 && (i % 16 == 0))strOut += "\n";
            snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
            strOut += buf;
        }
        strOut += "\n";
        OutputDebugStringA(strOut.c_str());
    }

    //��ѯ���̷���
    int MakeDriverInfo()
    {
        std::string result;

        // ���� A �� Z ��������
        for (int i = 1; i <= 26; i++) {
            char driveLetter = 'A' + i - 1;
            std::string drivePath = std::string(1, driveLetter) + ":\\";

            // ���Խ���ǰ����Ŀ¼����Ϊ��ǰ������
            if (SetCurrentDirectoryA(drivePath.c_str())) {//�ж�����Щ����
                if (!result.empty()) {
                    result += ',';
                }
                result += driveLetter;//����������
            }
        }

        CPacket pack(1, (BYTE*)result.c_str(), result.size());
        //CEdoyunTool::Dump((BYTE*)pack.Data(), pack.size());
        CServerSocket::getInstance()->Send(pack);
        return 0;

    }



    //��ѯָ��Ŀ¼�µ������ļ�
    int MakeDirectoryInfo()
    {
        std::string strpath;
        //std::list<FILEINFO> lstFileInfos;
        if (!CServerSocket::getInstance()->GetFilePath(strpath))
        {
            OutputDebugString(TEXT("��ǰ��������ǻ�ȡ�ļ�Ŀ¼�б������������!!"));
            return -1;
        }
        //����Ϊ��ǰ����Ŀ¼
        if (!SetCurrentDirectoryA(strpath.c_str()))
        {
            //����ʧ��
            FILEINFO finfo;
            finfo.HasNext = FALSE;//û�к����ļ�
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));//���
            CServerSocket::getInstance()->Send(pack);//����
            OutputDebugString(TEXT("û��Ȩ�޷���Ŀ¼"));
            return -2;
        }

        //���õ�ǰ����Ŀ¼�ɹ�
        _finddata_t fdata; // �������� fdata
        int hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1)//�ҹ���Ŀ¼��ƥ��ĵ�һ���ļ�  ��һ������ʹ��ͨ��������ļ�����
        {
            OutputDebugString(TEXT("û���ҵ��κ��ļ�"));
            //���ͽ����ļ�
            FILEINFO finfo;
            finfo.HasNext = false;//���ý����ļ����
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));//���
            CServerSocket::getInstance()->Send(pack);//����

            return -3;
        }
        int Count = 0;
        //����������Ч�ļ����ͻ���
        do
        {
            FILEINFO finfo;
            finfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
            memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
            CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));//���
            CServerSocket::getInstance()->Send(pack);//����
            //TRACE("[������]�����ļ�=%s\r\n", finfo.szFileName);
            Count++;
            //TRACE("[���������ݰ���\r\n]");
            //Dump((BYTE*)pack.Data(), pack.size());

            //TRACE("[������]�����ļ���[%s]\r\n", finfo.szFileName);

            //lstFileInfos.push_back(finfo);
        } while (!_findnext(hfind, &fdata));//���ҹ���Ŀ¼ƥ�����һ���ļ�

        //���ͽ����ļ�
        FILEINFO finfo;
        finfo.HasNext = false;//���ý����ļ����
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));//���
        CServerSocket::getInstance()->Send(pack);//����

        TRACE("Count=%d\r\n", Count);
        return 0;
    }

    //�����ļ�ָ���ļ�
    int RunFile()
    {
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//����˫���ļ�

        //���ͻ�ӦACK
        CPacket pack(3, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    //�������ļ����ϴ����ͻ��ˣ�
    int DownloadFile()
    {
        //���ļ�
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        long long data = 0;
        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
        if (err != 0)
        {
            CPacket pack(4, (BYTE*)&data, 8);
            CServerSocket::getInstance()->Send(pack);
            return -1;
        }

        if (pFile != NULL)
        {
            //��ͻ��˷����ļ��ܴ�С
            fseek(pFile, 0, SEEK_END);//pFileָ���Ƶ��ļ�ĩβ��ƫ����Ϊ0
            data = _ftelli64(pFile);//��ȡ�ļ���ǰλ��
            CPacket head(4, (BYTE*)&data, 8);
            CServerSocket::getInstance()->Send(head);
            fseek(pFile, 0, SEEK_SET);//pFileָ���Ƶ��ļ���ͷ��ƫ����Ϊ0

            //1KB 1KB��ͻ��˷���
            char buffer[1024] = "";
            size_t rlen = 0;
            do {
                rlen = fread(buffer, 1, 1024, pFile);//ÿ�ζ�ȡ1024��char�ַ�  pFIleָ�������ƶ�
                CPacket pack(4, (BYTE*)buffer, rlen);
                CServerSocket::getInstance()->Send(pack);
            } while (rlen >= 1024);
            fclose(pFile);
        }
        //���ͽ������
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;

    }

    int DeleteLocalFile()
    {
        //TODO:
        std::string strPath;
        CServerSocket::getInstance()->GetFilePath(strPath);
        //TCHAR sPath[MAX_PATH] = TEXT("");
        //MultiByteToWideChar(CP_ACP, 0, strPath.c_str(),
        //    strPath.size(),(LPWSTR)sPath, sizeof(sPath) / sizeof(TCHAR));
        DeleteFileA(strPath.c_str());

        //���ͽ������
        CPacket pack(9, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }
    //����¼�
    int MouseEvent()
    {
        MOUSEEV mouse;
        if (CServerSocket::getInstance()->GetMouseEvent(mouse))
        {
            SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);//����ƶ�����Ļ��Ӧλ��
            DWORD nFlags = 0;
            switch (mouse.nButton)
            {
            case 0://���
                nFlags = 1;
                break;
            case 1://�Ҽ�
                nFlags = 2;
                break;
            case 2://�м�
                nFlags = 4;
                break;
            case 4://û�а���
                nFlags = 8;
                break;
            }
            if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
            switch (mouse.nAction)
            {
            case 0://����
                nFlags |= 0x10;
                break;
            case 1://˫��
                nFlags |= 0x20;
                break;
            case 2://����
                nFlags |= 0x40;
                break;
            case 3://�ſ�
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
            CPacket pack(4, NULL, 0);
            CServerSocket::getInstance()->Send(pack);
        }
        else {
            OutputDebugString(TEXT("��ȡ����������ʧ�ܣ���"));
            return -1;
        }
        return 0;
    }
#include <atlimage.h>
    int SendScreen()
    {
        //��Ļ��ͼ
        HDC hScreen = ::GetDC(NULL);//��ȡ��Ļ�����ľ������Ļ��ͼ��
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//RGBλ��� R8λ G8λ B8λ ��24λ
        int nWidth = GetDeviceCaps(hScreen, HORZRES);//��
        int nHeight = GetDeviceCaps(hScreen, VERTRES);

        //����һ��ͼ�����
        CImage screen;
        screen.Create(nWidth, nHeight, nBitPerPixel);

        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);//��hScreenͼ���Ƶ�screenͼ����

        //ɾ����Ļ��ͼ
        ReleaseDC(NULL, hScreen);

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//����һ�����ƶ����ڴ��
        if (hMem == NULL)return -1;
        IStream* pStream = NULL;//����һ��ָ��IStream�ӿڵ�ָ��
        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//�����ڴ�鴴��һ���ڴ�����pStreamָ��������
        if (ret == S_OK)
        {
            screen.Save(pStream, Gdiplus::ImageFormatPNG);//��ͼƬ���浽�ڴ�����
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);//����ָ���Ƶ�������ʼλ��
            PBYTE pData = (PBYTE)GlobalLock(hMem);//�����ڴ�飬ת��Ϊ�ֽ���ָ�룬��ȡ�ڴ�����ʼ��ַ
            SIZE_T nSize = GlobalSize(hMem);//��ȡ�����ڴ���С
            CPacket pack(6, pData, nSize);
            CServerSocket::getInstance()->Send(pack);
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
    int LockMachine()
    {
        if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE))
        {
            //�����̴߳��ڱο�
            _beginthreadex(NULL, 0, CCommand::threadLockDlg, this, 0, &threadid);

        }
        //����Ӧ����Ϣ
        CPacket pack(7, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }
    //����
    int UnlockMachine() {

        //��ָ���̷߳�����ϢEsc������Ϣ
        PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0);
        CPacket pack(8, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
        return 0;
    }

    int TestConnect()
    {
        CPacket pack(1981, NULL, 0);
        int ret = CServerSocket::getInstance()->Send(pack);
        TRACE("Send ret = %d\r\n", ret);
        return 0;
    }
};

