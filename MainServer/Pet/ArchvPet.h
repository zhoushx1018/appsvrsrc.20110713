/**
 *	Pet 序列化用的类
 *	
 */

#ifndef ARCHVPET_H
#define ARCHVPET_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"


//技能 
class PetBrief
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	PetBrief()
	:petId(0),petType(0),petLevel(0)
	{}
	
public:
	//成员变量
	UInt32 petId;
	UInt16 petType;
	string petName;
	UInt32 petLevel;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
	SERIAL_ENTRY(petId)
	SERIAL_ENTRY(petType)
	SERIAL_ENTRY(petName)
	SERIAL_ENTRY(petLevel)
	END_SERIAL_MAP()

};


class PetEquip
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	PetEquip():EquipIndex(0),ItemID(0),EntityID(0),Durability(0),BindState(0)
	{}
	
public:
	//成员变量
	Byte EquipIndex;   //装备编号
	UInt32 ItemID;     //物品ID
	UInt32 EntityID;   //实体ID
	UInt16 Durability;  //耐久度
	Byte BindState;    //绑定状态

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
	SERIAL_ENTRY(EquipIndex)
	SERIAL_ENTRY(ItemID)
	SERIAL_ENTRY(EntityID)
	SERIAL_ENTRY(Durability)
	SERIAL_ENTRY(BindState)
	END_SERIAL_MAP()

};




class PetBonus
	:public BaseArchive
{
public:
	PetBonus():MPRegen(0), HPRegen(0), MaxHP(0), MaxMP(0), MoveSpeed(0), Agility(0),
		Intelligence(0), Strength(0), attackPower(0), 
		AttackSpeed(0), Defence(0), MDefence(0), CritRate(0) ,HitRate(0),DodgeRate(0)
		{}
public:

	UInt32 MaxHP;
	UInt32 MaxMP;
	UInt32 attackPower;
	UInt32 Defence;
	UInt32 MDefence;
	UInt32 CritRate;
	UInt32 Strength;
	UInt32 Intelligence;
	UInt32 Agility;	
	UInt32 MoveSpeed;
	UInt32 HitRate;
	UInt32 DodgeRate;
	UInt32 AttackSpeed;
	UInt32 HPRegen;
	UInt32 MPRegen;
	
	
	BEGIN_SERIAL_MAP()
	SERIAL_ENTRY(MaxHP)
	SERIAL_ENTRY(MaxMP)
	SERIAL_ENTRY(attackPower)
	SERIAL_ENTRY(Defence)
	SERIAL_ENTRY(MDefence)
	SERIAL_ENTRY(CritRate)
	SERIAL_ENTRY(Strength)
	SERIAL_ENTRY(Intelligence)
	SERIAL_ENTRY(Agility)
	SERIAL_ENTRY(MoveSpeed)
	SERIAL_ENTRY(HitRate)
	SERIAL_ENTRY(DodgeRate)
	SERIAL_ENTRY(AttackSpeed)
	SERIAL_ENTRY(HPRegen)
	SERIAL_ENTRY(MPRegen)
	END_SERIAL_MAP()
	
};

class PetInfo
	:public BaseArchive
{
public:
	PetInfo()
	:  Exp(0), MaxExp(0), AddPoint(0),
	Strength(0), Intelligence(0),
	Agility(0), MoveSpeed(0), HP(0), MP(0), MaxHP(0), MaxMP(0), HPRegen(0), MPRegen(0),
	AttackPowerHigh(), AttackPowerLow(),
	  AttackSpeed(0), Defence(0), MDefence(0), CritRate(0), HitRate(0),
	DodgeRate(0), Level(0){}
public:
	
	UInt32 Exp     ;                        //经验值
	UInt32 MaxExp   ;                       //该等级最大经验
	UInt32 HP       ;                       //生命值，红
	UInt32 MP       ;                       //魔法值，蓝
	UInt32 MaxHP     ;                      //最大红
	UInt32 MaxMP    ;                       //最大蓝
	UInt32 AttackPowerHigh  ;               //物理攻击力，上限
	UInt32 AttackPowerLow  ;                //物理攻击力，下限
	UInt32 Defence    ;                     //防御力，物理防御力
	UInt32 MDefence   ;                     //魔抗；魔法防御力 
	UInt32 CritRate   ;                     //暴击率
	UInt32 AddPoint ;                       //待加属性点
	UInt32 Strength  ;                      //力量
	UInt32 Intelligence   ;                //之力
	UInt32 Agility    ;                     //敏捷度
	UInt32 MoveSpeed   ;                    //移动速度
	UInt32 HitRate    ;                     //命中率
	UInt32 DodgeRate  ;                      //闪避率
	UInt32 AttackSpeed   ;                  //攻击速度
	UInt32 HPRegen  ;                       //回红速度
	UInt32 MPRegen   ;                      //回蓝速度
	UInt32 Level;                           //级别
	string PetName;                      //名字

	BEGIN_SERIAL_MAP()
	SERIAL_ENTRY(Exp)
	SERIAL_ENTRY(MaxExp)
	SERIAL_ENTRY(HP)
	SERIAL_ENTRY(MP)
	SERIAL_ENTRY(MaxHP)
	SERIAL_ENTRY(MaxMP)
	SERIAL_ENTRY(AttackPowerHigh)
	SERIAL_ENTRY(AttackPowerLow)
	SERIAL_ENTRY(Defence)
	SERIAL_ENTRY(MDefence)
	SERIAL_ENTRY(CritRate)
	SERIAL_ENTRY(AddPoint)
	SERIAL_ENTRY(Strength)
	SERIAL_ENTRY(Intelligence)
	SERIAL_ENTRY(Agility)
	SERIAL_ENTRY(MoveSpeed)
	SERIAL_ENTRY(HitRate)
	SERIAL_ENTRY(DodgeRate)
	SERIAL_ENTRY(AttackSpeed)
	SERIAL_ENTRY(HPRegen)
	SERIAL_ENTRY(MPRegen)
	SERIAL_ENTRY(Level)
	SERIAL_ENTRY(PetName)
	END_SERIAL_MAP()

	
};

class PetbrifInfo
	:public BaseArchive
{
	public:    
	PetbrifInfo():petID(0),PetType(0),Level(0),IsOut(0)
	{}
	public:
		UInt32 petID;
		UInt32 PetType;
		string PetName;
		UInt32 Level;
		Byte IsOut;
	public:
		BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(petID)
		SERIAL_ENTRY(PetType)
		SERIAL_ENTRY(PetName)
		SERIAL_ENTRY(Level)
		SERIAL_ENTRY(IsOut)
		END_SERIAL_MAP()
};

#endif

