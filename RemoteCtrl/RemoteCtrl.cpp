// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//


#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "Command.h"
#include <string>
#include "ServerSocket.h"
#include <conio.h>
#include "CEdoyunQueue.h"
#include "EdoyunServer.h"
#include <MSWSock.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//系统变量路径
#define SYSTEMPATH TEXT("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")
//开机自启文件路径
#define STARTUPPATH TEXT("C:\\Users\\xbj\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

// 唯一的应用程序对象
CWinApp theApp;
//选择开机启动方式
bool ChooseAutoInvoke() {
    CString strInfo = TEXT("启动远程监控！\n");
    strInfo += TEXT("继续运行该程序，将使得这台机器处于被监控状态!\n");
    strInfo += TEXT("按下“取消”按钮，退出程序!\n");
    strInfo += TEXT("按下“是”按钮，设置程序开机自启!\n");
    strInfo += TEXT("按下“否”按钮，程序仅运行一次!\n");
    int ret = MessageBox(NULL, strInfo, TEXT("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES)
    {
        if (!CEdoyunTool::WriteStartupDir(STARTUPPATH)) {}//尝试方式一设置开机自启 写入开机启动文件  
        else if (!CEdoyunTool::WriteRefisterTable(SYSTEMPATH)) {}//尝试方式二设置开机自启 写入注册表自启动 
        else
        {
            MessageBox(NULL, TEXT("设置开机自启动失败\r\n"), TEXT("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
    }
    else if (ret == IDCANCEL) return false;
    return true;
}


int main()
{
    //if (!CEdoyunTool::Init()) return 1;
    //EdoyunServer server;
    //server.StartService();
    //getchar();
    
    if (!CEdoyunTool::IsAdmin())//管理员用户  TODO:这里条件取反 为了测试方便避免提权操作(也就是普通用户会进入此逻辑)
    {
        if (!CEdoyunTool::Init()) return 1;//MFC命令行项目初始化
        //MessageBox(NULL, TEXT("管理员"), TEXT("用户状态"), 0);
        if (ChooseAutoInvoke())//设置开机自启
        {
            CCommand cmdObject;//业务处理对象
            //运行服务器
            int ret = CServerSocket::getInstance()->Run(&cmdObject ,&CCommand::RunCommand);//执行命令，传入业务层回调函数
            switch (ret)
            {
            case -1:
                MessageBox(NULL, TEXT("网络初始化异常，未能成功初始化，请检查网络状态"), TEXT("网络初始化失败"), MB_OK | MB_ICONERROR);
                break;
            case -2:
                MessageBox(NULL, TEXT("多次无法正常接入用户，自动结束程序"), TEXT("客户端连接失败"), MB_OK | MB_ICONERROR);
                break;
            }
        }
    }
    else//普通用户   需要提权创建新进程
    {
        if (CEdoyunTool::RunAsAdmin() == false) return 1;
        MessageBox(NULL, TEXT("普通用户"), TEXT("用户状态"), 0);
    }
    return 0;
}
