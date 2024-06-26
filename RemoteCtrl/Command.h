/*
ҵ��ģ��
���ã�������������ûص����������ݰ�,���ݲ������������Ӧ���������д���

����ʵ��ԭ��
    1.����һ���̣߳��߳��ڴ���һ���ڱο�  _beginthreadex()
    2.�����ڱο��СΪ��Ļ��С���ö���λ��Ϊ��Ļλ��   ����CRect��С   dlg.MoveWindow(rect)
    3.���������ΧΪ���Ͻ�һ�����أ��������������������
    4.����while()�¼�ѭ�� ������ǰ�߳�  ���յ�Esc��Ϣʱ�Ž���ѭ��  MSG msg ; GetMessage() TranslateMessage() DispatchMessage()
    5.�ָ���ꡢ�������������ڱο�

�鿴���̷���ԭ��
    1.�л��� A-Z:\\  Ŀ¼�Ƿ�ɹ� �ɹ����ڴ˸�Ŀ¼  �л���ǰ����·��API SetCurrentDirectoryA()

��ѯָ��Ŀ¼�µ������ļ�ԭ��
    1.�л��鿴Ŀ¼Ϊ����Ŀ¼  SetCurrentDirectoryA()
    2.long hFile = _findfirst()�鿴��һ���ļ�
    3._findnext()�鿴��һ���ļ�

    struct _finddata_t Ϊ�ļ���Ϣ�ṹ��

����ָ���ļ�ԭ��
    1.ʹ��API ShellExecuteA()  ˫��Ч��

�����ļ�ԭ��
    1.fopen_s()���ļ�
    2.fseek()������ļ���С ���͸��ͻ���
    3.fread()һ�ζ�һ���������Ĵ�С���� ���͸��ͻ��� ֱ���������ݷ������
    4.fclose()�ر��ļ�

ɾ���ļ�ԭ��
    1.ʹ��API DeleteFileA()

����¼�ԭ��
    1.����������ת��ΪMOUSEEV�ṹ����
    2.��ͬ����������ȡ��ͬ��Ϊ  API mouse_event() ��Ӧ����¼�

��ͼʵ��ԭ��:
    1.��ͼ
        (1)��ȡ��Ļ������DC  HDC hScreen = ::GetDC(NULL);
        (2)������Ļ������DC ����һ��ͼ�����
                 CImage screen;     
                 screen.Create(nWidth, nHeight, nBitPerPixel);
        (3)����Ļλͼ���Ƶ�ͼ�����λͼ����ɽ�ͼ  BitBlt()
        (4)�ͷ���Ļ������

    2.��ͼ������CImage���浽string��
        (1)����һ�����ƶ����ڴ��  HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
        (2)����һ���ڴ��� �����ڴ��
            IStream* pStream = NULL;//�ڴ���ָ��
            HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//�����ڴ�鴴��һ���ڴ���
        (3)ʹ���ڴ���ָ�뽫ͼ�񱣴浽�ڴ����
            screen.Save(pStream, Gdiplus::ImageFormatPNG);
        (4)���ڴ���ָ���Ƶ���ʼλ��
            pStream->Seek(bg, STREAM_SEEK_SET, NULL)
        (5)�����ڴ�� ����ڴ��ĵ�ַ
            PBYTE pData = (PBYTE)GlobalLock(hMem)//����ڴ���ַ
            SIZE_T nSize = GlobalSize(hMem);//��ȡ�����ڴ���С
        (6)�����ڴ�����
            listPacket.push_back(CPacket(6, pData, nSize));
        (7)�ڴ����� GlobalUnlock(hMem);
           �ͷ��ڴ�� GlobalFree(hMem);  
           �ͷ��ڴ��� pStream->Release();
           �ͷ�ͼ�������� screen.ReleaseDC();
*/
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

//ҵ������
class CCommand
{
public:
    //��Ҫ��<cmd,������> ����map��
    CCommand();

    //ִ�к��� �������õĻص�����    ���ó�Ա����cmdObject->ExcuteCommand()
    static void RunCommand(void* cmdObject, int nCmd, CPacket& inPacket,std::list<CPacket>& listPacket);//listPacket����

    int ExcuteCommand(int nCmd, CPacket& inPacket,std::list<CPacket>& listPacket);//ӳ��ִ�к��� key=cmd value=ִ�к���

    

protected:
    typedef int(CCommand::* CMDFUNC)(std::list<CPacket>& listPacket, CPacket& inPacket);//����ָ��
    std::map<int, CMDFUNC> m_mapFunction;//������ŵ���������ӳ��
    CLockDialog dlg;//�����ڱο�
    unsigned threadid;//�߳�ID

private:
    //�ڱο��̺߳���
    static unsigned __stdcall threadLockDlg(void* arg);
    void threadLockDlgMain();

    //��ѯ���̷���
    int MakeDriverInfo(std::list<CPacket>& listPacket, CPacket& inPacket);

    //��ѯָ��Ŀ¼�µ������ļ�
    int MakeDirectoryInfo(std::list<CPacket>& listPacket, CPacket& inPacket);

    //�����ļ�ָ���ļ�
    int RunFile(std::list<CPacket>& listPacket, CPacket& inPacket);

    //�������ļ����ϴ����ͻ��ˣ�
    int DownloadFile(std::list<CPacket>& listPacket, CPacket& inPacket);

    int DeleteLocalFile(std::list<CPacket>& listPacket, CPacket& inPacket);

    //����¼�
    int MouseEvent(std::list<CPacket>& listPacket, CPacket& inPacket);

    //���ͽ�ͼ
    int SendScreen(std::list<CPacket>& listPacket, CPacket& inPacket);

    //����
    int LockMachine(std::list<CPacket>& listPacket, CPacket& inPacket);

    //����
    int UnlockMachine(std::list<CPacket>& listPacket, CPacket& inPacket);

    //��������
    int TestConnect(std::list<CPacket>& listPacket, CPacket& inPacket);
};

