#include "framework.h"
#include "pch.h"
#include <string>

#pragma pack(push)
#pragma pack(1)
//包 [包头2 包长度4 控制命令2 包数据 和校验2]
class CPacket
{
public:
	CPacket();
	~CPacket();
	CPacket(const BYTE* pData, size_t& nSize);//解包
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize);//打包
	CPacket(const CPacket& pack);//拷贝构造
	CPacket& operator=(const CPacket& pack);//赋值构造

	int size();//包大小
	const char* CPacket::Data(std::string& strOut) const;//包

public:
	//WORD:unsiged short(2字节)		DWORD:unsigned long(4字节)
	WORD sHead;//包头 FEFF  2B
	DWORD nLength;//包长度（从控制命令开始，到和校验结束）		4B
	WORD sCmd;//控制命令	2B
	std::string strData;//包数据
	WORD sSum;//和校验	2B
};

#pragma pack(pop)
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
}FILEINFO, * PFILEINFO;


//是否自动关闭
enum {
	CSM_AUTOCLOSE = 1,//CSM = Client SOCKET Mode 自动关闭模式
};

//包数据结构体 用在消息发送
typedef struct PacketData {
	std::string strData;//数据
	UINT nMode;//模式
	WPARAM AttParam;
	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nAttParam = 0)
	{
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);//深拷贝
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


//鼠标结构体
typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;//动作
		nButton = -1;//按钮
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//动作 点击、移动、双击
	WORD nButton;//按钮 左键、右键、滚轮
	POINT ptXY;//坐标
}MOUSEEV, * PMOUSEEV;