#include"pch.h"
#include "Packet.h"
//包类

//构造
CPacket::CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0)
{

}
CPacket::CPacket(const CPacket& pack)
{
	sHead = pack.sHead;
	nLength = pack.nLength;
	sCmd = pack.sCmd;
	strData = pack.strData;
	sSum = pack.sSum;
}
CPacket& CPacket::operator=(const CPacket& pack)
{
	if (this != &pack)
	{
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	return *this;

}
CPacket::~CPacket()
{

}
// 解析包  拆包
CPacket::CPacket(const BYTE* pData, size_t& nSize)
{
	//包 [包头2 包长度4 控制命令2 包数据2 和校验2]
	size_t i = 0;
	//取包头位
	for (; i < nSize; i++)
	{
		if ((*(WORD*)(pData + i)) == 0xFEFF)//找到包头
		{
			sHead = *(WORD*)(pData + i);
			i += 2;
			break;
		}
	}

	if ((i + 4 + 2 + 2) > nSize)//包数据不全 只有 [包头 包长度 控制命令 和校验]  没有数据段 解析失败
	{
		nSize = 0;
		return;
	}

	//取包长度位
	nLength = *(DWORD*)(pData + i); i += 4;
	if (nLength + i > nSize)//包未完全接收到 nLength+sizeof(包头)+sizeof(包长度) pData缓冲区越界了
	{
		nSize = 0;
		return;
	}

	//取出控制命令位
	sCmd = *(WORD*)(pData + i); i += 2;

	//保存数据段
	if (nLength > 4)
	{
		strData.resize(nLength - 2 - 2);//nLength - [控制命令位长度] - [校验位长度]
		memcpy((void*)strData.c_str(), pData + i, nLength - 4);
		i = i + nLength - 2 - 2;
	}

	//取出校验位 并校验
	sSum = *(WORD*)(pData + i); i += 2;
	WORD sum = 0;
	for (size_t j = 0; j < strData.size(); j++)
	{
		sum += BYTE(strData[j]) & 0xFF;//只取字符低八位
	}
	//TRACE("[客户端] sHead=%d nLength=%d data=[%s]  sSum=%d  sum = %d\r\n", sHead, nLength, strData.c_str(), sSum, sum);
	if (sum == sSum)
	{
		nSize = i;
		return;
	}
	nSize = 0;
}
//打包：封装成包
CPacket::CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
{
	sHead = 0xFEFF;
	nLength = nSize + 4;
	sCmd = nCmd;


	if (nSize > 0)//有数据段
	{
		//打包数据段
		strData.resize(nSize);
		memcpy((void*)strData.c_str(), pData, nSize);
	}
	else//无数据段
	{
		strData.clear();
	}

	//打包检验位
	sSum = 0;
	for (size_t j = 0; j < strData.size(); j++)
	{
		sSum += BYTE(strData[j]) & 0xFF;//只取字符低八位
	}
	TRACE("[服务器] sHead=%d nLength=%d data=[%s]  sSum=%d\r\n", sHead, nLength, strData.c_str(), sSum);
}


int CPacket::size()
{
	return nLength + 6;
}

//将包转为字符串类型
const char* CPacket::Data()
{
	strOut.resize(nLength + 6);
	BYTE* pData = (BYTE*)strOut.c_str();
	*(WORD*)pData = sHead;
	*(DWORD*)(pData + 2) = nLength;
	*(WORD*)(pData + 2 + 4) = sCmd;
	memcpy(pData + 2 + 4 + 2, strData.c_str(), strData.size());
	*(WORD*)(pData + 2 + 4 + 2 + strData.size()) = sSum;
	return strOut.c_str();
}