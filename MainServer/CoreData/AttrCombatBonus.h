//		加成战斗属性类

#ifndef ATTRCOMBATBONUS_H
#define ATTRCOMBATBONUS_H

#include "OurDef.h"


class AttrCombatBonus
{
public:
	friend class Creature;

public:
	AttrCombatBonus()
	:_maxHpBonus(0),
	_maxMpBonus(0),
	_hpRegenBonus(0),
	_mpRegenBonus(0),
	_attackPowerHighBonus(0),
	_attackPowerLowBonus(0),
	_attackScopeBonus(0),
	_attackSpeedBonus(0),
	_bulletSpeedBonus(0),
	_defenceBonus(0),
	_mDefenceBonus(0),
	_critRateBonus(0),
	_hitRateBonus(0),
	_dodgeRateBonus(0),
	_strengthBonus(0),
	_intelligenceBonus(0),
	_agilityBonus(0),
	_movespeedBonus(0)
	{}
	~AttrCombatBonus(){}


private:
	//加成红上限
	UInt32 _maxHpBonus;

	//加成蓝上限
	UInt32 _maxMpBonus;

	//加成回红速度
	UInt32 _hpRegenBonus;

	//加成回蓝速度
	UInt32 _mpRegenBonus;

	//加成攻击力上限
	UInt32 _attackPowerHighBonus;

	//加成攻击力下限
	UInt32 _attackPowerLowBonus;

	//加成攻击范围
	UInt32 _attackScopeBonus;

	//加成攻击速度
	UInt32 _attackSpeedBonus;

	//加成子弹速度
	UInt32 _bulletSpeedBonus;

	//加成防御力
	UInt32 _defenceBonus;

	//加成魔抗,魔法防御力
	UInt32 _mDefenceBonus;

	//加成暴击率
	UInt32 _critRateBonus;

	//加成命中率
	UInt32 _hitRateBonus;

	//加成闪避率
	UInt32 _dodgeRateBonus;
	//力量加成
	UInt32 _strengthBonus;
	//智力加成
	UInt32 _intelligenceBonus;
	//敏捷加成
	UInt32 _agilityBonus;
	//速度加成
	UInt32 _movespeedBonus;
	
};


#endif

