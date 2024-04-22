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
	 int ExcuteCommand(int nCmd, std::list<CPacket>& listPacket,CPacket& inPacket);//ӳ��ִ�к��� key=cmd value=ִ�к���

     //ִ�к���
     static void RunCommand(void* cmdObject, int status, std::list<CPacket>& listPacket, CPacket& inPacket)//listPacket����
     {
         CCommand* thiz = (CCommand*)cmdObject;
         if (status > 0)
         {
             int ret = thiz->ExcuteCommand(status, listPacket, inPacket);//ִ�к���
             if (ret != 0)
             {
                 TRACE("ִ������ʧ�ܣ�%d ret = %d\r\n", status, ret);
             }
         }
         else
         {
             MessageBox(NULL, TEXT("�޷����������û����Զ�����"), TEXT("�ͻ�������ʧ��"), MB_OK | MB_ICONERROR);
         }
         
     }
protected:
	typedef int(CCommand::*CMDFUNC)(std::list<CPacket>& listPacket, CPacket& inPacket);//��Ա����ָ��
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
    int MakeDriverInfo(std::list<CPacket>& listPacket, CPacket& inPacket)
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

        listPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
        return 0;

    }



    //��ѯָ��Ŀ¼�µ������ļ�
    int MakeDirectoryInfo(std::list<CPacket>& listPacket, CPacket& inPacket)
    {
        std::string strpath = inPacket.strDate;
        //����Ϊ��ǰ����Ŀ¼
        if (!SetCurrentDirectoryA(strpath.c_str()))
        {
            //����ʧ��
            FILEINFO finfo;
            finfo.HasNext = FALSE;//û�к����ļ�
            listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
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
            listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));

            return -3;
        }
        int Count = 0;
        //����������Ч�ļ����ͻ���
        do
        {
            FILEINFO finfo;
            finfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
            memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));

            listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
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

        listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        TRACE("Count=%d\r\n", Count);
        return 0;
    }

    //�����ļ�ָ���ļ�
    int RunFile(std::list<CPacket>& listPacket, CPacket& inPacket)
    {
        std::string strPath = inPacket.strDate;
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//����˫���ļ�

        listPacket.push_back(CPacket(3, NULL, 0));
        return 0;
    }

    //�������ļ����ϴ����ͻ��ˣ�
    int DownloadFile(std::list<CPacket>& listPacket, CPacket& inPacket)
    {
        //���ļ�
        std::string strPath = inPacket.strDate;
        long long data = 0;
        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
        if (err != 0)
        {
            listPacket.push_back(CPacket(4, (BYTE*)&data, 8));
            return -1;
        }

        if (pFile != NULL)
        {
            //��ͻ��˷����ļ��ܴ�С
            fseek(pFile, 0, SEEK_END);//pFileָ���Ƶ��ļ�ĩβ��ƫ����Ϊ0
            data = _ftelli64(pFile);//��ȡ�ļ���ǰλ��
            listPacket.push_back(CPacket(4, (BYTE*)&data, 8));
            fseek(pFile, 0, SEEK_SET);//pFileָ���Ƶ��ļ���ͷ��ƫ����Ϊ0

            //1KB 1KB��ͻ��˷���
            char buffer[1024] = "";
            size_t rlen = 0;
            do {
                rlen = fread(buffer, 1, 1024, pFile);//ÿ�ζ�ȡ1024��char�ַ�  pFIleָ�������ƶ�
                listPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
            } while (rlen >= 1024);
            fclose(pFile);
        }
        //���ͽ������
        listPacket.push_back(CPacket(4, NULL, 0));
        return 0;

    }

    int DeleteLocalFile(std::list<CPacket>& listPacket, CPacket& inPacket)
    {
        std::string strPath = inPacket.strDate;
        //TCHAR sPath[MAX_PATH] = TEXT("");
        //MultiByteToWideChar(CP_ACP, 0, strPath.c_str(),
        //    strPath.size(),(LPWSTR)sPath, sizeof(sPath) / sizeof(TCHAR));
        DeleteFileA(strPath.c_str());

        //���ͽ������
        listPacket.push_back(CPacket(9, NULL, 0));
        return 0;
    }
    //����¼�
    int MouseEvent(std::list<CPacket>& listPacket, CPacket& inPacket)
    {
        MOUSEEV mouse;
        memcpy(&mouse, inPacket.strDate.c_str(), sizeof(MOUSEEV));
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
            listPacket.push_back(CPacket(5, NULL, 0));
       
        return 0;
    }
#include <atlimage.h>
    int SendScreen(std::list<CPacket>& listPacket, CPacket& inPacket)
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
    int LockMachine(std::list<CPacket>& listPacket, CPacket& inPacket)
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
    //����
    int UnlockMachine(std::list<CPacket>& listPacket, CPacket& inPacket) {

        //��ָ���̷߳�����ϢEsc������Ϣ
        PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0);
        listPacket.push_back(CPacket(8, NULL, 0));
        return 0;
    }

    int TestConnect(std::list<CPacket>& listPacket, CPacket& inPacket)
    {
        listPacket.push_back(CPacket(1981, NULL, 0));
        return 0;
    }
};

