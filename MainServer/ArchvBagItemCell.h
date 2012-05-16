/**
 *	ItemCell
 *	
 */

#ifndef ARCHVBAGITEM_H
#define ARCHVBAGITEM_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"


//背包物品信息
class ItemCell
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ItemCell():celIndex(0),ItemID(0),EntityID(0),cdTime(0),num(0),durability(0),bindStatus(0)
	{}
	
public:
	//成员变量
	UInt16 celIndex;
	UInt32 ItemID;
	UInt32 	EntityID;
	UInt16 cdTime;
	UInt16 num;
	UInt16 durability;
	Byte bindStatus;
	

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(celIndex)
		SERIAL_ENTRY(ItemID)
		SERIAL_ENTRY(EntityID)
		SERIAL_ENTRY(cdTime)
		SERIAL_ENTRY(num)
		SERIAL_ENTRY(durability)	
		SERIAL_ENTRY(bindStatus)
	END_SERIAL_MAP()

};
class ItemList
	:public BaseArchive
{
	public:
		//成员变量赋初值
		ItemList():ItemID(0),num(0)
		{}
		
	public:
		//成员变量
		UInt32 ItemID;
		UInt16 num;
	//成员变量序列化操作，Serializer类将按字段顺序进行序列化
		BEGIN_SERIAL_MAP()
			SERIAL_ENTRY(ItemID)
			SERIAL_ENTRY(num)
		END_SERIAL_MAP()

};




#endif
