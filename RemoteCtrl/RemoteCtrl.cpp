// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"

#include "ServerSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;
void Dump(BYTE* pData, size_t nSize)
{
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0))strOut += "\n";
        snprintf(buf,sizeof(buf), "%02X ", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}
/*
更改当前的工作驱动器
int _chdrive(int drive);
drive 是要更改到的驱动器号码，例如，1 代表 A 驱动器，2 代表 B 驱动器 ，以此类推直到26。
如果成功，则返回 0；否则返回 -1。
*/
int MakeDriverInfo()
{
    std::string result;

    // 遍历 A 到 Z 的驱动器
    for (int i = 1; i <= 26; i++) {
        char driveLetter = 'A' + i - 1;
        std::string drivePath = std::string(1, driveLetter) + ":\\";

        // 尝试将当前工作目录更改为当前驱动器
        if (SetCurrentDirectoryA(drivePath.c_str())) {//判断有哪些分区
            if (!result.empty()) {
                result += ',';
            }
            result += driveLetter;//将分区保存
        }
    }

    CPacket pack(1,(BYTE*)result.c_str(), result.size());
    Dump((BYTE*)pack.Data(), pack.size());
    //CServerSocket::getInstance()->Send(pack);
    return 0;
  
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {

            //// TODO: 在此处为应用程序的行为编写代码。
            //CServerSocket* pserver =  CServerSocket::getInstance();
            //int count = 0;
            //if (!pserver->InitSocket())
            //{
            //    MessageBox(NULL, TEXT("网络初始化异常，未能成功初始化，请检查网络状态"), TEXT("网络初始化失败"), MB_OK | MB_ICONERROR);
            //    exit(0);
            //};
            //while (CServerSocket::getInstance()) {
            //    if (!pserver->AcceptClient())
            //    {
            //        if (count >= 3)
            //        {
            //            MessageBox(NULL, TEXT("多次无法正常接入用户，自动结束程序"), TEXT("客户端连接失败"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, TEXT("无法正常接入用户，自动重试"), TEXT("客户端连接失败"), MB_OK | MB_ICONERROR);
            //        count++;
            //    }
            //    int ret = pserver->DealCommand();
            //    //TODO:处理命令
            //}
            int nCmd = 1;
            switch (nCmd)
            {
            case 1:
                MakeDriverInfo();//查看磁盘分区
                break;
            }
            

        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
