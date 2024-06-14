// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//


#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "Command.h"



#include "ServerSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

//设置开启自启
/*
开启启动的时候，程序的权限是跟随启动用户的
如果两者权限不一致，则会导致程序启动失败
开机启动对环境变量有影响，如果依赖dll(动态库)，则可能启动失败
解决方法：
方法一【复制这些dll到system32下面或者syswow64下面】system32下面，多是64为位程序 SysWOW64下面多是32位程序
方法二【使用静态库，而非动态库】
*/


//修改注册表开机启动(登录过程中启动) 
// 第一步：可执行文件创建软连接到系统变量文件下  第二步：将此软连接添加到开机自启注册表中
int WriteRefisterTable(const CString strPath){
         if (PathFileExists(strPath))return 0;//注册表中已经存在

        //创建软链接 到系统目录下面 C:\\Windows\\SysWOW64
        char sPath[MAX_PATH] = "";
        char sSys[MAX_PATH] = "";
        std::string strExe = "\\RemoteCtrl.exe ";
        GetCurrentDirectoryA(MAX_PATH, sPath);//当前路径
        GetSystemDirectoryA(sSys, sizeof(sSys));//系统路径
        std::string strCmd = "mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;
        system(strCmd.c_str());//执行命令

        //打开注册表开启自启位置
        CString strSubKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";//注册表开机自启路径
        HKEY hKey = NULL;
        int ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
        if (ret != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            MessageBox(NULL, TEXT("打开注册表失败! 是否权限不足?\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
            return -1;
        }

        //将可执行文件软连接添加到注册表开启自启路径下
        ret = RegSetValueEx(hKey, TEXT("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
        if (ret != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            MessageBox(NULL, TEXT("注册表添加文件失败 是否权限不足?\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
            return -2;

        }
        RegCloseKey(hKey);
        return 0;
}

//写入自启动文件方式
int WriteStartupDir(const CString& strPath)
{
    CString strCmd = GetCommandLine();
    strCmd.Replace(TEXT("\""), TEXT(""));
    BOOL ret = CopyFile(strCmd,strPath, FALSE);
    if (ret == FALSE)
    {
        MessageBox(NULL, TEXT("复制文件夹失败，是否权限不足?\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
        return -1;
    }
    return 0;
}

//选择开机启动方式
void ChooseAutoInvoke(){
    CString strInfo = TEXT("启动远程监控！\n");
    strInfo += TEXT("继续运行该程序，将使得这台机器处于被监控状态!\n");
    strInfo += TEXT("按下“取消”按钮，退出程序!\n");
    strInfo += TEXT("按下“是”按钮，设置程序开机自启!\n");
    strInfo += TEXT("按下“否”按钮，程序仅运行一次!\n");
    int ret = MessageBox(NULL, strInfo, TEXT("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES)
    {
        //系统路径
        CString SystemPath = CString(TEXT("C:\\Windows\\SysWOW64\\RemoteCtrl.exe"));//注意这里32位(SysWOW64)和64位(system32)默认系统变量位置不停同
        //开机自启文件路径
        CString StartupPath = TEXT("C:\\Users\\xbj\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe");

        if (!WriteStartupDir(StartupPath))//尝试写入开机启动文件  设置开机自启
        {
        }
        else if (!WriteRefisterTable(SystemPath))//尝试写入注册表自启动 设置开机自启
        {
        }
        else
        {
            MessageBox(NULL, TEXT("设置开机自启动失败?\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
        }
    }
    else if (ret == IDCANCEL)
    {
        exit(0);
    }
    return;
}
int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误400
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            CCommand cmdObject;//业务处理对象
            ChooseAutoInvoke();
            //获得服务器实例
            CServerSocket* pserver =  CServerSocket::getInstance(); 
            //运行服务器
            int ret = pserver->Run(&CCommand::RunCommand,&cmdObject);//执行命令，传入业务层回调函数
            switch (ret)
            {
            case -1:
                MessageBox(NULL, TEXT("网络初始化异常，未能成功初始化，请检查网络状态"), TEXT("网络初始化失败"), MB_OK | MB_ICONERROR);
                exit(0);
                break;
            case -2:
                MessageBox(NULL, TEXT("多次无法正常接入用户，自动结束程序"), TEXT("客户端连接失败"), MB_OK | MB_ICONERROR);
                exit(0);
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
