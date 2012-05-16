//		生物类  基类

#ifndef CREATURE_H
#define CREATURE_H

#include "OurDef.h"
#include "ArchvMap.h"
#include <pthread.h>
#include <string>
#include "AttrGeneral.h"
#include "AttrCombat.h"
#include "AttrCombatBonus.h"

using namespace std;

//方向
enum DIRECT{
	DIRECT_NORTH = 1,	//北
	DIRECT_EASTNORTH,	//东北
	DIRECT_EAST			,	//东
	DIRECT_EASTSOUTH,	//东南
	DIRECT_SOUTH		,	//南
	DIRECT_WESTSOUTH,	//西南
	DIRECT_WEST			,	//西
	DIRECT_WESTNORTH,	//西北
};

class Creature
{
public:
	virtual ~Creature();
protected:
	Creature();		//禁止实例化对象

public:
	//--------------打印-----------
	void Print();
	//--------------业务处理-----------
	int CalcCurrPos();
	double CalcPosDistance( ArchvPosition &pos1, ArchvPosition &pos2, Byte & direct );
	Byte CalcDirect( ArchvPosition &pos1, ArchvPosition &pos2 );

	//--------------加解锁-----------
	void LockData();
	void UnlockData();


public:
	//获取基础属性
	UInt32 ID();
	string Name();
	Byte Flag();
	UInt32 Type();
	Byte Camp();
	UInt32 MapID();
	ArchvRoute Route();
	Byte Direct();
	ArchvPosition Pos();
	Byte Status();
	UInt32 StatusChangeTime();
	UInt32 Level();
	UInt32 Exp();
	UInt32 MaxExp();
	UInt32 AddPoint();
	UInt32 Strength();
	UInt32 Intelligence();
	UInt32 Agility();
	UInt32 MoveSpeed();
	UInt32 Hp();
	UInt32 Mp();
	UInt32 MaxHp();
	UInt32 MaxMp();
	UInt32 HpRegen();
	UInt32 MpRegen();
	UInt32 AttackPowerHigh();
	UInt32 AttackPowerLow();
	UInt32 AttackScope();
	UInt32 AttackSpeed();
	UInt32 BulletSpeed();
	UInt32 Defence();
	UInt32 MDefence();
	UInt32 CritRate();
	UInt32 HitRate();
	UInt32 DodgeRate();

	//获取加成属性
	UInt32 MaxHpBonus();
	UInt32 MaxMpBonus();
	UInt32 HpRegenBonus();
	UInt32 MpRegenBonus();
	UInt32 AttackPowerHighBonus();
	UInt32 AttackPowerLowBonus();
	UInt32 AttackScopeBonus();
	UInt32 AttackSpeedBonus();
	UInt32 BulletSpeedBonus();
	UInt32 DefenceBonus();
	UInt32 MDefenceBonus();
	UInt32 CritRateBonus();
	UInt32 HitRateBonus();
	UInt32 DodgeRateBonus();
	UInt32 MovSpeedBonus();
	UInt32 StrengthBonus();
	UInt32 IntelligenceBonus();
	UInt32 AgilityBonus();



	//基础属性修改
	void ID( UInt32 input );
	void Camp(Byte input);
	void Name( string input );
	void Flag( Byte input );
	void Type( UInt32 input );
	void Level(UInt32 input );
	void Exp(UInt32 input );
	void MaxExp(UInt32 input );
	void AddPoint(UInt32 input );
	void Strength(UInt32 input );
	void Intelligence(UInt32 input);
	void Agility(UInt32 input);
	void Hp(UInt32 input);
	void Mp(UInt32 input);
	void MaxHp(UInt32 input);
	void MaxMp(UInt32 input);
	void HpRegen(UInt32 input);
	void MpRegen(UInt32 input);
	void AttackPowerHigh(UInt32 input);
	void AttackPowerLow(UInt32 input);
	void AttackScope(UInt32 input);
	void AttackSpeed(UInt32 input);
	void BulletSpeed(UInt32 input);
	void Defence(UInt32 input);
	void MDefence(UInt32 input);
	void CritRate(UInt32 input);
	void HitRate(UInt32 input);
	void DodgeRate(UInt32 input);
	void Route( const ArchvRoute& input);
	void Direct( Byte input);
	void Pos( const ArchvPosition& input);
	void Status( Byte input );
	void MapID( UInt32 input );
	void MoveSpeed( UInt32 input );

	//加成属性的设置
	void MaxHpBonus( UInt32 input);
	void MaxMpBonus( UInt32 input);
	void HpRegenBonus( UInt32 input);
	void MpRegenBonus( UInt32 input);
	void AttackPowerHighBonus( UInt32 input);
	void AttackPowerLowBonus( UInt32 input);
	void AttackScopeBonus( UInt32 input);
	void AttackSpeedBonus( UInt32 input);
	void BulletSpeedBonus( UInt32 input);
	void DefenceBonus( UInt32 input);
	void MDefenceBonus( UInt32 input);
	void CritRateBonus( UInt32 input);
	void HitRateBonus( UInt32 input);
	void DodgeRateBonus( UInt32 input);
	void MovSpeedBonus( UInt32 input);
	void StrengthBonus( UInt32 input);
	void IntelligenceBonus( UInt32 input);
	void AgilityBonus( UInt32 input);

	//基础属性的加减
	void AddLevel(Int32 input);
	void AddExp(Int32 input);
	void AddMaxExp(Int32 input);
	void AddAddPoint(Int32 input);
	void AddStrength(Int32 input);
	void AddIntelligence(Int32 input);
	void AddAgility(Int32 input);
	void AddHp(Int32 input);
	void AddMp(Int32 input);
	void AddMaxHp(Int32 input);
	void AddMaxMp(Int32 input);
	void AddHpRegen(Int32 input);
	void AddMpRegen(Int32 input);
	void AddAttackPowerHigh(Int32 input);
	void AddAttackPowerLow(Int32 input);
	void AddAttackScope(Int32 input);
	void AddAttackSpeed(Int32 input);
	void AddBulletSpeed(Int32 input);
	void AddDefence(Int32 input);
	void AddMDefence(Int32 input);
	void AddCritRate(Int32 input);
	void AddHitRate(Int32 input);
	void AddDodgeRate(Int32 input);
	void AddMoveSpeed(Int32 input);

	//加成属性的加减
	void AddMaxHpBonus(Int32 input);
	void AddMaxMpBonus(Int32 input);
	void AddHpRegenBonus(Int32 input);
	void AddMpRegenBonus(Int32 input);
	void AddAttackPowerHighBonus(Int32 input);
	void AddAttackPowerLowBonus(Int32 input);
	void AddAttackScopeBonus(Int32 input);
	void AddAttackSpeedBonus(Int32 input);
	void AddBulletSpeedBonus(Int32 input);
	void AddDefenceBonus(Int32 input);
	void AddMDefenceBonus(Int32 input);
	void AddCritRateBonus(Int32 input);
	void AddHitRateBonus(Int32 input);
	void AddDodgeRateBonus(Int32 input);
	void AddMoveSpeedBonus(Int32 input);
	void AddStrengthBonus(Int32 input);
	void AddIntelligenceBonus(Int32 input);
	void AddAgilityBonus(Int32 input);




protected:

	//线程锁
	pthread_mutex_t _mutexData;

	//一般属性
	AttrGeneral _attrGeneral;

	//基础战斗属性
	AttrCombat _attrCombat;

	//加成战斗属性
	AttrCombatBonus _attrCombatBonus;



};


#endif

