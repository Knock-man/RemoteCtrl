#include "pch.h"
#include "Command.h"

//主要将<cmd,处理函数>存入map中
CCommand::CCommand() :threadid(0)
{
	struct {
		int nCmd;//操作命令
		CMDFUNC func;//函数指针
	}data[] = {
		{1,&CCommand::MakeDriverInfo},//查看磁盘分区
		{2,&CCommand::MakeDirectoryInfo},//查看目录下的所有文件
		{3,&CCommand::RunFile},//运行文件
		{4,&CCommand::DownloadFile},//下载文件
		{5,&CCommand::MouseEvent},//鼠标事件
		{6,&CCommand::SendScreen},//发送截图
		{7,&CCommand::LockMachine},//锁机
		{8,&CCommand::UnlockMachine},//解锁
		{9,&CCommand::DeleteLocalFile},//删除文件
		{1981,&CCommand::TestConnect},//测试
		{-1,NULL},
	};

	//<操作,函数指针>  存入map
	for (int i = 0; data[i].nCmd != -1; i++)
	{
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}

}

//执行函数 网络层回调    调用成员函数ExcuteCommand（）
void CCommand::RunCommand(void* cmdObject, int nCmd, CPacket& inPacket, std::list<CPacket>& listPacket)//listPacket钩子
{
    CCommand* thiz = (CCommand*)cmdObject;
    if (nCmd > 0)
    {
        int ret = thiz->ExcuteCommand(nCmd, inPacket,listPacket);//执行函数
        if (ret != 0)
        {
            TRACE("执行命令失败：%d ret = %d\r\n", nCmd, ret);
        }
    }
    else
    {
        MessageBox(NULL, TEXT("无法正常接入用户，自动重试"), TEXT("客户端连接失败"), MB_OK | MB_ICONERROR);
    }

}

//执行成员函数，调用命令对应的函数
int CCommand::ExcuteCommand(int nCmd, CPacket& inPacket,std::list<CPacket>& listPacket)
{
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it != m_mapFunction.end())
	{
		return (this->*it->second)(listPacket, inPacket);//执行这个函数
	}
	return 0;
};

//查询磁盘分区
int CCommand::MakeDriverInfo(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    std::string result;//储存所有存在的盘符

    // 遍历 A 到 Z 的驱动器
    for (int i = 1; i <= 26; i++) {
        char driveLetter = 'A' + i - 1;//盘符
        std::string drivePath = std::string(1, driveLetter) + ":\\";

        // 尝试将当前工作目录更改为当前驱动器
        if (SetCurrentDirectoryA(drivePath.c_str())) {//判断有哪些分区,更改成功说明盘符存在
            if (!result.empty()) {
                result += ',';
            }
            result += driveLetter;//将分区保存
        }
    }
    listPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
    return 0;
}


/*遍历目录三步
    struct _finddata_t c_file;//文件结构体
    long hFile; //文件句柄
    if ((hFile = _findfirst("C:\\path\\to\\directory\\*.txt", &c_file)) != -1L) {
          do {
              printf("%s\n", c_file.name);
          } while (_findnext(hFile, &c_file) == 0);
          _findclose(hFile);
     }

*/
//查询指定目录下的所有文件
int CCommand::MakeDirectoryInfo(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    std::string strpath = inPacket.strData;
    //设置为当前工作目录
    if (!SetCurrentDirectoryA(strpath.c_str()))
    {
        //设置失败，
        FILEINFO finfo;
        finfo.HasNext = FALSE;//没有后续文件
        listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        OutputDebugString(TEXT("没有权限访问目录"));
        return -2;
    }

    //设置当前工作目录成功
    _finddata_t c_file; //文件结构体
    int hFile = 0;//文件句柄
    if ((hFile = _findfirst("*", &c_file)) == -1)//找工作目录中匹配的第一个文件  第一个参数使用通配符代表文件类型
    {
        
        //发送结束文件
        FILEINFO finfo;//文件信息结构体
        finfo.HasNext = FALSE;//设置结束文件标记
        listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        OutputDebugString(TEXT("没有找到任何文件"));
        return -3;
    }
    //int Count = 0;//文件数目

    //挨个发送有效文件给客户端
    do
    {
        FILEINFO finfo;
        finfo.IsDirectory = ((c_file.attrib & _A_SUBDIR) != 0);
        memcpy(finfo.szFileName, c_file.name, strlen(c_file.name));
        listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));      
       // Count++;  
        //TRACE("[服务器]发送文件名[%s]\r\n", finfo.szFileName);     
    } while (!_findnext(hFile, &c_file));//查找工作目录匹配的下一个文件

    //发送结束文件
    FILEINFO finfo;
    finfo.HasNext = false;//设置是最后一个文件
    listPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
    //TRACE("Count=%d\r\n", Count);
    return 0;
}

//运行文件指定文件
int CCommand::RunFile(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    std::string strPath = inPacket.strData;
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);//类似双击文件 运行
    listPacket.push_back(CPacket(3, NULL, 0));
    return 0;
}

//打开下载文件（上传给客户端）
int CCommand::DownloadFile(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    //打开文件
    std::string strPath = inPacket.strData;
    long long dataSize = 0;
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    if (err != 0)//文件打开失败
    {
        listPacket.push_back(CPacket(4, (BYTE*)&dataSize, 8));
        return -1;
    }

    //文件打开成功
    if (pFile != NULL)
    {
        //向客户端发送文件总大小
        fseek(pFile, 0, SEEK_END);//pFile指针移到文件末尾，偏移量为0
        dataSize = _ftelli64(pFile);//获取文件指针当前位置    表文件大小
        listPacket.push_back(CPacket(4, (BYTE*)&dataSize, 8));


        fseek(pFile, 0, SEEK_SET);//pFile指针移到文件开头，偏移量为0
        //1KB 1KB向客户端发送
        char buffer[1024] = "";
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);//每次读取1024个char字符  pFIle指针会向后移动
            listPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
        } while (rlen >= 1024);//只有读取数据量大于缓冲区大小时才会没有读完
        fclose(pFile);
    }
    //发送结束标记
    listPacket.push_back(CPacket(4, NULL, 0));
    return 0;

}

int CCommand::DeleteLocalFile(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    std::string strPath = inPacket.strData;
    DeleteFileA(strPath.c_str());

    //发送结束标记
    listPacket.push_back(CPacket(9, NULL, 0));
    return 0;
}

//鼠标事件
int CCommand::MouseEvent(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    MOUSEEV mouse;
    memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));

    DWORD nFlags = 0;
    switch (mouse.nButton)////左键、右键、滚轮
    {
    case 0://左键             0000 0001
        nFlags = 1;
        break;
    case 1://右键             0000 0010
        nFlags = 2; 
        break;
    case 2://滚轮             0000 0100
        nFlags = 4;
        break;
    case 4://没有按键         0000 1000
        nFlags = 8;
        break;
    }
    if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);//移动光标到指定屏幕指定位置

    //动作    单击、移动、双击
    switch (mouse.nAction)
    {
    case 0://单击             0001 0000
        nFlags |= 0x10; 
        break;
    case 1://双击             0010 0000
        nFlags |= 0x20;
        break;
    case 2://按下             0100 0000
        nFlags |= 0x40;
        break;
    case 3://放开             1000 0000
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

    //发送结束标志
    listPacket.push_back(CPacket(5, NULL, 0));
    return 0;
}

//发送截图
int CCommand::SendScreen(std::list<CPacket>& listPacket, CPacket& inPacket)
{
   
    HDC hScreen = ::GetDC(NULL);//获取屏幕上下文
    /*
        HDC：是一个句柄（Handle），代表一个设备上下文。设备上下文是一个Windows数据结构，它包含了绘制图形所需的信息，
        如设备类型、绘图颜色、线条和填充样式等。
        
        ::GetDC：用于获取指定窗口（或整个屏幕）的设备上下文
    */

    //获取屏幕上下文的颜色位数 宽和高
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//每个像素的颜色位数(颜色深度)
    /*
    nBitPerPixel=32;
    意味着每个像素使用32位来表示颜色
    通常包括24位用于红、绿、蓝（RGB）颜色通道（每个通道8位）
    以及额外的8位可能用于alpha通道（透明度）或其他目的（尽管在屏幕上显示时，这额外的8位通常不被使用，因为屏幕不支持透明度）。
    */
    //TRACE("像素的位数:%d", nBitPerPixel);
    int nWidth = GetDeviceCaps(hScreen, HORZRES);//屏幕宽
    int nHeight = GetDeviceCaps(hScreen, VERTRES);//屏幕高

    //创建一个图像对象(依据屏幕上下文宽高颜色位数)  将屏幕的位图复制到图像对象中的位图中
    CImage screen;
    screen.Create(nWidth, nHeight, nBitPerPixel);
    BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);//将hScreen图像复制到screen图像中
    /*
          
          从一个设备上下文（DC）的指定区域复制位图到一个目标设备上下文的指定区域
          BOOL BitBlt(  
          HDC     hdcDest,          // 目标设备上下文句柄  
          int     nXDest,           // 目标矩形左上角的X坐标  
          int     nYDest,           // 目标矩形左上角的Y坐标  
          int     nWidth,           // 矩形区域的宽度  
          int     nHeight,          // 矩形区域的高度  
          HDC     hdcSource,        // 源设备上下文句柄  
          int     nXSrc,            // 源矩形左上角的X坐标  
          int     nYSrc,            // 源矩形左上角的Y坐标  
          DWORD   rop               // 光栅操作码  
            );
    
    */
    
    ReleaseDC(NULL, hScreen);//释放屏幕上下文

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//分配一个可移动的内存块
    if (hMem == NULL)return -1;
    IStream* pStream = NULL;//声明一个指向IStream接口的指针
    HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);//基于内存块创建一个内存流，pStream指向流对象
    if (ret == S_OK)
    {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);//将图片保存到内存流中(流指针会移动)
        LARGE_INTEGER bg = { 0 };//偏移量
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);//将流指针移到流的起始位置
        PBYTE pData = (PBYTE)GlobalLock(hMem);//锁定内存块，转化为字节型指针，获取内存块的起始地址
        //Globallock函数用于锁定内存中指定的内存块，并返回一个地址值，该地址值指向内存块的起始处。
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
    dlg.ShowWindow(SW_SHOW);//显示对话框

    //设置遮蔽框大小
    CRect rect;//声明矩形区域
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);//获得屏幕的宽度
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN) + 70;//获得屏幕的高度
    dlg.MoveWindow(rect);//移动调整对话框的大小(和窗口一样大)

    //设置文字位置大小
    CWnd* pText = dlg.GetDlgItem(IDC_STATIC);//获取文字控件指针   IDC_STATIC是控件ID
    if (pText)
    {
        CRect rtText;
        pText->GetWindowRect(rtText);//提前文字控件坐标矩形
        int nWindth = rtText.Width();//修改大小
        int x = (rect.right - nWindth) / 2;
        int nHeight = rtText.Height();
        int y = (rect.bottom - nHeight) / 2;
        pText->MoveWindow(x, y, rtText.Width(), rtText.Height());//文字控件移动到对话框中间
    }

    //窗口置于顶层
    dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    //隐藏鼠标
    ShowCursor(false);
    //隐藏任务栏
    ::ShowWindow(::FindWindow(TEXT("shell_TrayWnd"), NULL), SW_HIDE);

    //限制鼠标活动范围
    dlg.GetWindowRect(rect);
    rect.left = 0;//限制鼠标只能在左上角一个像素点移动
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
        if (msg.message == WM_KEYDOWN)//键盘按下
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
    PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0);//向线程发送消息
    listPacket.push_back(CPacket(8, NULL, 0));
    return 0;
}

int CCommand::TestConnect(std::list<CPacket>& listPacket, CPacket& inPacket)
{
    listPacket.push_back(CPacket(1981, NULL, 0));
    return 0;
}
