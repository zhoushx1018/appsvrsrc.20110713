//宠物类
#ifndef PET_H

#include "OurDef.h"
#include "Creature.h"
#define PET_H
#define AddAddPoint_Pet 1
#define Strength_ADDD(p) (p==1)?3:((p==2)?1:1)
#define Intelligence_ADDD(p) (p==1)?2:((p==2)?2:1)
#define Agility_ADDD(p) (p==1)?2:((p==2)?1:2)

class ConnectionPool;
class Pet
	:public Creature
{

public:
	Pet();
	~Pet();
public:
	//-------------------------初始化和入库
	int InitPetCache(UInt32 petID, ConnectionPool * cp );
	int DB2Cache(UInt32 petID, ConnectionPool * cp);
	int DB2CacheBonus(UInt32 petID, ConnectionPool * cp);
	
	int DB2Cache();
	int Cache2DB();
	int DB2CacheBonus();
	int Cache2Bonus();


	//属性加减

	//--------------------------------------------------------
	Int16 IfTheExpToMax();
	//判断经验是否到达上限
	void Pet_ADDLev();
	
	//人物的升级引起的角色属性变化
	void PetAddStrength(Int32 input);
	void PetAddIntelligence(Int32 input);
	void PetAddAgility(Int32 input);
	void PetAddStrengthBonus(Int32 input);
	void PetAddIntelligenceBonus(Int32 input);
	void PetAddAgilityBonus(Int32 input);
	Int32 PetExpAdd(Int32 input);

private:
	//所有成员
	UInt32 _petType;
	UInt32 _petkind;
	ConnectionPool* _cp;
	
};


#endif

