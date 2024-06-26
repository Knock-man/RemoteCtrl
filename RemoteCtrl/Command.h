/*
业务模块
作用：接收网络层利用回调传来的数据包,根据操作命令，调用相应处理函数进行处理

锁机实现原理：
    1.开启一个线程，线程内创建一个遮蔽框  _beginthreadex()
    2.设置遮蔽框大小为屏幕大小，置顶，位置为屏幕位置   设置CRect大小   dlg.MoveWindow(rect)
    3.限制鼠标活动范围为左上角一个像素，隐藏任务栏，隐藏鼠标
    4.开启while()事件循环 阻塞当前线程  当收到Esc消息时才结束循环  MSG msg ; GetMessage() TranslateMessage() DispatchMessage()
    5.恢复鼠标、任务栏，销毁遮蔽框

查看磁盘分区原理：
    1.切换到 A-Z:\\  目录是否成功 成功存在此根目录  切换当前工作路径API SetCurrentDirectoryA()

查询指定目录下的所有文件原理：
    1.切换查看目录为工作目录  SetCurrentDirectoryA()
    2.long hFile = _findfirst()查看第一个文件
    3._findnext()查看下一个文件

    struct _finddata_t 为文件信息结构体

运行指定文件原理：
    1.使用API ShellExecuteA()  双击效果

下载文件原理：
    1.fopen_s()打开文件
    2.fseek()计算出文件大小 发送给客户端
    3.fread()一次读一个缓冲区的大小数据 发送给客户端 直到所有数据发送完毕
    4.fclose()关闭文件

删除文件原理：
    1.使用API DeleteFileA()

鼠标事件原理：
    1.将接收数据转换为MOUSEEV结构数据
    2.不同动作按键采取不同行为  API mouse_event() 响应鼠标事件

截图实现原理:
    1.截图
        (1)获取屏幕上下文DC  HDC hScreen = ::GetDC(NULL);
        (2)依据屏幕上下文DC 创建一个图像对象
                 CImage screen;     
                 screen.Create(nWidth, nHeight, nBitPerPixel);
        (3)将屏幕位图复制到图像对象位图上完成截图  BitBlt()
        (4)释放屏幕上下文

    2.将图像类型CImage保存到string中
        (1)分配一个可移动的内存块  HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
        (2)创建一个内存流 管理内存块
            IStream* pStream = NULL;//内存流指针
            HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//基于内存块创建一个内存流
        (3)使用内存流指针将图像保存到内存块中
            screen.Save(pStream, Gdiplus::ImageFormatPNG);
        (4)将内存流指针移到起始位置
            pStream->Seek(bg, STREAM_SEEK_SET, NULL)
        (5)锁定内存块 获得内存块的地址
            PBYTE pData = (PBYTE)GlobalLock(hMem)//获得内存块地址
            SIZE_T nSize = GlobalSize(hMem);//获取分配内存块大小
        (6)发送内存数据
            listPacket.push_back(CPacket(6, pData, nSize));
        (7)内存块解锁 GlobalUnlock(hMem);
           释放内存块 GlobalFree(hMem);  
           释放内存流 pStream->Release();
           释放图像上下文 screen.ReleaseDC();
*/
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

//业务处理类
class CCommand
{
public:
    //主要将<cmd,处理函数> 存入map中
    CCommand();

    //执行函数 网络层调用的回调函数    调用成员函数cmdObject->ExcuteCommand()
    static void RunCommand(void* cmdObject, int nCmd, CPacket& inPacket,std::list<CPacket>& listPacket);//listPacket钩子

    int ExcuteCommand(int nCmd, CPacket& inPacket,std::list<CPacket>& listPacket);//映射执行函数 key=cmd value=执行函数

    

protected:
    typedef int(CCommand::* CMDFUNC)(std::list<CPacket>& listPacket, CPacket& inPacket);//函数指针
    std::map<int, CMDFUNC> m_mapFunction;//从命令号到处理函数的映射
    CLockDialog dlg;//锁机遮蔽框
    unsigned threadid;//线程ID

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

    //发送截图
    int SendScreen(std::list<CPacket>& listPacket, CPacket& inPacket);

    //锁机
    int LockMachine(std::list<CPacket>& listPacket, CPacket& inPacket);

    //解锁
    int UnlockMachine(std::list<CPacket>& listPacket, CPacket& inPacket);

    //测试连接
    int TestConnect(std::list<CPacket>& listPacket, CPacket& inPacket);
};

