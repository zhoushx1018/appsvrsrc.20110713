/**
 *	角色(Role) 序列化用的类
 *	
 */

#ifndef ARCHVROLE_H
#define ARCHVROLE_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"
#include "./Pet/ArchvPet.h"


//角色信息
class ArchvRoleInfo
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvRoleInfo():roleId(0),level(0), exp(0), maxExp(0),
		proID(0), guildID(0), glory(0), hp(0),mp(0), maxHp(0), maxMp(0),
		attackPowerHigh(0),attackPowerLow(0),defence(0),mDefence(0),critRate(0),
		crime(0),addPoint(0),strength(0),intelligence(0),agility(0),
		moveSpeed(0), hitRate(0),dodgeRate(0), attackSpeed(0),HPRegen(0),MPRegen(0)
	{}
	
	virtual ~ArchvRoleInfo(){}

public:
	//成员变量
	UInt32 roleId;
	string roleName;
	UInt32 level;
	UInt32 exp;
	UInt32 maxExp;
	UInt32 proID;
	UInt32 guildID;
	UInt32 glory;
	UInt32 hp;
	UInt32 mp;
	UInt32 maxHp;
	UInt32 maxMp;
	UInt32 attackPowerHigh;
	UInt32 attackPowerLow;
	UInt32 defence;
	UInt32 mDefence;
	UInt32 critRate;
	UInt32 crime;
	UInt32 addPoint;
	UInt32 strength;
	UInt32 intelligence;
	UInt32 agility;
	UInt32 lastMapID;
	UInt16 lastX;
	UInt16 lastY;
	UInt32 moveSpeed;
	UInt32 hitRate;
	UInt32 dodgeRate;
	UInt32 attackSpeed;
	UInt32 HPRegen;
	UInt32 MPRegen;
	Byte IsAdult;
	PetbrifInfo petbrif;
	Byte vipLevel;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(roleId)
		SERIAL_ENTRY(roleName)
		SERIAL_ENTRY(level)
		SERIAL_ENTRY(exp)
		SERIAL_ENTRY(maxExp)
		SERIAL_ENTRY(proID)
		SERIAL_ENTRY(guildID)
		SERIAL_ENTRY(glory)
		SERIAL_ENTRY(hp)
		SERIAL_ENTRY(mp)
		SERIAL_ENTRY(maxHp)
		SERIAL_ENTRY(maxMp)
		SERIAL_ENTRY(attackPowerHigh)
		SERIAL_ENTRY(attackPowerLow)
		SERIAL_ENTRY(defence)
		SERIAL_ENTRY(mDefence)
		SERIAL_ENTRY(critRate)
		SERIAL_ENTRY(crime)
		SERIAL_ENTRY(addPoint)
		SERIAL_ENTRY(strength)
		SERIAL_ENTRY(intelligence)
		SERIAL_ENTRY(agility)
		SERIAL_ENTRY(lastMapID)
		SERIAL_ENTRY(lastX)
		SERIAL_ENTRY(lastY)
		SERIAL_ENTRY(moveSpeed)
		SERIAL_ENTRY(hitRate)
		SERIAL_ENTRY(dodgeRate)
		SERIAL_ENTRY(attackSpeed)
		SERIAL_ENTRY(HPRegen)
		SERIAL_ENTRY(MPRegen)
		SERIAL_ENTRY(IsAdult)
		SERIAL_ENTRY(petbrif)
		SERIAL_ENTRY(vipLevel)
	END_SERIAL_MAP()

};
//角色变更属性
class ArchvRoleChange
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvRoleChange():level(0), exp(0), maxExp(0),
		 glory(0), hp(0),mp(0), maxHp(0), maxMp(0),
		attackPowerHigh(0),attackPowerLow(0),defence(0),mDefence(0),critRate(0),
		crime(0),addPoint(0),strength(0),intelligence(0),agility(0),
		moveSpeed(0), hitRate(0),dodgeRate(0), attackSpeed(0),HPRegen(0),MPRegen(0)
	{}
	
	virtual ~ArchvRoleChange(){}

public:
	//成员变量
	UInt32 level;
	UInt32 exp;
	UInt32 maxExp;
	UInt32 glory;
	UInt32 hp;
	UInt32 mp;
	UInt32 maxHp;
	UInt32 maxMp;
	UInt32 attackPowerHigh;
	UInt32 attackPowerLow;
	UInt32 defence;
	UInt32 mDefence;
	UInt32 critRate;
	UInt32 crime;
	UInt32 addPoint;
	UInt32 strength;
	UInt32 intelligence;
	UInt32 agility;
	UInt32 moveSpeed;
	UInt32 hitRate;
	UInt32 dodgeRate;
	UInt32 attackSpeed;
	UInt32 HPRegen;
	UInt32 MPRegen;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(level)
		SERIAL_ENTRY(exp)
		SERIAL_ENTRY(maxExp)
		SERIAL_ENTRY(glory)
		SERIAL_ENTRY(hp)
		SERIAL_ENTRY(mp)
		SERIAL_ENTRY(maxHp)
		SERIAL_ENTRY(maxMp)
		SERIAL_ENTRY(attackPowerHigh)
		SERIAL_ENTRY(attackPowerLow)
		SERIAL_ENTRY(defence)
		SERIAL_ENTRY(mDefence)
		SERIAL_ENTRY(critRate)
		SERIAL_ENTRY(crime)
		SERIAL_ENTRY(addPoint)
		SERIAL_ENTRY(strength)
		SERIAL_ENTRY(intelligence)
		SERIAL_ENTRY(agility)
		SERIAL_ENTRY(moveSpeed)
		SERIAL_ENTRY(hitRate)
		SERIAL_ENTRY(dodgeRate)
		SERIAL_ENTRY(attackSpeed)
		SERIAL_ENTRY(HPRegen)
		SERIAL_ENTRY(MPRegen)
	END_SERIAL_MAP()

};

//角色装备加成信息
class ArchvRoleBonus
	:public BaseArchive
{
	public:
		//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvRoleBonus():maxHp(0),maxMp(0),attackPowerHigh(0),attackPowerLow(0),
		defence(0),mDefence(0),critRate(0),
		strength(0),intelligence(0),agility(0),
		moveSpeed(0), hitRate(0),dodgeRate(0),attackSpeed(0),HPRegen(0),MPRegen(0)
		{}
	virtual ~ArchvRoleBonus(){}
		public:
		UInt32 maxHp;
		UInt32 maxMp;
		UInt32 attackPowerHigh;
		UInt32 attackPowerLow;
		UInt32 defence;
		UInt32 mDefence;
		UInt32 critRate;
		UInt32 strength;
		UInt32 intelligence;
		UInt32 agility;
		UInt32 moveSpeed;
		UInt32 hitRate;
		UInt32 dodgeRate;
		UInt32 attackSpeed;
		UInt32 HPRegen;
		UInt32 MPRegen;
		
		//成员变量序列化操作，Serializer类将按字段顺序进行序列化
		BEGIN_SERIAL_MAP()
			SERIAL_ENTRY(maxHp)
			SERIAL_ENTRY(maxMp)
			SERIAL_ENTRY(attackPowerHigh)
			SERIAL_ENTRY(attackPowerLow)
			SERIAL_ENTRY(defence)
			SERIAL_ENTRY(mDefence)
			SERIAL_ENTRY(critRate)
			SERIAL_ENTRY(strength)
			SERIAL_ENTRY(intelligence)
			SERIAL_ENTRY(agility)
			SERIAL_ENTRY(moveSpeed)
			SERIAL_ENTRY(hitRate)
			SERIAL_ENTRY(dodgeRate)
			SERIAL_ENTRY(attackSpeed)
			SERIAL_ENTRY(HPRegen)
			SERIAL_ENTRY(MPRegen)
		END_SERIAL_MAP()
};


//角色简要信息
class ArchvRoleBriefInfo
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvRoleBriefInfo():roleId(0),level(0), proID(0),hp(0),mp(0)
	,mapID(0),lastX(0),lastY(0)
	{}
	virtual ~ArchvRoleBriefInfo(){}
	
public:
	//成员变量
	UInt32 roleId;
	string roleName;
	UInt32 level;
	Byte proID;
	UInt32 hp;
	UInt32 mp;
	UInt32 mapID;
	UInt16 lastX;
	UInt16 lastY;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(roleId)
		SERIAL_ENTRY(roleName)
		SERIAL_ENTRY(level)
		SERIAL_ENTRY(proID)
		SERIAL_ENTRY(hp)
		SERIAL_ENTRY(mp)
		SERIAL_ENTRY(mapID)
		SERIAL_ENTRY(lastX)
		SERIAL_ENTRY(lastY)
	END_SERIAL_MAP()

};

class TeamRole
	:public BaseArchive
{
	public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	TeamRole():roleId(0),status(0)
	{}
	virtual ~TeamRole(){}
	
public:
	//成员变量
	UInt32 roleId;
	Byte status;	



	//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP() SERIAL_ENTRY(roleId)
		SERIAL_ENTRY(status)
		END_SERIAL_MAP()
};

class RoleRune: public BaseArchive {
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	RoleRune() :
		runeID(0), remainderTime(0) {
	}
	virtual ~RoleRune(){}

public:
	//成员变量
	UInt32 runeID;
	UInt32 remainderTime;

	//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP() SERIAL_ENTRY(runeID)
		SERIAL_ENTRY(remainderTime)
		END_SERIAL_MAP()
};

#endif
