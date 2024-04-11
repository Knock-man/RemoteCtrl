// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include<list>
#include<stdio.h>
#include<atlimage.h>

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

//查询磁盘分区
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
    //Dump((BYTE*)pack.Data(), pack.size());
    CServerSocket::getInstance()->Send(pack);
    return 0;
  
}

//文件信息结构体
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
//查询指定目录下的所有文件
int MakeDirectoryInfo()
{
    std::string strpath;
    //std::list<FILEINFO> lstFileInfos;
    if (!CServerSocket::getInstance()->GetFilePath(strpath))
    {
        OutputDebugString(TEXT("当前的命令，不是获取文件目录列表，命令解析错误!!"));
        return -1;
    }

    //设置为当前工作目录
    if (!SetCurrentDirectoryA(strpath.c_str()))
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

//运行文件指定文件
int RunFile()
{
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL , NULL, SW_SHOWNORMAL);//类似双击文件

    //发送回应ACK
    CPacket pack(3, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    return 0;

}

//打开下载文件（上传给客户端）
int DownloadFile()
{
    //打开文件
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    long long data = 0;
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile,strPath.c_str(), "rb");
    if (err!=0)
    {
        CPacket pack(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }

    if (pFile != NULL)
    {
        //向客户端发送文件总大小
        fseek(pFile, 0, SEEK_END);//pFile指针移到文件末尾，偏移量为0
        data = _ftelli64(pFile);//获取文件当前位置
        CPacket head(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(head);
        fseek(pFile, 0, SEEK_SET);//pFile指针移到文件开头，偏移量为0

        //1KB 1KB向客户端发送
        char buffer[1024] = "";
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);//每次读取1024个char字符  pFIle指针会向后移动
            CPacket pack(4, (BYTE*)buffer, rlen);
            CServerSocket::getInstance()->Send(pack);
        } while (rlen >= 1024);
        fclose(pFile);
    }
    //发送结束标记
    CPacket pack(4, NULL, 0);
    CServerSocket::getInstance()->Send(pack);
    
}
//鼠标事件
int MouseEvent()
{
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse))
    {
        SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
        DWORD nFlags = 0;
        switch (mouse.nButton)
        {
        case 0://左键
            nFlags = 1;
            break;
        case 1://右键
            nFlags = 2;
            break;
        case 2://中键
            nFlags = 4;
            break;
        case 4://没有按键
            nFlags = 8;
            break;
        }
        if(nFlags!=8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
        switch (mouse.nAction)
        {
        case 0://单击
            nFlags |= 0x10;
            break;
        case 1://双击
            nFlags |= 0x20;
            break;
        case 2://按下
            nFlags |= 0x40;
            break;
        case 3://放开
            nFlags |= 0x80;
            break;
        default:
            break;
        }

        switch (nFlags)
        {
        case 0x21://左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            //没有break会接着执行一次单击
        case 0x11://左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41://左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81://左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22://右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12://右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        
        case 0x42://右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82://右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x24://中建双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14://中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        
        case 0x44://中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84://中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x08://单纯鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
    }
    else {
        OutputDebugString(TEXT("获取鼠标操作参数失败！！"));
        return -1;
    }
    return 0;
}

#include <atlimage.h>
int SendScreen()
{
    
    //屏幕截图
    HDC hScreen = ::GetDC(NULL);//获取屏幕上下文句柄（屏幕截图）
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//RGB位深度 R8位 G8位 B8位 共24位
    int nWidth = GetDeviceCaps(hScreen, HORZRES);//宽
    int nHeight = GetDeviceCaps(hScreen, VERTRES);

    //创建一个图像对象
    CImage screen;
    screen.Create(nWidth, nHeight, nBitPerPixel);

    BitBlt(screen.GetDC(), 0, 0, 1920, 1020, hScreen, 0, 0, SRCCOPY);//将hScreen图像复制到screen图像中

    //删除屏幕截图
    ReleaseDC(NULL, hScreen);

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//分配一个可移动的内存块
    if (hMem == NULL)return -1;
    IStream* pStream = NULL;//声明一个指向IStream接口的指针
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//基于内存块创建一个内存流，pStream指向流对象
    if (ret == S_OK)
    {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);//将图片保存到内存流中
        LARGE_INTEGER bg = { 0 };
        pStream->Seek(bg,STREAM_SEEK_SET, NULL);//将流指针移到流的起始位置
        PBYTE pData = (PBYTE)GlobalLock(hMem);//锁定内存块，转化为字节型指针，获取内存块的起始地址
        SIZE_T nSize = GlobalSize(hMem);//获取分配内存块大小
        CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);
        GlobalUnlock(hMem);//内存块解锁
    }
    

    /*
    //测试PNG和JPEGCPU损耗对比
    DWORD tick = GetTickCount64();
    screen.Save(TEXT("test2020.png"), Gdiplus::ImageFormatPNG);
    TRACE("pnd %d\n", GetTickCount64() - tick);
    tick = GetTickCount64();
    screen.Save(TEXT("test2020.jpg"), Gdiplus::ImageFormatJPEG);
    TRACE("JPEG %d\n", GetTickCount64() - tick);
    */
    pStream->Release();//释放流
    GlobalFree(hMem);//释放内存块
    screen.ReleaseDC();
    
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
            int nCmd = 6;
            switch (nCmd)
            {
            case 1://查看磁盘分区
                MakeDriverInfo();
                break;
            case 2://查看指定目录下的所有文件
                MakeDirectoryInfo();
                break;
            case 3://打开文件
                RunFile();
                break;
            case 4://下载文件
                DownloadFile();
                break;
            case 5://鼠标操作
                MouseEvent();
                break;
            case 6://发送屏幕内容=>发送屏幕的截图
                SendScreen();
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
