#ifndef PETSVC_H
#define PETSVC_H
#include "GWProxy.h"
#include "ConnectionPool.h"

#include "Serializer.h"
#include "../Avatar/ArchvAvatar.h"
#include "ArchvPK.h"
#include "ArchvBagItemCell.h"
#include "ArchvPet.h"
#include "ArchvRole.h"
#include "Pet.h"



#ifndef PET_MAX_LEVEL
#define PET_MAX_LEVEL 70   //宠物最大等级
#endif

#ifndef PET_MAX_NUM 
#define PET_MAX_NUM 6
#endif


class PetSvc
{
public:
	//构造函数	
	PetSvc(void *sever,ConnectionPool *cp);
	
	//析构函数
	~PetSvc();

	void OnProcessPacket(Session& session,Packet& packet);
	//处理包
	void ClientErrorAck(Session& session, Packet& packet, UInt32 RetCode);
	
	void ProcessPacket(Session& session, Packet& packet);

	//客户端错误应答s
	void ClientErrorAck(Session& session, Packet& packet);


	//========================== c-s ack ===================================

	//[msgtype:1002] 查询宠物详细信息;
	void ProcessPetDetailInfo(Session& session, Packet& packet);

	//[mystype:1005]宠物卸下装备 
	void ProcessPetEquipGetOff(Session& session, Packet& packet);

	//[mystype:1006] 宠物穿上装备
	void ProcessPetEquipPutOn(Session& session, Packet& packet);

	//宠物合成 ，返回0 成功， 非0 失败
	int OnPetComposite(UInt32 &roleID, UInt32 &petType);
	
	//宠物丢弃，返回 0 成功，非 0 失败
	int OnPetAbandon(UInt32 &roleID,UInt32 &petID);
	
	
	//加成减小
	void GetPetBonusoff(UInt32 petID,List<ArchvroleBonuschange>& Bon,Pet& pet);

	//加成增加
	void GetPetBonusin(UInt32 petID,List<ArchvroleBonuschange>& Bon,Pet& pet);

	//加成计算
	void PetBonusFunc(UInt32 petID,UInt32 BonusAttrID,Int32 num,Pet& pet);

	//获得加成值
	UInt32 PetGetNewBonus( Pet& pet, UInt32 l);

	void GetPetItemBonus(UInt32 ItemID,List<ArchvroleBonuschange>& Bon);

	//========================== c-s 宠物旅馆ack ===================================

	//[msgtype:2302]激活当前宠物
	void ProcessPetIsUse(Session& session, Packet& packet);



	//============================ S-C  Ack =========================
	
	//[MsgType:1001]宠物基本属性变更
	void NotifyPetAttrChange(UInt32 RoleID);

	//[MsgType:1002]宠物加成属性变更
	void NotifyPetBonus(UInt32 RoleID,List<ArchvroleBonuschange> &k);

	//[MsgType:1003]宠物装备改变
	void NotifyPetEquipChange(UInt32 RoleID,List<ItemCell>& lic);

	//[MsgType:1004]宠物技能变更
	void NotifyPetSkillChange(UInt32 RoleID,List<ArchvSkill>&lskill);

	//[MsgType:1006]宠物激活
	void NotifyPetOut(UInt32 &RoleID,PetbrifInfo& petinfo);


    // 暂时定为:MsgType[1007]
	/////////////////////////增加经验通知，消息类型待定
	void NotifyPetAddExp(UInt32 &roleID,UInt32 petID,UInt32 &curExp);

	//============================ S-C  宠物旅馆Ack =========================

	//[MsgType:2101]宠物的获得
	void NotifyGetPet(UInt32 &roleID,PetBrief petBrief);

private:
	
	MainSvc * _mainSvc;
	//IniFile _file;
	ConnectionPool *_cp;
};


#endif

