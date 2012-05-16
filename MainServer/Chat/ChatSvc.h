//业务处理 server   背包相关
#ifndef CHATSVC_H
#define CHATSVC_H

#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"


class MainSvc;
 
class ChatSvc
{
public:
	ChatSvc(void* service, ConnectionPool * cp);
	~ChatSvc();
	void SetService( MainSvc * svc );

 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);



	//===================主业务====================================
	
	void GetTeamRoleIDs(UInt32 teamID,UInt32 roleID,list<UInt32>& itor);//不包括自己
	
	void ProcessChat(Session& session,Packet& packet);//msgtype 901 聊天
	
    void NotifyChat(Byte Falg,UInt32 roleID,List<UInt32> lrid,string& Message);
	
	//===================子业务=======================================

	
	//---------------------s-c的广播----------------------------------


private:
	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;

};


#endif

