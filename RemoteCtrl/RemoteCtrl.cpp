// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include<list>
#include<stdio.h>

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

//查询分区
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

typedef struct file_info
{
    file_info()
    {
        IsInvalid = false;//默认为有效文件
        IsDirectory = -1;
        HasNext = TRUE;
        memset(szFileName, 0, sizeof(szFileName));
    }
    BOOL IsInvalid;//是否有效
    BOOL IsDirectory;//文件类型 0文件 1目录
    BOOL HasNext;//是否还有后续 0没有 1有
    char szFileName[256];//文件名
}FILEINFO,*PFILEINFO;

#include <io.h> // 包含用于文件查找的头文件
int MakeDirectoryInfo()
{
    std::string strpath;
    //std::list<FILEINFO> lstFileInfos;
    if (!CServerSocket::getInstance()->GetFilePath(strpath))
    {
        OutputDebugString(TEXT("当前的命令，不是获取文件目录列表，命令解析错误!!"));
        return -1;
    }
    if (!SetCurrentDirectoryA(strpath.c_str()))//设置为当前工作目录
    {
        //设置失败
        FILEINFO finfo;
        finfo.IsInvalid = true;//无效文件
        finfo.IsDirectory = true;//为目录
        finfo.HasNext = FALSE;//没有后续文件
        memcpy(finfo.szFileName, strpath.c_str(), strpath.size());//文件路径
        //lstFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));//打包
        CServerSocket::getInstance()->Send(pack);//发送
        OutputDebugString(TEXT("没有权限访问目录"));
        return -2;
    }
    //设置当前工作目录成功
    _finddata_t fdata; // 声明变量 fdata
    int hfind = 0;
    if ((hfind =_findfirst("*", &fdata)) == -1)//找工作目录中匹配的第一个文件  第一个参数使用通配符代表文件类型
    {
        OutputDebugString(TEXT("没有找到任何文件"));
        return -3;
    }
    //发送有效文件给客户端
    do
    {
        FILEINFO finfo;
        finfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));//打包
        CServerSocket::getInstance()->Send(pack);//发送
        //lstFileInfos.push_back(finfo);
    } while (!_findnext(hfind, &fdata));//查找工作目录匹配的下一个文件
    
    //发送结束文件
    FILEINFO finfo;
    finfo.HasNext = false;//设置结束文件标记
    CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));//打包
    CServerSocket::getInstance()->Send(pack);//发送
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
            case 1://查看磁盘分区
                MakeDriverInfo();
                break;
            case 2://查看指定目录下的文件
                MakeDirectoryInfo();
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
