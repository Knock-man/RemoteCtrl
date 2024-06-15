#pragma once
class CEdoyunTool
{
public:
    static void Dump(BYTE* pData, size_t nSize)
    {
        std::string strOut;
        for (size_t i = 0; i < nSize; i++)
        {
            char buf[8] = "";
            if (i > 0 && (i % 16 == 0))strOut += "\n";
            snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
            strOut += buf;
        }
        strOut += "\n";
        OutputDebugStringA(strOut.c_str());
    }


    //��ӡ��ǰ�߳����һ��������Ϣ
    static void ShowError()
    {
        LPCSTR lpMessageBuf = NULL;
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&lpMessageBuf, 0, NULL);
        OutputDebugString(lpMessageBuf);
        MessageBox(NULL, lpMessageBuf, TEXT("�������"), MB_OK | MB_ICONERROR);
        LocalFree((HLOCAL)lpMessageBuf);
    }

    //�жϵ������Ƿ���Ȩ
    static bool IsAdmin() {
        //�õ���ǰ���̵����� Token
        HANDLE hToken = NULL;//����
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            ShowError();
            return false;
        }

        //��ȡ��Ȩ��Ϣ
        TOKEN_ELEVATION eve;
        DWORD len = 0;
        if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE)
        {
            ShowError();
            return false;
        }
        CloseHandle(hToken);

        //�ж��Ƿ���Ȩ
        if (len == sizeof(eve))
        {
            return eve.TokenIsElevated;//>0 ��Ȩ  ==0 û����Ȩ
        }
        printf("length of tokeninformation is %d\r\n", len);
        return false;

    }

 //��Ȩ
/*
������
���ذ�ȫ���ԡ����ز��ԡ���ȫѡ��
�˻�������Ա�˻�״̬  ����
�˻���ʹ�ÿ�����ı����˻�ֻ������п���̨��¼ ����
*/
    static bool RunAsAdmin() {
        // ��ȡ��ǰ����·��  
        WCHAR sPath[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, sPath, MAX_PATH); // ��ȡ��ǰ��ִ���ļ�·��  

        // ����������Ϣ  
        STARTUPINFOW si = { 0 };
        si.cb = sizeof(STARTUPINFOW);
        PROCESS_INFORMATION pi = { 0 };

        BOOL ret = CreateProcessWithLogonW((LPCWSTR)"Administrator", NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
        //MessageBox(NULL, (LPCSTR)sPath, TEXT("�û�״̬"), 0);
        if (!ret) {
            MessageBox(NULL, TEXT("��������ʧ��"), TEXT("�������"), MB_OK | MB_ICONERROR);
            ShowError();
            return false;
        }

        MessageBox(NULL, TEXT("���̴����ɹ�"), TEXT("�û�״̬"), 0);

        // �ȴ��������  
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

 //���ÿ�������
/*
����������ʱ�򣬳����Ȩ���Ǹ��������û���
�������Ȩ�޲�һ�£���ᵼ�³�������ʧ��
���������Ի���������Ӱ�죬�������dll(��̬��)�����������ʧ��
���������
����һ��������Щdll��system32�������syswow64���桿system32���棬����64Ϊλ���� SysWOW64�������32λ����
��������ʹ�þ�̬�⣬���Ƕ�̬�⡿
*/


//���ÿ����������޸�ע���ʽ(��¼����������) 
//��������ע���λ�ã������\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Run
    static int WriteRefisterTable(const CString SystemPath) {
        if (PathFileExists(SystemPath))return 0;//ע������Ѿ�����

        //��ִ���ļ����Ƶ�ϵͳ�����ļ���
        char exePath[MAX_PATH] = "";//��ǰ��ִ���ļ�·��
        GetModuleFileName(NULL,exePath, MAX_PATH);
        bool ret = CopyFile(exePath, SystemPath, FALSE);
        if (ret == FALSE)
        {
            MessageBox(NULL, TEXT("�����ļ���ʧ�ܣ��Ƿ�Ȩ�޲���?\r\n"), TEXT("����"), MB_ICONERROR | MB_TOPMOST);
            return -1;
        }

        //��ע���������λ��
        CString strSubKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";//ע���������·��
        HKEY hKey = NULL;
         ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
        if (ret != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            MessageBox(NULL, TEXT("��ע���ʧ��! �Ƿ�Ȩ�޲���?\r\n"), TEXT("����"), MB_ICONERROR | MB_TOPMOST);
            return -2;
        }

        //����ִ���ļ���������ӵ�ע���������·����
        ret = RegSetValueEx(hKey, TEXT("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)SystemPath, SystemPath.GetLength() * sizeof(TCHAR));
        if (ret != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            MessageBox(NULL, TEXT("ע�������ļ�ʧ�� �Ƿ�Ȩ�޲���?\r\n"), TEXT("����"), MB_ICONERROR | MB_TOPMOST);
            return -3;
        }
        RegCloseKey(hKey);
        return 0;
    }

    //���ÿ���������д���������ļ��з�ʽ���û���¼֮��������
    //�����ļ��д򿪷�ʽ��win+R ����:shell:startup
    static int WriteStartupDir(const CString& strPath)
    {
        TCHAR strCmd[MAX_PATH] = TEXT("");
        GetModuleFileName(NULL, strCmd, MAX_PATH);//��ȡ��ִ���ļ�·��

        BOOL ret = CopyFile(strCmd, strPath, FALSE);//��ִ���ļ��ŵ����������ļ�����
        if (ret == FALSE)
        {
            MessageBox(NULL, TEXT("�����ļ���ʧ�ܣ��Ƿ�Ȩ�޲���?\r\n"), TEXT("����"), MB_ICONERROR | MB_TOPMOST);
            return -1;
        }
        return 0;
    }

    static bool Init()
    {
        //ͨ�ã�����MFL��������Ŀ��ʼ��
        HMODULE hModule = ::GetModuleHandle(nullptr);
        if (hModule == nullptr)
        {
            // TODO: ���Ĵ�������Է�����Ҫ
            wprintf(L"����: GetModuleHandle ʧ��\n");
            return false;
        }

        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: �ڴ˴�ΪӦ�ó������Ϊ��д���롣
            wprintf(L"����: MFC ��ʼ��ʧ��\n");
            return false;
        }
        return true;
    }
};

