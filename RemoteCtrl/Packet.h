/* 
#包类#
作用：用在网络的数据传输
格式：[0xFEFF|包长度|控制命令|数据|检验位]     
长度：[  2B  |  4B  |   2B   |data| 2B   ]

设计：包长度 = 控制命令长度 + 数据长度 + 检验位长度
	  检验位 = 数据段每个字符的低八位的和
	  控制命令 = 代表此包进行的操作 例如 1查看分区 2查看文件 1981测试包

	  接收方接收到包时，根据包头识别数据流中包的起始位置，根据控制命令进行相应的操作，根据检验位判断接收数据是否错误,
*/
#pragma once
#include"framework.h"
#include"pch.h"

#pragma pack(push)
#pragma pack(1)
//包 [包头2 包长度4 控制命令2 包数据 和校验2]
class CPacket
{
public:
	CPacket();
	~CPacket();
	CPacket(const CPacket& pack);//拷贝构造函数
	CPacket& operator=(const CPacket& pack);//赋值构造函数

	CPacket(const BYTE* pData, size_t& nSize);//解包 将pData指向的字符串拆分成包数据存放在包对象中
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize);//打包 将数据封装成包储存在包对象中
	
	

	int size();//包大小
	const char* Data();//将包转换为字符串格式返回

public:
	//WORD:unsiged short(2字节)		DWORD:unsigned long(4字节)
	WORD sHead;//包头 FEFF   2字节
	DWORD nLength;//包长度（从控制命令开始，到和校验结束）   4字节
	WORD sCmd;//控制命令  2字节
	std::string strData;//包数据段
	WORD sSum;//和校验	2字节
	std::string strOut;//整个包字符串形式
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

//鼠标结构体
typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//按键	点击、移动、双击
	WORD nButton;//动作	左键、右键、滚轮
	POINT ptXY;//坐标
}MOUSEEV, * PMOUSEEV;