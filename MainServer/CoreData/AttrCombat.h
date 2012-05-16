//		基础战斗属性类

#ifndef ATTRCOMBAT_H
#define ATTRCOMBAT_H

#include "OurDef.h"

class AttrCombat
{
public:
	friend class Creature;
	
public:
	AttrCombat()
	:_hp(0),
	_mp(0),
	_maxHp(0),
	_maxMp(0),
	_hpRegen(0),
	_mpRegen(0),
	_attackPowerHigh(0),
	_attackPowerLow(0),
	_attackScope(0),
	_attackSpeed(0),
	_bulletSpeed(0),
	_defence(0),
	_mDefence(0),
	_critRate(0),
	_hitRate(0),
	_dodgeRate(0)
	{}
	~AttrCombat(){}

private:
	//红
	UInt32 _hp;

	//蓝
	UInt32 _mp;

	//红上限
	UInt32 _maxHp;

	//蓝上限
	UInt32 _maxMp;

	//回红速度
	UInt32 _hpRegen;

	//回蓝速度
	UInt32 _mpRegen;

	//攻击力上限
	UInt32 _attackPowerHigh;

	//攻击力下限
	UInt32 _attackPowerLow;

	//攻击范围
	UInt32 _attackScope;

	//攻击速度
	UInt32 _attackSpeed;

	//子弹速度
	UInt32 _bulletSpeed;

	//防御力
	UInt32 _defence;

	//魔抗,魔法防御力
	UInt32 _mDefence;

	//暴击率
	UInt32 _critRate;

	//命中率
	UInt32 _hitRate;

	//闪避率
	UInt32 _dodgeRate;
	
};


#endif

