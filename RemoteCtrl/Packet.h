#pragma once
#include"framework.h"
#include"pch.h"

#pragma pack(push)
#pragma pack(1)
//�� [��ͷ2 ������4 ��������2 ������ ��У��2]
class CPacket
{
public:
	CPacket();
	~CPacket();
	CPacket(const CPacket& pack);//�������캯��
	CPacket& operator=(const CPacket& pack);//��ֵ���캯��

	CPacket(const BYTE* pData, size_t& nSize);//��� ��pDataָ����ַ�����ֳɰ����ݴ���ڰ�������
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize);//��� �����ݷ�װ�ɰ������ڰ�������
	
	

	int size();//����С
	const char* Data();//����ת��Ϊ�ַ�����ʽ����

public:
	//WORD:unsiged short(2�ֽ�)		DWORD:unsigned long(4�ֽ�)
	WORD sHead;//��ͷ FEFF   2�ֽ�
	DWORD nLength;//�����ȣ��ӿ������ʼ������У�������   4�ֽ�
	WORD sCmd;//��������  2�ֽ�
	std::string strData;//�����ݶ�
	WORD sSum;//��У��	2�ֽ�
	std::string strOut;//�������ַ�����ʽ
};
#pragma pack(pop)


//�ļ���Ϣ�ṹ��
typedef struct file_info
{
	file_info()
	{
		IsInvalid = false;//Ĭ��Ϊ��Ч�ļ�
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//�Ƿ���Ч
	BOOL IsDirectory;//�ļ����� 0�ļ� 1Ŀ¼
	BOOL HasNext;//�Ƿ��к��� 0û�� 1��
	char szFileName[256];//�ļ���
}FILEINFO, * PFILEINFO;

//���ṹ��
typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//����	������ƶ���˫��
	WORD nButton;//����	������Ҽ�������
	POINT ptXY;//����
}MOUSEEV, * PMOUSEEV;