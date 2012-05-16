#include "Creature.h"
#include <time.h>
#include "Log.h"
#include <math.h>
Creature::Creature()
{
	pthread_mutex_init(&_mutexData, NULL);

}

Creature::~Creature()
{
	pthread_mutex_destroy(&_mutexData);
}

void Creature::LockData()
{
	pthread_mutex_lock(&_mutexData);
}

void Creature::UnlockData()
{
	pthread_mutex_unlock(&_mutexData);
}

UInt32 Creature::ID()
{

	return _attrGeneral._ID;
}

string Creature::Name()
{

	return _attrGeneral._name;
}


Byte Creature::Flag()
{

	return _attrGeneral._flag;
}

UInt32 Creature::Type()
{

	return _attrGeneral._type;
}


Byte Creature::Camp()
{

	return _attrGeneral._camp;
}

UInt32 Creature::MapID()
{

	return _attrGeneral._mapID;
}


ArchvRoute Creature::Route()
{

	return _attrGeneral._route;
}

Byte Creature::Direct()
{
	return _attrGeneral._direct;
}

ArchvPosition Creature::Pos()
{
	return _attrGeneral._pos;
}

Byte Creature::Status()
{

	return _attrGeneral._status;
}

UInt32 Creature::StatusChangeTime()
{

	return _attrGeneral._statusChangeTime;
}

UInt32 Creature::Level()
{

	return _attrGeneral._level;
}

UInt32 Creature::Exp()
{

	return _attrGeneral._exp;
}

UInt32 Creature::MaxExp()
{

	return _attrGeneral._maxExp;
}


UInt32 Creature::AddPoint()
{

	return _attrGeneral._addPoint;
}

UInt32 Creature::Strength()
{

	return _attrGeneral._strength;
}

UInt32 Creature::Intelligence()
{

	return _attrGeneral._intelligence;
}

UInt32 Creature::Agility()
{

	return _attrGeneral._agility;
}

UInt32 Creature::MoveSpeed()
{

	return _attrGeneral._moveSpeed;
}


UInt32 Creature::Hp()
{

	return _attrCombat._hp;
}

UInt32 Creature::Mp()
{

	return _attrCombat._mp;
}

UInt32 Creature::MaxHp()
{

	return _attrCombat._maxHp;
}

UInt32 Creature::MaxMp()
{

	return _attrCombat._maxMp;
}

UInt32 Creature::HpRegen()
{

	return _attrCombat._hpRegen;
}

UInt32 Creature::MpRegen()
{

	return _attrCombat._mpRegen;
}

UInt32 Creature::AttackPowerHigh()
{

	return _attrCombat._attackPowerHigh;
}

UInt32 Creature::AttackPowerLow()
{

	return _attrCombat._attackPowerLow;
}

UInt32 Creature::AttackScope()
{

	return _attrCombat._attackScope;
}

UInt32 Creature::AttackSpeed()
{

	return _attrCombat._attackSpeed;
}

UInt32 Creature::BulletSpeed()
{

	return _attrCombat._bulletSpeed;
}

UInt32 Creature::Defence()
{

	return _attrCombat._defence;
}

UInt32 Creature::MDefence()
{

	return _attrCombat._mDefence;
}

UInt32 Creature::CritRate()
{

	return _attrCombat._critRate;
}

UInt32 Creature::HitRate()
{

	return _attrCombat._hitRate;
}

UInt32 Creature::DodgeRate()
{

	return _attrCombat._dodgeRate;
}

UInt32 Creature::MaxHpBonus()
{

	return _attrCombatBonus._maxHpBonus;
}

UInt32 Creature::MaxMpBonus()
{

	return _attrCombatBonus._maxMpBonus;
}

UInt32 Creature::HpRegenBonus()
{

	return _attrCombatBonus._hpRegenBonus;
}

UInt32 Creature::MpRegenBonus()
{

	return _attrCombatBonus._mpRegenBonus;
}

UInt32 Creature::AttackPowerHighBonus()
{

	return _attrCombatBonus._attackPowerHighBonus;
}

UInt32 Creature::AttackPowerLowBonus()
{

	return _attrCombatBonus._attackPowerLowBonus;
}

UInt32 Creature::AttackScopeBonus()
{

	return _attrCombatBonus._attackScopeBonus;
}

UInt32 Creature::AttackSpeedBonus()
{

	return _attrCombatBonus._attackSpeedBonus;
}

UInt32 Creature::BulletSpeedBonus()
{

	return _attrCombatBonus._bulletSpeedBonus;
}

UInt32 Creature::DefenceBonus()
{

	return _attrCombatBonus._defenceBonus;
}

UInt32 Creature::MDefenceBonus()
{

	return _attrCombatBonus._mDefenceBonus;
}

UInt32 Creature::CritRateBonus()
{

	return _attrCombatBonus._critRateBonus;
}

UInt32 Creature::HitRateBonus()
{

	return _attrCombatBonus._hitRateBonus;
}

UInt32 Creature::DodgeRateBonus()
{

	return _attrCombatBonus._dodgeRateBonus;
}
UInt32 Creature::MovSpeedBonus()
{
	return _attrCombatBonus._movespeedBonus;
}
UInt32 Creature::StrengthBonus()
{
	return _attrCombatBonus._strengthBonus;
}
UInt32 Creature::IntelligenceBonus()
{
	return _attrCombatBonus._intelligenceBonus;
}
UInt32 Creature::AgilityBonus()
{
	return _attrCombatBonus._agilityBonus;
}


void Creature::MaxHpBonus( UInt32 input)
{
	_attrCombatBonus._maxHpBonus=input;
}
void Creature::MaxMpBonus( UInt32 input)
{
	_attrCombatBonus._maxMpBonus=input;
}
void Creature::HpRegenBonus( UInt32 input)
{
	_attrCombatBonus._hpRegenBonus=input;
}
void Creature::MpRegenBonus( UInt32 input)
{
	_attrCombatBonus._mpRegenBonus=input;
}
void Creature::AttackPowerHighBonus( UInt32 input)
{
	_attrCombatBonus._attackPowerHighBonus=input;
}
void Creature::AttackPowerLowBonus( UInt32 input)
{
	_attrCombatBonus._attackPowerLowBonus=input;
}
void Creature::AttackScopeBonus( UInt32 input)
{
	_attrCombatBonus._attackScopeBonus=input;
}
void Creature::AttackSpeedBonus( UInt32 input)
{
	_attrCombatBonus._attackSpeedBonus=input;
}
void Creature::BulletSpeedBonus( UInt32 input)
{
	_attrCombatBonus._bulletSpeedBonus=input;
}
void Creature::DefenceBonus( UInt32 input)
{
	_attrCombatBonus._defenceBonus=input;
}
void Creature::MDefenceBonus( UInt32 input)
{
	_attrCombatBonus._mDefenceBonus=input;
}
void Creature::CritRateBonus( UInt32 input)
{

}
void Creature::HitRateBonus( UInt32 input)
{
	_attrCombatBonus._hitRateBonus=input;
}
void Creature::DodgeRateBonus( UInt32 input)
{
	_attrCombatBonus._dodgeRateBonus=input;
}
void Creature::MovSpeedBonus( UInt32 input)
{
	_attrCombatBonus._movespeedBonus=input;
}
void Creature::StrengthBonus( UInt32 input)
{
	_attrCombatBonus._strengthBonus=input;
}
void Creature::IntelligenceBonus( UInt32 input)
{
	_attrCombatBonus._intelligenceBonus=input;
}
void Creature::AgilityBonus( UInt32 input)
{
	_attrCombatBonus._agilityBonus=input;
}
void Creature::Camp(Byte input)
{
	_attrGeneral._camp=input;
}
void Creature::ID( UInt32 input)
{

	_attrGeneral._ID = input;
}

void Creature::Name( string input )
{

	_attrGeneral._name = input;
}

void Creature::Flag( Byte input)
{

	_attrGeneral._flag = input;
}

void Creature::Type( UInt32 input)
{

	_attrGeneral._type = input;
}


void Creature::Level( UInt32 input)
{

	_attrGeneral._level = input;
}

void Creature::Exp( UInt32 input)
{

	_attrGeneral._exp = input;
}

void Creature::MaxExp( UInt32 input)
{

	_attrGeneral._maxExp = input;
}


void Creature::AddPoint( UInt32 input)
{

	_attrGeneral._addPoint= input;
}

void Creature::Strength( UInt32 input)
{

	_attrGeneral._strength = input;
}

void Creature::Intelligence( UInt32 input)
{

	_attrGeneral._intelligence = input;
}

void Creature::Agility( UInt32 input)
{

	_attrGeneral._agility = input;
}

void Creature::Hp( UInt32 input)
{

	_attrCombat._hp = input;
}

void Creature::Mp( UInt32 input)
{

	_attrCombat._mp = input;
}

void Creature::MaxHp( UInt32 input)
{

	_attrCombat._maxHp = input;
}

void Creature::MaxMp( UInt32 input)
{

	_attrCombat._maxMp = input;
}

void Creature::HpRegen( UInt32 input)
{

	_attrCombat._hpRegen = input;
}

void Creature::MpRegen( UInt32 input)
{

	_attrCombat._mpRegen = input;
}

void Creature::AttackPowerHigh( UInt32 input)
{

	_attrCombat._attackPowerHigh = input;
}

void Creature::AttackPowerLow( UInt32 input)
{

	_attrCombat._attackPowerLow = input;
}

void Creature::AttackScope( UInt32 input)
{

	_attrCombat._attackScope = input;
}

void Creature::AttackSpeed( UInt32 input)
{

	_attrCombat._attackSpeed = input;
}

void Creature::BulletSpeed( UInt32 input)
{

	_attrCombat._bulletSpeed = input;
}

void Creature::Defence( UInt32 input)
{

	_attrCombat._defence = input;
}

void Creature::MDefence( UInt32 input)
{

	_attrCombat._mDefence = input;
}

void Creature::CritRate( UInt32 input)
{

	_attrCombat._critRate = input;
}

void Creature::HitRate( UInt32 input)
{

	_attrCombat._hitRate = input;
}

void Creature::DodgeRate( UInt32 input)
{

	_attrCombat._dodgeRate = input;
}

void Creature::Route( const ArchvRoute& input)
{

	_attrGeneral._route = input;
}


void Creature::Direct( Byte input)
{

	_attrGeneral._direct = input;
}


void Creature::Pos( const ArchvPosition& input)
{

	_attrGeneral._pos = input;
}

void Creature::Status( Byte input)
{

	_attrGeneral._status = input;
	_attrGeneral._statusChangeTime = time(NULL);
}


void Creature::MapID( UInt32 input)
{
	_attrGeneral._mapID = input;
}

void Creature::MoveSpeed( UInt32 input)
{
	_attrGeneral._moveSpeed= input;
}

void Creature::AddLevel(Int32 input)
{
	if(input < 0 && abs(input) > _attrGeneral._level)
	{
		_attrGeneral._level = 0;
	}
	else
	{
		_attrGeneral._level += input;
	}
}

void Creature::AddExp(Int32 input)
{
	if(input < 0 && abs(input) > _attrGeneral._exp)
	{
		_attrGeneral._exp = 0;
	}
	else
	{
		_attrGeneral._exp += input;
	}
}


void Creature::AddMaxExp(Int32 input)
{
	if(input < 0 && abs(input) > _attrGeneral._maxExp)
	{
		_attrGeneral._maxExp = 0;
	}
	else
	{
		_attrGeneral._maxExp += input;
	}
}

void Creature::AddAddPoint(Int32 input)
{
	if(input < 0 && abs(input) > _attrGeneral._addPoint)
	{
		_attrGeneral._addPoint = 0;
	}
	else
	{
		_attrGeneral._addPoint += input;
	}
}

void Creature::AddStrength(Int32 input)
{
	if(input < 0 && abs(input) > _attrGeneral._strength)
	{
		_attrGeneral._strength = 0;
	}
	else
	{
		_attrGeneral._strength += input;
	}
}

void Creature::AddIntelligence(Int32 input)
{
	if(input < 0 && abs(input) > _attrGeneral._intelligence)
	{
		_attrGeneral._intelligence = 0;
	}
	else
	{
		_attrGeneral._intelligence += input;
	}
}

void Creature::AddAgility(Int32 input)
{
	if(input < 0 && abs(input) > _attrGeneral._agility)
	{
		_attrGeneral._agility = 0;
	}
	else
	{
		_attrGeneral._agility += input;
	}
}

void Creature::AddHp(Int32 input)
{

	if((_attrCombat._hp+input)<_attrCombat._maxHp+_attrCombatBonus._maxHpBonus)
	{
		if(input < 0 && abs(input) > _attrCombat._hp)
			_attrCombat._hp = 0;
		else
		_attrCombat._hp += input;
	}
	else
	{
		_attrCombat._hp=_attrCombat._maxHp+_attrCombatBonus._maxHpBonus;
	}
}

void Creature::AddMp(Int32 input)
{
	if((_attrCombat._mp+input)< _attrCombat._maxMp+_attrCombatBonus._maxMpBonus)
	{
		if(input < 0 && abs(input) > _attrCombat._mp)
			_attrCombat._mp = 0;
		else
			_attrCombat._mp += input;
	}
	else
	{
		_attrCombat._mp=_attrCombat._maxMp+_attrCombatBonus._maxMpBonus;
	}

}

void Creature::AddMaxHp(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._maxHp)
	{
		_attrCombat._maxHp = 0;
	}
	else
	{
		_attrCombat._maxHp+= input;
	}
}

void Creature::AddMaxMp(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._maxMp)
	{
		 _attrCombat._maxMp = 0;
	}
	else
	{
		_attrCombat._maxMp+= input;
	}
}

void Creature::AddHpRegen(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._hpRegen)
	{
		 _attrCombat._hpRegen = 0;
	}
	else
	{
		_attrCombat._hpRegen += input;
	}
}

void Creature::AddMpRegen(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._mpRegen)
	{
		 _attrCombat._mpRegen = 0;
	}
	else
	{
		_attrCombat._mpRegen += input;
	}
}

void Creature::AddAttackPowerHigh(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._attackPowerHigh)
	{
		 _attrCombat._attackPowerHigh = 0;
	}
	else
	{
		_attrCombat._attackPowerHigh += input;
	}
}

void Creature::AddAttackPowerLow(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._attackPowerLow)
	{
		_attrCombat._attackPowerLow = 0;
	}
	else
	{
		_attrCombat._attackPowerLow += input;
	}
}

void Creature::AddAttackScope(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._attackScope)
	{
		 _attrCombat._attackScope = 0;
	}
	else
	{
		_attrCombat._attackScope += input;
	}
}

void Creature::AddAttackSpeed(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._attackSpeed)
	{
		 _attrCombat._attackSpeed = 0;
	}
	else
	{
		_attrCombat._attackSpeed += input;
	}
}

void Creature::AddBulletSpeed(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._bulletSpeed)
	{
		_attrCombat._bulletSpeed = 0;
	}
	else
	{
		_attrCombat._bulletSpeed += input;
	}
}

void Creature::AddDefence(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._defence)
	{
		 _attrCombat._defence = 0;
	}
	else
	{
		_attrCombat._defence += input;
	}
}

void Creature::AddMDefence(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._mDefence)
	{
		_attrCombat._mDefence = 0;
	}
	else
	{
		_attrCombat._mDefence += input;
	}
}

void Creature::AddCritRate(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._critRate)
	{
		_attrCombat._critRate = 0;
	}
	else
	{
		_attrCombat._critRate += input;
	}
}

void Creature::AddHitRate(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._hitRate)
	{
		_attrCombat._hitRate = 0;
	}
	else
	{
		_attrCombat._hitRate += input;
	}
}

void Creature::AddDodgeRate(Int32 input)
{
	if(input < 0 && abs(input) > _attrCombat._dodgeRate)
	{
		_attrCombat._dodgeRate = 0;
	}
	else
	{
		_attrCombat._dodgeRate += input;
	}
}

void Creature::AddMoveSpeed( Int32 input)
{
	if(input < 0 && abs(input) >_attrGeneral._moveSpeed)
	{
		_attrGeneral._moveSpeed = 0;
	}
	else
	{
		_attrGeneral._moveSpeed+= input;
	}
}



void Creature::AddMaxHpBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._maxHpBonus)
	{
		_attrCombatBonus._maxHpBonus = 0;
	}
	else
	{
		_attrCombatBonus._maxHpBonus += input;
	}
}

void Creature::AddMaxMpBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._maxMpBonus)
	{
		_attrCombatBonus._maxMpBonus = 0;
	}
	else
	{
		_attrCombatBonus._maxMpBonus += input;
	}
}

void Creature::AddHpRegenBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._hpRegenBonus)
	{
		_attrCombatBonus._hpRegenBonus = 0;
	}
	else
	{
		_attrCombatBonus._hpRegenBonus += input;
	}
}

void Creature::AddMpRegenBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._mpRegenBonus)
	{
		_attrCombatBonus._mpRegenBonus = 0;
	}
	else
	{
		_attrCombatBonus._mpRegenBonus += input;
	}
}

void Creature::AddAttackPowerHighBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._attackPowerHighBonus)
	{
		_attrCombatBonus._attackPowerHighBonus = 0;
	}
	else
	{
		_attrCombatBonus._attackPowerHighBonus += input;
	}
}

void Creature::AddAttackPowerLowBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._attackPowerLowBonus)
	{
		_attrCombatBonus._attackPowerLowBonus = 0;
	}
	else
	{
		_attrCombatBonus._attackPowerLowBonus += input;
	}

}

void Creature::AddAttackScopeBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._attackScopeBonus)
	{
		_attrCombatBonus._attackScopeBonus = 0;
	}
	else
	{
		_attrCombatBonus._attackScopeBonus += input;
	}

}

void Creature::AddAttackSpeedBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._attackSpeedBonus)
	{
		_attrCombatBonus._attackSpeedBonus = 0;
	}
	else
	{
		_attrCombatBonus._attackSpeedBonus += input;
	}
}

void Creature::AddBulletSpeedBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._bulletSpeedBonus)
	{
		_attrCombatBonus._bulletSpeedBonus = 0;
	}
	else
	{
		_attrCombatBonus._bulletSpeedBonus += input;
	}
}

void Creature::AddDefenceBonus(Int32 input)
{
	//LOG(LOG_ERROR,__FILE__,__LINE__,"input[%d]---_defenceBonus[%d]",input,_attrCombatBonus._defenceBonus);
	if(input < 0 && abs(input) >_attrCombatBonus._defenceBonus)
	{
		_attrCombatBonus._defenceBonus = 0;
	}
	else
	{
		_attrCombatBonus._defenceBonus += input;
	}
}

void Creature::AddMDefenceBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._mDefenceBonus)
	{
		_attrCombatBonus._mDefenceBonus = 0;
	}
	else
	{
		_attrCombatBonus._mDefenceBonus += input;
	}

}

void Creature::AddCritRateBonus(Int32 input)
{
     LOG(LOG_ERROR,__FILE__,__LINE__,"input[%d]-_critRateBonus[%d]",input,_attrCombatBonus._critRateBonus);
	if(input < 0 && abs(input) >_attrCombatBonus._critRateBonus)
	{
		_attrCombatBonus._critRateBonus = 0;
	}
	else
	{
		_attrCombatBonus._critRateBonus += input;
	}

	LOG(LOG_ERROR,__FILE__,__LINE__,"---AddCritRateBonus-----_critRateBonus[%d]",_attrCombatBonus._critRateBonus);
}

void Creature::AddHitRateBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._hitRateBonus)
	{
		_attrCombatBonus._hitRateBonus = 0;
	}
	else
	{
		_attrCombatBonus._hitRateBonus += input;
	}
}

void Creature::AddDodgeRateBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._dodgeRateBonus)
	{
		_attrCombatBonus._dodgeRateBonus = 0;
	}
	else
	{
		_attrCombatBonus._dodgeRateBonus += input;
	}
}
void Creature::AddMoveSpeedBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._movespeedBonus)
	{
		_attrCombatBonus._movespeedBonus = 0;
	}
	else
	{
		_attrCombatBonus._movespeedBonus +=input;
	}
}
void Creature::AddStrengthBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._strengthBonus)
	{
		_attrCombatBonus._strengthBonus = 0;
	}
	else
	{
		_attrCombatBonus._strengthBonus +=input;
	}
}
void Creature::AddIntelligenceBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._intelligenceBonus)
	{
		_attrCombatBonus._intelligenceBonus = 0;
	}
	else
	{
		_attrCombatBonus._intelligenceBonus +=input;
	}
}
void Creature::AddAgilityBonus(Int32 input)
{
	if(input < 0 && abs(input) >_attrCombatBonus._agilityBonus)
	{
		_attrCombatBonus._agilityBonus = 0;
	}
	else
	{
		_attrCombatBonus._agilityBonus +=input;
	}
}


//计算角色在地图上的当前位置,并整理移动路径
//	剔除移动路径中已走过的关键点
//	并把当前位置作为新移动路径的第一个关键点
//@return  0 成功  非0 失败
int Creature::CalcCurrPos()
{

	UInt32 uiTime = time(NULL);
	double dSpeed = ROLE_MOVE_SPEED*10/1000.0;				//速度， dSpeed 单位: 格/秒,  _moveSpeed单位:  微型格/0.1秒
																					// 其中  1格子=1000微型格
	double dDistance = 0;			//用户从起始时间到当前时间的理论累计行程
	double	dDisLeft = 0.0;
	ArchvPosition	tmpPos1;
	ArchvPosition	tmpPos2;
	double	dPosDis;							//坐标间距离
	int	iCalcFlag = 0;						//计算标志， 为1说明角色仍在行走，需根据比例计算当前位置
	int iCount = 0;								//走过关键点的个数

/*DEBUG_PRINTF3( "1111111 ,Route.listPos.size()[%d], uiTime[%ld], _attrGeneral._route.time[%ld] \n",
	_attrGeneral._route.listPos.size(), uiTime,  (long)_attrGeneral._route.time );*/
/*LOG(LOG_DEBUG,__FILE__,__LINE__,"-direct[%d],pos.X[%d],pos.Y[%d] ",
	_attrGeneral._direct, _attrGeneral._pos.X, _attrGeneral._pos.Y );*/


	//关键坐标的数量
	if( _attrGeneral._route.listPos.size() <=1 ||
		uiTime <= _attrGeneral._route.time )
		return 0;

	//计算用户累计行程
	dDistance = ( time(NULL) - _attrGeneral._route.time ) * dSpeed;
DEBUG_PRINTF1( "2222222 ,dDistance[%f] \n",  dDistance );
	if( dDistance <= 0.0 )
		return 0;

	//计算用户当前坐标
	List<ArchvPosition>::iterator it;
	tmpPos1 = *(_attrGeneral._route.listPos.begin());
	tmpPos2 = *(_attrGeneral._route.listPos.begin());
	dDisLeft = dDistance;
	for( it = _attrGeneral._route.listPos.begin(); it !=  _attrGeneral._route.listPos.end(); it++ )
	{
		iCount++;

		//第一个略过
		if( it == _attrGeneral._route.listPos.begin() )
 			continue;

		tmpPos2 = *(it);
		dPosDis = CalcPosDistance( tmpPos1, tmpPos2, _attrGeneral._direct );

		if( (dPosDis - dDisLeft ) >= 0.1 )
		{
			iCalcFlag = 1;
			break;
		}

		dDisLeft -= dPosDis;

		tmpPos1 = *(it);
	}

	//角色是否仍在行走
	if( 1 == iCalcFlag )
	{
		//角色仍在行走，根据比例计算当前位置
		_attrGeneral._pos.X = tmpPos1.X + (Int16)(( dDisLeft/dPosDis )*( tmpPos2.X - tmpPos1.X ));
		_attrGeneral._pos.Y = tmpPos1.Y + (Int16)(( dDisLeft/dPosDis )*( tmpPos2.Y - tmpPos1.Y ));

		//剔除走过的关键点
		iCount--;
		List<ArchvPosition>::iterator it;
		for( int i = 0; i < iCount; i++ )
		{
			it = _attrGeneral._route.listPos.begin();
			_attrGeneral._route.listPos.erase(it);
		}

		//当前位置作为新移动路径的第一个关键点
		it = _attrGeneral._route.listPos.begin();
		_attrGeneral._route.time = time(NULL);		//当前时间
		_attrGeneral._route.listPos.insert( it, _attrGeneral._pos );

DEBUG_PRINTF6( "333333 , dDisLeft[%f], dPosDis[%f], tmpPos1.X[%d], tmpPos1.Y[%d]; tmpPos2.X[%d], tmpPos2.Y[%d]; \n",
	dDisLeft, dPosDis, tmpPos1.X, tmpPos1.Y, tmpPos2.X, tmpPos2.Y );

DEBUG_PRINTF3( "4444 ,direct[%d],pos.X[%d],pos.Y[%d] \n", _attrGeneral._direct, _attrGeneral._pos.X, _attrGeneral._pos.Y );

	}
	else
	{
		//角色不再移动，停在最后一个关键点
		_attrGeneral._pos.X = tmpPos2.X;
		_attrGeneral._pos.Y = tmpPos2.Y;

		//整理移动路径
		ArchvPosition posTmp = _attrGeneral._route.listPos.back();
		_attrGeneral._route.time = time(NULL);		//当前时间
		_attrGeneral._route.listPos.clear();
		_attrGeneral._route.listPos.push_back(posTmp);		//仅保留最后一个关键点


DEBUG_PRINTF3( "5555 ,direct[%d],pos.X[%d],pos.Y[%d] \n", _attrGeneral._direct, _attrGeneral._pos.X, _attrGeneral._pos.Y );
	}

	return 0;
}



//近似计算两坐标间的直线距离
//	为使得计算结果准确，需保证本函数两个入参坐标的连线，要么与坐标轴平行，要么与坐标轴呈45度角
//	原因:本游戏的角色移动，只有八个方向，每两个临近方向的夹角是45度角；
//		任何角色的移动路径，必须保证其中的坐标点均是拐点；以方便服务端做近视计算
//
//@param	pos1		坐标1
//@param	pos2		坐标2
//@param	direct	返回的方向
//@return	>=0		两坐标的直线距离   <0		失败
double Creature::CalcPosDistance( ArchvPosition &pos1, ArchvPosition &pos2, Byte & direct )
{
	UInt32 absX = abs(pos1.X-pos2.X);
	UInt32 absY = abs(pos1.Y-pos2.Y);

	//计算方向
	direct = CalcDirect( pos1, pos2 );

	//计算两点间距离
	if( absX > 0 && absY > 0 )
		return 1.41421 * absX;

	return (absX+absY);
}


//计算方向
//@param	pos1		坐标1
//@param	pos2		坐标2
//@return	direct	返回的方向
Byte Creature::CalcDirect( ArchvPosition &pos1, ArchvPosition &pos2 )
{
	Byte bDrt = 0;	//行走方向,    东 0x10   西 0x90     北 0x01  南 0x09
	Int16 minusX = pos1.X-pos2.X;
	Int16 minusY = pos1.Y-pos2.Y;
	Byte bRet = 0;

	//同一个点，返回随机方向
	if( 0 == minusX && 0 == minusY )
		return AttrGeneral::GetRandDirect();

	//计算东西方向
	if( minusX > 0 )
		bDrt = 0x90;		//往西走
	else if( minusX < 0 )
		bDrt = 0x10;		//往东走

	//计算南北方向
	if( minusY > 0 )
		bDrt += 0x01;		//往北走
	else if( minusY < 0 )
		bDrt += 0x09;		//往南走

	switch( bDrt )
	{
		case 0x01:	//北
			bRet = DIRECT_NORTH;
			break;

		case 0x11:	//东北
			bRet = DIRECT_EASTNORTH;
			break;

		case 0x10:	//东
			bRet = DIRECT_EAST;
			break;

		case 0x19:	//东南
			bRet = DIRECT_EASTSOUTH;
			break;

		case 0x09:	//南
			bRet = DIRECT_SOUTH;
			break;

		case 0x99:	//西南
			bRet = DIRECT_WESTSOUTH;
			break;

		case 0x90:	//西
			bRet = DIRECT_WEST;
			break;

		case 0x91:	//西北
			bRet = DIRECT_WESTNORTH;
			break;

		default:		//默认 随机方向
			bRet = AttrGeneral::GetRandDirect();
			break;
	}

	return bRet;
}

void Creature::Print()
{
	DEBUG_PRINTF12("ID[%d]	,Flag[%d]	,Type[%d]	,Camp[%d] \
		,MapID[%d]	,Status[%d]	,MaxHp[%d]	,MaxMp[%d] \
		,Hp[%d]	,Mp[%d]	,MoveSpeed[%d]	,AttackPowerHigh[%d] ",
		ID()	,Flag()	,Type()	,Camp()
		,MapID()	,Status()	,MaxHp()	,MaxMp()
		,Hp()	,Mp()	,MoveSpeed()	,AttackPowerHigh() );

	DEBUG_PRINTF8(" AttackPowerLow[%d]	,AttackScope[%d]	,AttackSpeed[%d]	,BulletSpeed[%d] \
		,HitRate[%d]	,DodgeRate[%d]	,Defence[%d]	,MDefence[%d]",
		AttackPowerLow()	,AttackScope()	,AttackSpeed()	,BulletSpeed()
		,HitRate()	,DodgeRate()	,Defence()	,MDefence() );
}


