#pragma once
//������
#include <Windows.h>
#include<atlimage.h>
#include <string>
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

	//���ַ������� תΪ CImage����
	/*
		(1)�����ڴ��
		(2)�����ڴ��������ڴ��
		(3)�ַ�������д���ڴ����
		(4)�ڴ���ָ���ƶ�����ͷ
		(5)���ڴ�����ݱ��浽image������
	
	*/
    static int Bytes2Image(CImage& image, const std::string& strBuffer)
    {
		BYTE* pData = (BYTE*)strBuffer.c_str();
		HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0);//�����ڴ��
		if (hMen == NULL)
		{
			TRACE("�ڴ治���ˣ�\r\n");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hMen, TRUE, &pStream);//�����ڴ��������ڴ��
		if (hRet == S_OK)
		{
			ULONG length = 0;
			pStream->Write(pData, strBuffer.size(), &length);//������д���ڴ����
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);//��ָ���Ƶ���ʼλ��
			if ((HBITMAP)image != NULL)  image.Destroy();
			image.Load(pStream);//ͼ�񱣴��ڴ������
		}
		return hRet;
    }
};

