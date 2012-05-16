//		一般属性类
#ifndef ATTRGENERAL_H
#define ATTRGENERAL_H

#include "OurDef.h"
#include "ArchvMap.h"
#include <string>


using namespace std;

class AttrGeneral
{
public:
	friend class Creature;
	
public:
	AttrGeneral()
	:_ID(0),
	_flag(0),
	_type(0),
	_camp(0),
	_mapID(0),
	_status(0),					
	_level(0),
	_exp(0),
	_maxExp(0),
	_addPoint(0),
	_strength(0),
	_intelligence(0),
	_agility(0),
	_moveSpeed(541)
	{
		_direct = GetRandDirect();
		_statusChangeTime = time(NULL);
	}

	~AttrGeneral()
	{
	}

	static Byte GetRandDirect()
	{
		UInt32 uiTime = time(NULL);
		srand( uiTime );
		return (rand()%(MAXROLEDIRECT)) + 1;
	}

	//--------------业务处理-----------

private:
	//ID
	UInt32 _ID;

	//名称
	string _name;

	//生物标志	1 角色  2 怪   3 npc  4 宠物
	Byte _flag;

	//生物类型	角色的proID 或者怪Type 或者 NpcType  或者宠物Type
	UInt32 _type;

	//阵营  1 凯铎 2 仙亚  3 中立 4 全敌对关系  5 全和平
	//
	Byte	_camp;

	//地图ID 0 表示未进入地图
	UInt32 _mapID;

	//移动路径
	ArchvRoute  _route;

	//当前方向
	Byte _direct;

	//当前位置
	ArchvPosition _pos;

	//生物状态	//	0 死 1 活着未战斗  2 战斗
	Byte _status;

	//生物状态变更时间
	//	记录最近一次生物状态变更时间
	UInt32 _statusChangeTime;

	//游戏级别
	UInt32 _level;

	//经验
	UInt32 _exp;

	//最大经验值
	UInt32 _maxExp;

	//待加属性点
	UInt32 _addPoint;

	//力量
	UInt32 _strength;

	//智力
	UInt32 _intelligence;

	//敏捷
	UInt32 _agility;

	//移动速度, 截止20101225没有使用,保留
	UInt32 _moveSpeed;

};


#endif

