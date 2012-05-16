/**
 *	背包 序列化用的类
 *	
 */

#ifndef DBBAG_H
#define DBBAG_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"


//背包物品信息
class DBBagItem:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	DBBagItem():RoleID(0),CellIndex(0),CellType(0),ItemType(0),ItemID(0),EntityID(0),NUM(0)
	{}
	
	virtual ~DBBagItem(){}

public:
	//成员变量
	UInt32 RoleID;
	UInt16 CellIndex;
	UInt16 CellType;
	UInt16 ItemType;
	UInt32 ItemID;
	UInt32 	EntityID;
	UInt32 NUM;
	
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(RoleID)
		SERIAL_ENTRY(CellIndex)
		SERIAL_ENTRY(CellType)
		SERIAL_ENTRY(ItemID)
		SERIAL_ENTRY(EntityID)
		SERIAL_ENTRY(NUM)
	END_SERIAL_MAP()
	

//成员变量序列化操作，Serializer类将按字段顺序进行序列化

};

class ItemMapTravelPoint
{
	public:
		ItemMapTravelPoint():MapID(0),X(0),Y(0)
		{}
	public:
		UInt32 MapID;
		UInt16 X;
		UInt16 Y;
		
};
	
class EquipItem
{
	public:
		EquipItem():ItemID(0),EntityID(0)
				{}
	public:
		UInt32 ItemID;
		UInt32 EntityID;
};


//装备耐久度变化(装备损耗)
class ArchvEquipDurability:public BaseArchive
{

  public:
  	 ArchvEquipDurability():EquipIndex(0),ItemID(0),EntityID(0),Durability(0)
  	 {}

  	virtual ~ArchvEquipDurability(){}
	 
  public:
  	 Byte   EquipIndex;
	 UInt32 ItemID;
	 UInt32 EntityID;
	 UInt32 Durability;


	 BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(EquipIndex)
		SERIAL_ENTRY(ItemID)
		SERIAL_ENTRY(EntityID)
		SERIAL_ENTRY(Durability)
	END_SERIAL_MAP()
};

#endif
