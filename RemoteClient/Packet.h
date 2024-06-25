#include "framework.h"
#include "pch.h"
#include <string>

#pragma pack(push)
#pragma pack(1)
//�� [��ͷ2 ������4 ��������2 ������ ��У��2]
class CPacket
{
public:
	CPacket();
	~CPacket();
	CPacket(const BYTE* pData, size_t& nSize);//���
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize);//���
	CPacket(const CPacket& pack);//��������
	CPacket& operator=(const CPacket& pack);//��ֵ����

	int size();//����С
	const char* CPacket::Data(std::string& strOut) const;//��

public:
	//WORD:unsiged short(2�ֽ�)		DWORD:unsigned long(4�ֽ�)
	WORD sHead;//��ͷ FEFF  2B
	DWORD nLength;//�����ȣ��ӿ������ʼ������У�������		4B
	WORD sCmd;//��������	2B
	std::string strData;//������
	WORD sSum;//��У��	2B
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


//�Ƿ��Զ��ر�
enum {
	CSM_AUTOCLOSE = 1,//CSM = Client SOCKET Mode �Զ��ر�ģʽ
};

//�����ݽṹ�� ������Ϣ����
typedef struct PacketData {
	std::string strData;//����
	UINT nMode;//ģʽ
	WPARAM AttParam;
	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nAttParam = 0)
	{
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);//���
		nMode = mode;
		AttParam = nAttParam;
	}
	PacketData(const PacketData& data)
	{
		strData = data.strData;
		nMode = data.nMode;
		AttParam = data.AttParam;
	}
	PacketData& operator=(const PacketData& data)
	{
		if (this != &data) {
			strData = data.strData;
			nMode = data.nMode;
			AttParam = data.AttParam;
		}
		return *this;
	}
}PACKET_DATA;


//���ṹ��
typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;//����
		nButton = -1;//��ť
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//���� ������ƶ���˫��
	WORD nButton;//��ť ������Ҽ�������
	POINT ptXY;//����
}MOUSEEV, * PMOUSEEV;