#ifndef ARCHVSHOP_H
#define ARCHVSHOP_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"


//简要任务信息
class ArchvShopItem
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvShopItem():ItemID(0),Category(0),NowPrice(0),Num(0)
	{}
	
public:
	//成员变量
	UInt32	ItemID;
	Byte		Category;
	UInt16		NowPrice;
	UInt16      Num;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(ItemID)
		SERIAL_ENTRY(Category)
		SERIAL_ENTRY(NowPrice)
		SERIAL_ENTRY(Num)
		
	END_SERIAL_MAP()

};

class SpecialItem 
	:public BaseArchive

{
	public:
		SpecialItem():ItemID(0),NowPrice(0),Num(0)
			{}
	public:
		UInt32 ItemID;
		UInt16 NowPrice;
		UInt16 Num;
		
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(ItemID)
		SERIAL_ENTRY(NowPrice)
		SERIAL_ENTRY(Num)
		
	END_SERIAL_MAP()
};





#endif

