#pragma once
#include<map>
#include<atlimage.h>
#include<direct.h>
#include<list>
#include<stdio.h>

#include <io.h> // 包含用于文件查找的头文件
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
    int ExcuteCommand(int nCmd, std::list<CPacket>& listPacket, CPacket& inPacket);//映射执行函数 key=cmd value=执行函数

    //执行函数
    static void RunCommand(void* cmdObject, int status, std::list<CPacket>& listPacket, CPacket& inPacket);//listPacket钩子
protected:
    typedef int(CCommand::* CMDFUNC)(std::list<CPacket>& listPacket, CPacket& inPacket);//成员函数指针
    std::map<int, CMDFUNC> m_mapFunction;//从命令号到功能的映射
    CLockDialog dlg;
    unsigned threadid;

private:

    //遮蔽框线程函数
    static unsigned __stdcall threadLockDlg(void* arg);
    void threadLockDlgMain();


    //查询磁盘分区
    int MakeDriverInfo(std::list<CPacket>& listPacket, CPacket& inPacket);

    //查询指定目录下的所有文件
    int MakeDirectoryInfo(std::list<CPacket>& listPacket, CPacket& inPacket);
    //运行文件指定文件
    int RunFile(std::list<CPacket>& listPacket, CPacket& inPacket);
    //打开下载文件（上传给客户端）
    int DownloadFile(std::list<CPacket>& listPacket, CPacket& inPacket);

    int DeleteLocalFile(std::list<CPacket>& listPacket, CPacket& inPacket);
    //鼠标事件
    int MouseEvent(std::list<CPacket>& listPacket, CPacket& inPacket);

    int SendScreen(std::list<CPacket>& listPacket, CPacket& inPacket);



    //锁机
    int LockMachine(std::list<CPacket>& listPacket, CPacket& inPacket);
    //解锁
    int UnlockMachine(std::list<CPacket>& listPacket, CPacket& inPacket);
    int TestConnect(std::list<CPacket>& listPacket, CPacket& inPacket);

};

