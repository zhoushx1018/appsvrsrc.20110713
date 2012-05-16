/**
 *	PK 序列化用的类
 *
 */

#ifndef ARCHVPK_H
#define ARCHVPK_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"


//技能
class ArchvSkill
	:public BaseArchive
{

public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvSkill()
	:skillID(0),skillLevel(0)
	{}

public:
	//成员变量
	UInt16 skillID;
	Byte skillLevel;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(skillID)
		SERIAL_ENTRY(skillLevel)
	END_SERIAL_MAP()

};


//角色pk信息
class ArchvRolePKInfo
	:public BaseArchive
{

public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvRolePKInfo()
	:controlID(0),roleID(0),level(0),opposition(0),creatureFlag(0),creatureType(0)
	,maxHP(0),maxMP(0),hp(0),mp(0)
	,moveSpeed(0),currPosX(0),currPosY(0),direct(0),attackPowerHigh(0)
	,attackPowerLow(0)
	,attackArea(0),attackSpeed(0),attackDisplayTime(0)
	,attackBulletSpeed(0)
	,CritRate(0)
	,hitRate(0),dodgeRate(0)
	,defense(0),mDefense(0),Strength(0),Agile(0),Wisdom(0)
	{}

public:
	//成员变量
	UInt32 controlID;
	UInt32 roleID;
	UInt16 level;
	Byte opposition;
	Byte creatureFlag;
	UInt32 creatureType;
	UInt16 maxHP;
	UInt16 maxMP;
	UInt16 hp;
	UInt16 mp;
	UInt16 moveSpeed;
	UInt16 currPosX;
	UInt16 currPosY;
	Byte direct;
	UInt16 attackPowerHigh;
	UInt16 attackPowerLow;
	UInt16 attackArea;
	UInt16 attackSpeed;							//攻击速度；单位：次/0.1秒
	UInt16 attackDisplayTime;				//攻击展示时长
	UInt16 attackBulletSpeed;
	UInt16 hitRate;
	UInt16 dodgeRate;
	UInt16 CritRate;
	UInt16 defense;
	UInt16 mDefense;
	UInt16 Strength;//力量
	UInt16 Agile;//敏捷
	UInt16 Wisdom;//智力
	List<ArchvSkill> las;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(controlID)
		SERIAL_ENTRY(roleID)
		SERIAL_ENTRY(level)
		SERIAL_ENTRY(opposition)
		SERIAL_ENTRY(creatureFlag)
		SERIAL_ENTRY(creatureType)
		SERIAL_ENTRY(maxHP)
		SERIAL_ENTRY(maxMP)
		SERIAL_ENTRY(hp)
		SERIAL_ENTRY(mp)
		SERIAL_ENTRY(moveSpeed)
		SERIAL_ENTRY(currPosX)
		SERIAL_ENTRY(currPosY)
		SERIAL_ENTRY(direct)
		SERIAL_ENTRY(attackPowerHigh)
		SERIAL_ENTRY(attackPowerLow)
		SERIAL_ENTRY(attackArea)
		SERIAL_ENTRY(attackSpeed)
		SERIAL_ENTRY(attackDisplayTime)
		SERIAL_ENTRY(attackBulletSpeed)
		SERIAL_ENTRY(hitRate)
		SERIAL_ENTRY(dodgeRate)
		SERIAL_ENTRY(CritRate)
		SERIAL_ENTRY(defense)
		SERIAL_ENTRY(mDefense)
		SERIAL_ENTRY(Strength)
		SERIAL_ENTRY(Agile)
		SERIAL_ENTRY(Wisdom)
		SERIAL_ENTRY(las)
	END_SERIAL_MAP()

};


//pk结束信息
class ArchvPKEndInfo
	:public BaseArchive
{

public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvPKEndInfo()
	:creatureFlag(0),creatureType(0),roleID(0),opposition(0),live(0),hp(0),mp(0)
	{}

public:
	//成员变量
	Byte creatureFlag;
	UInt32 creatureType;
	UInt32 roleID;
	Byte opposition;
	Byte live;
	UInt16 hp;
	UInt16 mp;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(creatureFlag)
		SERIAL_ENTRY(creatureType)
		SERIAL_ENTRY(roleID)
		SERIAL_ENTRY(opposition)
		SERIAL_ENTRY(live)
		SERIAL_ENTRY(hp)
		SERIAL_ENTRY(mp)
	END_SERIAL_MAP()

};




//pk结束信息
class ArchvPKReward
	:public BaseArchive
{

public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvPKReward()
	:pkType(0),glory(0),exp(0),money(0)
	{}

public:
	//成员变量
	Byte pkType;
	UInt32 glory;
	UInt32 exp;
	UInt32 money;
	List<UInt32>   items;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(pkType)
		SERIAL_ENTRY(glory)
		SERIAL_ENTRY(exp)
		SERIAL_ENTRY(money)
		SERIAL_ENTRY(items)
	END_SERIAL_MAP()

};

//pk结束信息
class PKEndInfo
{
public:
	UInt32 roleID;
	ArchvPKReward pkr;
	ArchvPKEndInfo pkei;
};
class PKBriefList
{
public:
	PKBriefList():pkID(0)
	{}
	public:
		UInt32 pkID;
		list<UInt32> player1;
		list<UInt32> player2;//怪物只是存储怪物领头人
};
class PetInfoToPk
{
	public:
	UInt32 PetID;
	UInt32 Hp;
	UInt32 Mp;
	UInt32 Exp;
};
class RoleEquipToPk
{
public:
		UInt32 RoleID;
		Byte type;
};
class RoleInfopk
{
public:
	UInt32 RoleID;
	UInt32 Level;

};

class VipRole
	:public BaseArchive
{
public:
	VipRole():roleID(0),isVip(0)
		{}
public:
	UInt32 roleID;
	Byte   isVip;

	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(roleID)
		SERIAL_ENTRY(isVip)
	END_SERIAL_MAP()
};

#endif

