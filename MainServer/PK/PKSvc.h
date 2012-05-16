//业务处理 server   pk相关

#ifndef PKSVC_H
#define PKSVC_H

#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"
#include "ArchvPK.h"
#include "Role.h"



class MainSvc;
class ArchvPosition;
class Role;
class Monster;
class ArchvRolePKInfo;
class ArchvCreatureStatus;

class PKSvc
{
public:
	PKSvc(void* service, ConnectionPool * cp);
	~PKSvc();

 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);

	//------------S_C 交易-----------------------------------------
	
	//msgtype 401 pk结束，通知所有人；
	void NotifyPKEnd( list<PKEndInfo>& lpkei );
	

	//------------C_S 交易-----------------------------------------
	//msgtype 401 请求PK
	void	ProcessPKReq(Session& session,Packet& packet);

	//------------子业务处理 -----------------------------------------
	

private:
	ArchvPosition GetMidpoint( 	const ArchvPosition &pos1, const ArchvPosition &pos2 );
	ArchvPosition CalcPKOrigin( const ArchvPosition &input );
	int AdjustCoordinateX( UInt32 X, UInt32 originX );
	int AdjustCoordinateY( UInt32 Y, UInt32 originY );
	void GetCtStatus( List<ArchvRolePKInfo>& lrpki, List<ArchvCreatureStatus>& lcs );
	int MakeSSPkgForRole( 	const ArchvPosition &posPKOrigin, RolePtr &role, List<ArchvRolePKInfo> &lrpki,Byte type=1);
	int MakeSSPkgForMonster( 	const ArchvPosition &posPKOrigin, Monster &monster, List<ArchvRolePKInfo> &lrpki,Byte num=1);
	int MakeSSPkgForPet(const ArchvPosition &posPKOrigin, UInt32 roleID, List<ArchvRolePKInfo> &lrpki,Byte type=1,Byte dirte=1);
	
private:
	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;
	
};


#endif

