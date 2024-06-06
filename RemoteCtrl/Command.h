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
    //ִ�к��� �����ص�    ���ó�Ա����ExcuteCommand����
    static void RunCommand(void* cmdObject, int nCmd, std::list<CPacket>& listPacket, CPacket& inPacket);//listPacket����

    int ExcuteCommand(int nCmd, std::list<CPacket>& listPacket, CPacket& inPacket);//ӳ��ִ�к��� key=cmd value=ִ�к���

    

protected:
    typedef int(CCommand::* CMDFUNC)(std::list<CPacket>& listPacket, CPacket& inPacket);//����ָ��
    std::map<int, CMDFUNC> m_mapFunction;//������ŵ����ܵ�ӳ��
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

    int TestConnect(std::list<CPacket>& listPacket, CPacket& inPacket);

};

