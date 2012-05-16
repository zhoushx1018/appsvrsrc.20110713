//业务处理 server   Npc相关

#ifndef NPCSVC_H
#define NPCSVC_H

#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"
#include "LuaState.h"

class MainSvc;
 
class NpcSvc
{
public:
	NpcSvc(void* service, ConnectionPool * cp);
	~NpcSvc();

 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);

	int GetNpcStatus( UInt32 roleID, UInt32 level, UInt32 npcID, Byte& status );

	
	//------------S_C 交易-----------------------------------------


	//------------C_S 交易-----------------------------------------
	//msgtype 601 Npc对话 
	void ProcessNpcTalk(Session& session,Packet& packet);

	//msgtype 602 Npc商店物品查询
	void ProcessGetNpcShopItem(Session& session,Packet& packet);


	//------------子业务处理 -----------------------------------------
private:
	int ExecCanAcceptScript( UInt32 roleID, const string& script );

private:
	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;

	LuaState _luaState;
	
};


#endif

