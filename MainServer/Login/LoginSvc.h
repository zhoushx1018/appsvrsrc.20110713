//业务处理 server   Login 相关

#ifndef LOGINSVC_H
#define LOGINSVC_H

#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"
#include "ArchvPK.h"

class MainSvc;
class ArchvRoute;

class LoginSvc
{
public:
	LoginSvc(void* service, ConnectionPool * cp);
	~LoginSvc();

 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);

	int AdjustCoordinateX( UInt32 X, UInt32 originX );

	int AdjustCoordinateY( UInt32 Y, UInt32 originY );

	//------------S_C 交易-----------------------------------------


	//------------C_S 交易-----------------------------------------
	//msgtype 101 角色登陆 
	void ProcessRoleLogin(Session& session,Packet& packet);

	//msgtype 104 角色登出
	void ProcessRoleLogout(Session& session,Packet& packet);

	

	//msgtype 104 角色登出
	
	//------------子业务处理 -----------------------------------------
	Byte GetIfIsAdult(UInt32 roleID,UInt32& TopTime,UInt32& LastloginTime);//返回0未成年人，返回1成年人

	//s-c
	void NotifyCtAdultLogin(UInt32 roleID,Byte IsAdult,Byte Flag);

private:
	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;
	
};


#endif

