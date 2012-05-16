/**
 *	地图 序列化用的类
 *	
 */

#ifndef ARCHVMAP_H
#define ARCHVMAP_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"


//坐标
class ArchvPosition
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvPosition():X(0), Y(0)
	{}

	ArchvPosition(UInt16 inputX, UInt16 inputY)
	:X(inputX), Y(inputY)
	{}
 
public:
	//成员变量
	UInt16	X;
	UInt16	Y;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(X)
		SERIAL_ENTRY(Y)
	END_SERIAL_MAP()

};

//角色移动路径
class ArchvRoute
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvRoute():time(0)
	{
	}

	ArchvRoute(UInt32 uTime, ArchvPosition &pos)
	:time(uTime)
	{
		listPos.push_back(pos);
	}
	
public:
	//成员变量
	UInt32	time;							//移动路径初始时间
	List<ArchvPosition> listPos;		//用户移动坐标集合
	
//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(time)
		SERIAL_ENTRY(listPos)
	END_SERIAL_MAP()

};

//角色移动描述
class ArchvRoleMoveDesc
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvRoleMoveDesc():roleID(0), speed(0)
	{
	}
	
public:
	//成员变量
	UInt32				roleID;
	UInt32				speed;				//移动速度
	ArchvRoute		route;				//用户移动路径
	
	
//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(roleID)
		SERIAL_ENTRY(speed)
		SERIAL_ENTRY(route)
	END_SERIAL_MAP()

};



//场景生物(角色、怪、宠物)状态描述
class ArchvCreatureStatus
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvCreatureStatus():creatureFlag(0), creatureType(0), ID(0), status(0)
	{
	}
	
public:
	//成员变量
	Byte 					creatureFlag;
	UInt32 				creatureType;
	UInt32				ID;						//ID：角色ID、怪ID
	Byte					status;				//状态：0 死 1 活着未战斗 2 战斗
	
	
//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(creatureFlag)
		SERIAL_ENTRY(creatureType)
		SERIAL_ENTRY(ID)
		SERIAL_ENTRY(status)
	END_SERIAL_MAP()

};

class SenceMonster
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	SenceMonster(): MonsterType(0), num(0)
	{
	}
	
public:
	UInt32 MonsterType;
	UInt16 num;//1活的,2战斗中，4死的
	


};


class SenceMonsterAll
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	SenceMonsterAll():X(0), Y(0), MonsterID(0), MonsterType(0),Status(0),dieTime(0)
	{
	}
	
public:
	UInt16 X;
	UInt16 Y;
	UInt32 MonsterID;
	UInt32 MonsterType;
	Byte Status;//1活的,2战斗中，4死的
	UInt32 dieTime;
	list<SenceMonster> Monsterp;
	
	


};



#endif
