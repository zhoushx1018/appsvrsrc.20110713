//业务处理 server   RoleInfo 相关

#ifndef ROLEINFOSVC_H
#define ROLEINFOSVC_H

#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"
#include "ArchvPK.h"
#include "Role.h"

class MainSvc;
class NewTimer;

class RoleInfoSvc
{
public:
	RoleInfoSvc(void* service, ConnectionPool * cp);
	~RoleInfoSvc();

 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);

	int AdjustCoordinateX( UInt32 X, UInt32 originX );

	int AdjustCoordinateY( UInt32 Y, UInt32 originY );



	static void HandleEvent1( void * obj, void * arg, int argLen);
	//------------S_C 交易----------------------------------------
	void NotifySKill(UInt32 roleID,List<ArchvSkill>& sk);
	//通知技能变更
	void Notifyinfo(UInt32 roleID);

	void Notifyinfo(UInt32 roleID,RolePtr& role);
	//通知人物基本属性变更

	//[306]通知当前的经验
	void NotifyExp(UInt32 roleID);

	//------------C_S 交易-----------------------------------------
	//[MsgType:0301]查询角色属性fromDB (仅适合登陆后未进入地图使用;因为后台是从数据库中取数据)
	void	ProcessGetRoleInfoFromDB(Session& session,Packet& packet);

	//msgtype 302 查询角色简要属性
	void ProcessGetRoleBriefInfo(Session& session,Packet& packet);


	//msgtype 303技能查询
	void ProcessGetRoleSkill(Session& session,Packet& packet);

	//msgtype 304 技能加点
	void ProcessRoleSkillAdd(Session& session,Packet& packet);

	void RoleExpAddToDB(UInt32 roleID,int num);

	//[MsgType:0305] 查询角色属性from缓存 (适合登陆后且已进入地图使用;后台是从缓存中取数据)
	void	ProcessGetRoleInfoFromCache(Session& session,Packet& packet);
	
	Byte GetIfIsAdult(UInt32 roleID);
	//------------子业务处理 -----------------------------------------
	void NotifyCtAdult(UInt32 roleID,Byte Type);

	void NotifyRolePoint(UInt32 roleID,UInt32 point); 
		
    void GetSkill( UInt32 RoleID, List<ArchvSkill>& SK);

private:
	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;

	std::list<NewTimer*> _timerList;
	
};


#endif

