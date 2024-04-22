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
		{7,&CCommand::SendScreen},
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
}
;
