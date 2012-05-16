//业务处理 server   map 相关

#ifndef MAPSVC_H
#define MAPSVC_H

#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"
#include "ArchvPK.h"
#include "ArchvRole.h"

class MainSvc;
class ArchvRoute;

class MapSvc
{
public:
	MapSvc(void* service, ConnectionPool * cp);
	~MapSvc();

 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);

	int AdjustCoordinateX( UInt32 X, UInt32 originX );

	int AdjustCoordinateY( UInt32 Y, UInt32 originY );

	//------------S_C 交易-----------------------------------------
	
	//------------C_S 交易-----------------------------------------
	//[MsgType:0201]进入场景
	void ProcessEnterMap(Session& session,Packet& packet);

	//[MsgType:0202]移动
	void  ProcessMapMove(Session& session,Packet& packet);

	//[MsgType:0203] (使用飞行符文)瞬移
	void  ProcessTeleportByRune(Session& session,Packet& packet);

	void  NotifyEnterMap(UInt32 RoleID,UInt32 MapID,UInt16 X,UInt16 Y);

	void NotifyTeamLeader(UInt32 roleID,UInt32 leaderID);

	void NotifyActRuneListOnLogin(UInt32 roleID,List<RoleRune>& lic);

	//------------子业务处理 -----------------------------------------

private:
	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;
	
};


#endif

