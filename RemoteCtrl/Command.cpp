#include "pch.h"
#include "Command.h"

CCommand::CCommand() :threadid(0)
{
	struct {
		int nCmd;
		CMDFUNC func;
	}data[] = {
		{1,&CCommand::MakeDriverInfo},
		{2,&CCommand::MakeDirectoryInfo},
		{3,&CCommand::RunFile},
		{4,&CCommand::DownloadFile},
		{5,&CCommand::MouseEvent},
		{6,&CCommand::SendScreen},
		{7,&CCommand::LockMachine},
		{8,&CCommand::UnlockMachine},
		{9,&CCommand::DeleteLocalFile},
		{1981,&CCommand::TestConnect},
		{-1,NULL},
	};

	//存入map
	for (int i = 0; data[i].nCmd != -1; i++)
	{
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));

	}

}
int CCommand::ExcuteCommand(int nCmd, std::list<CPacket>& listPacket, CPacket& inPacket)
{
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it != m_mapFunction.end())
	{
		return (this->*it->second)(listPacket, inPacket);//执行这个函数
	}
	return 0;
};

//执行函数
void CCommand::RunCommand(void* cmdObject, int status, std::list<CPacket>& listPacket, CPacket& inPacket)//listPacket钩子
{
	CCommand* thiz = (CCommand*)cmdObject;
	if (status > 0)
	{
		int ret = thiz->ExcuteCommand(status, listPacket, inPacket);//执行函数
		if (ret != 0)
		{
			TRACE("执行命令失败：%d ret = %d\r\n", status, ret);
		}
	}
	else
	{
		MessageBox(NULL, TEXT("无法正常接入用户，自动重试"), TEXT("客户端连接失败"), MB_OK | MB_ICONERROR);
	}

}


//查询磁盘分区
int CCommand::MakeDriverInfo(std::list<CPacket>& listPacket, CPacket& inPacket)
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
    listPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
    return 0;
}



//查询指定目录下的所有文件
int CCommand::MakeDirectoryInfo(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    std::string strpath = inPacket.strDate;
    //设置为当前工作目录
    if (!SetCurrentDirectoryA(strpath.c_str()))
    {
        //设置失败
        FILEINFO finfo;
        finfo.HasNext = FALSE;//没有后续文件
        listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        OutputDebugString(TEXT("没有权限访问目录"));
        return -2;
    }

    //设置当前工作目录成功
    _finddata_t fdata; // 声明变量 fdata
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1)//找工作目录中匹配的第一个文件  第一个参数使用通配符代表文件类型
    {
        OutputDebugString(TEXT("没有找到任何文件"));
        //发送结束文件
        FILEINFO finfo;
        finfo.HasNext = false;//设置结束文件标记
        listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));

        return -3;
    }
    int Count = 0;
    //挨个发送有效文件给客户端
    do
    {
        FILEINFO finfo;
        finfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));

        listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        //TRACE("[服务器]发送文件=%s\r\n", finfo.szFileName);
        Count++;
        //TRACE("[服务器数据包：\r\n]");
        //Dump((BYTE*)pack.Data(), pack.size());

        //TRACE("[服务器]发送文件名[%s]\r\n", finfo.szFileName);

        //lstFileInfos.push_back(finfo);
    } while (!_findnext(hfind, &fdata));//查找工作目录匹配的下一个文件

    //发送结束文件
    FILEINFO finfo;
    finfo.HasNext = false;//设置结束文件标记

    listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
    TRACE("Count=%d\r\n", Count);
    return 0;
}

//运行文件指定文件
int CCommand::RunFile(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    std::string strPath = inPacket.strDate;
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//类似双击文件

    listPacket.push_back(CPacket(3, NULL, 0));
    return 0;
}

//打开下载文件（上传给客户端）
int CCommand::DownloadFile(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    //打开文件
    std::string strPath = inPacket.strDate;
    long long data = 0;
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    if (err != 0)
    {
        listPacket.push_back(CPacket(4, (BYTE*)&data, 8));
        return -1;
    }

    if (pFile != NULL)
    {
        //向客户端发送文件总大小
        fseek(pFile, 0, SEEK_END);//pFile指针移到文件末尾，偏移量为0
        data = _ftelli64(pFile);//获取文件当前位置
        listPacket.push_back(CPacket(4, (BYTE*)&data, 8));
        fseek(pFile, 0, SEEK_SET);//pFile指针移到文件开头，偏移量为0

        //1KB 1KB向客户端发送
        char buffer[1024] = "";
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);//每次读取1024个char字符  pFIle指针会向后移动
            listPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
        } while (rlen >= 1024);
        fclose(pFile);
    }
    //发送结束标记
    listPacket.push_back(CPacket(4, NULL, 0));
    return 0;

}

int CCommand::DeleteLocalFile(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    std::string strPath = inPacket.strDate;
    //TCHAR sPath[MAX_PATH] = TEXT("");
    //MultiByteToWideChar(CP_ACP, 0, strPath.c_str(),
    //    strPath.size(),(LPWSTR)sPath, sizeof(sPath) / sizeof(TCHAR));
    DeleteFileA(strPath.c_str());

    //发送结束标记
    listPacket.push_back(CPacket(9, NULL, 0));
    return 0;
}
//鼠标事件
int CCommand::MouseEvent(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    MOUSEEV mouse;
    memcpy(&mouse, inPacket.strDate.c_str(), sizeof(MOUSEEV));
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
    if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
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
    listPacket.push_back(CPacket(5, NULL, 0));

    return 0;
}

int CCommand::SendScreen(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    //屏幕截图
    HDC hScreen = ::GetDC(NULL);//获取屏幕上下文句柄（屏幕截图）
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//RGB位深度 R8位 G8位 B8位 共24位
    int nWidth = GetDeviceCaps(hScreen, HORZRES);//宽
    int nHeight = GetDeviceCaps(hScreen, VERTRES);

    //创建一个图像对象
    CImage screen;
    screen.Create(nWidth, nHeight, nBitPerPixel);

    BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);//将hScreen图像复制到screen图像中

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
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);//将流指针移到流的起始位置
        PBYTE pData = (PBYTE)GlobalLock(hMem);//锁定内存块，转化为字节型指针，获取内存块的起始地址
        SIZE_T nSize = GlobalSize(hMem);//获取分配内存块大小
        listPacket.push_back(CPacket(6, pData, nSize));
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



//锁机
int CCommand::LockMachine(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE))
    {
        //开辟线程打开遮蔽框
        _beginthreadex(NULL, 0, CCommand::threadLockDlg, this, 0, &threadid);

    }
    //发送应答消息
    listPacket.push_back(CPacket(7, NULL, 0));
    return 0;
}
//遮蔽框线程函数
unsigned __stdcall CCommand::threadLockDlg(void* arg)
{
    CCommand* thiz = (CCommand*)arg;
    thiz->threadLockDlgMain();
    _endthreadex(0);
    return 0;
}
void CCommand::threadLockDlgMain()
{
    //锁机弹出对话框
    dlg.Create(IDD_DIALOG_INFO, NULL);//创建对话框
    dlg.ShowWindow(SW_SHOW);

    //限制鼠标活动范围
    CRect rect;//声明矩形区域
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);//获得屏幕像素大小
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN) + 70;
    dlg.MoveWindow(rect);//将窗口移到矩形区域
    CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
    if (pText)
    {
        CRect rtText;
        pText->GetWindowRect(rtText);
        int nWindth = rtText.Width();
        int x = (rect.right - nWindth) / 2;
        int nHeight = rtText.Height();
        int y = (rect.bottom - nHeight) / 2;
        pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
    }
    //窗口置于顶层
    dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    //隐藏鼠标
    ShowCursor(false);
    //隐藏任务栏
    ::ShowWindow(::FindWindow(TEXT("shell_TrayWnd"), NULL), SW_HIDE);
    //限制鼠标活动范围
    dlg.GetWindowRect(rect);
    //限制鼠标只能在左上角一个像素点移动
    rect.left = 0;
    rect.top = 0;
    rect.right = 1;
    rect.bottom = 1;
    ClipCursor(rect);//鼠标活动矩形区域

    //消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        //按按键退出消息循环
        if (msg.message == WM_KEYDOWN)
        {
            if (msg.wParam == 0x1B)break;//按Esc键退出
        }
    }
    ClipCursor(NULL);//恢复鼠标范围
    //恢复任务栏
    ::ShowWindow(::FindWindow(TEXT("shell_TrayWnd"), NULL), SW_SHOW);
    //恢复鼠标
    ShowCursor(true);
    dlg.DestroyWindow();

}
//解锁
int CCommand::UnlockMachine(std::list<CPacket>& listPacket, CPacket& inPacket) {

    //向指定线程发送消息Esc按键消息
    PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0);
    listPacket.push_back(CPacket(8, NULL, 0));
    return 0;
}

int CCommand::TestConnect(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    listPacket.push_back(CPacket(1981, NULL, 0));
    return 0;
}
