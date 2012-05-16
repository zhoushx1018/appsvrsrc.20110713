//业务处理 server  avatar相关

#ifndef AVATARSVC_H
#define AVATARSVC_H

#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"
#include "LuaState.h"
#include "ArchvAvatar.h"
#include "ArchvPK.h"
#include "ArchvBagItemCell.h"
#include "Role.h"




class MainSvc;
class AvatarSvc
{
public:
	AvatarSvc(void* service, ConnectionPool * cp);
	~AvatarSvc();
 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);

	int CheckEquipType( int input );


	//------------S_C 交易-----------------------------------------
	//msgtype 501 Avatar更新，通知其他人
	void NotifyAvatarUpdate( UInt32 roleID ,UInt32 wepID,UInt32 coatID);
	//msgtype502 装备更新通知
	void NotifyEquipUpdate(UInt32 RoleID,List<ItemCell>& lic);
	void NotifyBonus(UInt32 RoleID,List<ArchvroleBonuschange> &k);

	void NotifySingleBonus(UInt32 RoleID, UInt16 bonusAttrID, UInt32 num);


	//------------C_S 交易-----------------------------------------

	void  ProcessGetRolesAvatar(Session& session,Packet& packet);//msgtype 501 获取多个角色 avatar


	void  ProcessUpdateAvatar(Session& session,Packet& packet);//msgtype 502 更新 avatar


	void  ProcessGetRolesBriefAvatar(Session& session,Packet& packet);//msgtype 503 获取多个角色简要 avatar


	void  ProcessGetRolesEquipPos(Session& session,Packet& packet);//msgtype 504 装备能放位置提示


	void  ProcessRolesEquipGetoff(Session& session,Packet& packet);//506 穿着的装备脱下


	void ProcessTestByWangLian(Session& session,Packet& packet);  //纯属个人测试，不作他用，删除对其他代码毫无影响

	//------------子业务处理 -----------------------------------------

	void GetRoleSkill(UInt32 roleID,List<ArchvSkill>& sk);//装备简要信息,返回0，成功，返回-1失败

	int GetEquipBrief(List<UInt32>& listRoleID , List<ArchvAvatarDescBrief>& ladb);

	//int GetItemBonus(UInt32 RoleID,List<UInt32>& l,List<ArchvroleBonuschange>& Bon,List<ArchvroleEuip>& Ebon);//判断是否需要遍历，查出身上是否具有另外加成的装备，返回1，需要，返回0不需要

	int IfneedToGet(UInt32 RoleID,List<UInt32>& l);


	int GetJustItemBonus(UInt32 ItemID,List<ArchvroleBonuschange>& Bon,List<UInt32>& l);//角色脱下装备，除了特殊属性之后的值

	void GetRoleBonusoff(UInt32 RoleID,List<ArchvroleBonuschange>& Bon);//角色穿上装备，除了特殊方式的值

	void GetRoleBonusin(UInt32 RoleID,List<ArchvroleBonuschange>& Bon);//角色的属性值的对应函数函数

	void RoleBonus(UInt32 RoleID,UInt32 BonusAttrID,Int32 num);//角色百分百加成，或者其他方式

	void RoleOtherBonus(UInt32 RoleID,List<UInt32>& L);

	UInt32 RoleGetNewBonus(UInt32 RoleID,UInt32 l,RolePtr& role);

	UInt32 RoleGetinfo(UInt32 RoleID,UInt32 l);

	Int32 Updatebind(UInt32 RoleID,UInt32 EntityID);

	UInt32 UseItem(UInt32 roleID,UInt32 ItemID,UInt32 EntityID,UInt32 EquipPos,UInt16 celIndex);


    //装备耐久度损耗
	int OnEquipDurabilityLoss(UInt32 roleID,UInt32 lossDurability);


//	Int32 CheckifhavaGet(UInt32 l,List <ArchvroleBonuschange>& bon);
	//Int32 GetBonusList(UInt32 RoleID,UInt32 l,List<ArchvroleBonuschange>& bon);
//private:


private:
	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;

	LuaState _luaState;

};


#endif

