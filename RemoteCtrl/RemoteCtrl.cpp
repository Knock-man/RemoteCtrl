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


class COverlapped {
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator;
    char m_buffer[4096];
    COverlapped() {
        m_operator = 0;
        memset(m_buffer, 0, sizeof(m_overlapped));
        memset(&m_overlapped, 0, sizeof(m_overlapped));
    }
};

/*
void iocp()
{
    //创建重叠结构套接字(一定非阻塞)
    SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    SOCKET client = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET)
    {
        CEdoyunTool::ShowError();
        return;
    }
    sockaddr_in addr;
    addr.sin_family = PF_INET;
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    addr.sin_port = htons(9527);
    bind(sock, (sockaddr*)&addr, sizeof(addr));
    listen(sock, 5);

    
    //绑定iocp和sock
    HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, sock, 4);
    CreateIoCompletionPort((HANDLE)sock, hIOCP, 0, 0);
    
    //创建重叠结构
    COverlapped overlapped;
    overlapped.m_operator = 1;
    DWORD received = 0;
    if (!AcceptEx(sock, client, overlapped.m_buffer, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &received, &overlapped.m_overlapped))
    {
        CEdoyunTool::ShowError();
    }

    overlapped.m_operator = 2;
    WSASend();
    overlapped.m_operator = 3;
    WSARecv();

    while (true)//代表一个线程
    {
        //监听事件发生
        LPOVERLAPPED pOverlapped = NULL;
        DWORD trasferred = 0;
        DWORD CompletionKey = 0;
        if (GetQueuedCompletionStatus(hIOCP, &trasferred, &CompletionKey, &pOverlapped, INFINITY))
        {
            //发生事件对象
            COverlapped* pO = CONTAINING_RECORD(pOverlapped, COverlapped, m_overlapped);
            switch (pO->m_operator)
            {
            case 1:
                //处理accept的操作
            }
            
        }
    }
}
*/
int main()
{
    if (!CEdoyunTool::Init()) return 1;
    EdoyunServer server;
    server.StartService();
    getchar();
    /*
    if (!CEdoyunTool::IsAdmin())//管理员用户  TODO:这里条件取反 为了测试方便避免提权操作
    {
        if (!CEdoyunTool::Init()) return 1;
        //MessageBox(NULL, TEXT("管理员"), TEXT("用户状态"), 0);
        if (ChooseAutoInvoke())//设置开机自启
        {
            CCommand cmdObject;//业务处理对象
            //运行服务器
            int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmdObject);//执行命令，传入业务层回调函数
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
        //MessageBox(NULL, TEXT("普通用户"), TEXT("用户状态"), 0);
    }*/
    return 0;
}
