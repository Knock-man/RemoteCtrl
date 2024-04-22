#include"pch.h"
#include "Packet.h"
//����
CPacket::CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0)
{

}
// ������
CPacket::CPacket(const BYTE* pData, size_t& nSize)
{
	//�� [��ͷ2 ������4 ��������2 ������2 ��У��2]
	size_t i = 0;
	//ȡ��ͷλ
	for (; i < nSize; i++)
	{
		if ((*(WORD*)(pData + i)) == 0xFEFF)//�ҵ���ͷ
		{
			sHead = *(WORD*)(pData + i);
			i += 2;
			break;
		}
	}

	if ((i + 4 + 2 + 2) > nSize)//�����ݲ�ȫ ֻ�� [��ͷ ������ �������� ��У��]  û�����ݶ� ����ʧ��
	{
		nSize = 0;
		return;
	}

	//ȡ������λ
	nLength = *(DWORD*)(pData + i); i += 4;
	if (nLength + i > nSize)//��δ��ȫ���յ� nLength+sizeof(��ͷ)+sizeof(������) pData������Խ����
	{
		nSize = 0;
		return;
	}

	//ȡ����������λ
	sCmd = *(WORD*)(pData + i); i += 2;

	//�������ݶ�
	if (nLength > 4)
	{
		strDate.resize(nLength - 2 - 2);//nLength - [��������λ����] - [У��λ����]
		memcpy((void*)strDate.c_str(), pData + i, nLength - 4);
		i = i + nLength - 2 - 2;
	}

	//ȡ��У��λ ��У��
	sSum = *(WORD*)(pData + i); i += 2;
	WORD sum = 0;
	for (size_t j = 0; j < strDate.size(); j++)
	{
		sum += BYTE(strDate[j]) & 0xFF;//ֻȡ�ַ��Ͱ�λ
	}
	//TRACE("[�ͻ���] sHead=%d nLength=%d data=[%s]  sSum=%d  sum = %d\r\n", sHead, nLength, strDate.c_str(), sSum, sum);
	if (sum == sSum)
	{
		nSize = i;
		return;
	}
	nSize = 0;
}
//�������װ�ɰ�
CPacket::CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
{
	sHead = 0xFEFF;
	nLength = nSize + 4;
	sCmd = nCmd;


	if (nSize > 0)//�����ݶ�
	{
		//������ݶ�
		strDate.resize(nSize);
		memcpy((void*)strDate.c_str(), pData, nSize);
	}
	else//�����ݶ�
	{
		strDate.clear();
	}

	//�������λ
	sSum = 0;
	for (size_t j = 0; j < strDate.size(); j++)
	{
		sSum += BYTE(strDate[j]) & 0xFF;//ֻȡ�ַ��Ͱ�λ
	}
	TRACE("[������] sHead=%d nLength=%d data=[%s]  sSum=%d\r\n", sHead, nLength, strDate.c_str(), sSum);
}
CPacket::CPacket(const CPacket& pack)
{
	sHead = pack.sHead;
	nLength = pack.nLength;
	sCmd = pack.sCmd;
	strDate = pack.strDate;
	sSum = pack.sSum;
}
CPacket& CPacket::operator=(const CPacket& pack)
{
	if (this != &pack)
	{
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strDate = pack.strDate;
		sSum = pack.sSum;
	}
	return *this;

}
CPacket::~CPacket()
{

}

int CPacket::size()
{
	return nLength + 6;
}

//����תΪ�ַ�������
const char* CPacket::Data()
{
	strOut.resize(nLength + 6);
	BYTE* pData = (BYTE*)strOut.c_str();
	*(WORD*)pData = sHead;
	*(DWORD*)(pData + 2) = nLength;
	*(WORD*)(pData + 2 + 4) = sCmd;
	memcpy(pData + 2 + 4 + 2, strDate.c_str(), strDate.size());
	*(WORD*)(pData + 2 + 4 + 2 + strDate.size()) = sSum;
	return strOut.c_str();
}