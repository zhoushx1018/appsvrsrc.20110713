/**
 *	换装 序列化用的类
 *	
 */

#ifndef ARCHVAVATAR_H
#define ARCHVAVATAR_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"



//avatar信息
class ArchvAvatarDesc
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvAvatarDesc()
	:roleId(0),equipIndex(0),itemID(0),entityID(0),durability(0),bindStatus(0)
	{}	
public:
	//成员变量
	UInt32 roleId;
	Byte equipIndex;
	UInt32 itemID;
	UInt32 entityID;
	UInt16 durability;
	Byte bindStatus;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(roleId)
		SERIAL_ENTRY(equipIndex)
		SERIAL_ENTRY(itemID)
		SERIAL_ENTRY(entityID)
		SERIAL_ENTRY(durability)
		SERIAL_ENTRY(bindStatus)
	END_SERIAL_MAP()

};

//简要avatar信息
class ArchvAvatarDescBrief
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvAvatarDescBrief()
	:roleId(0),proID(0),wpnItemID(0)
	,coatID(0)
	{}
	
public:
	//成员变量
	UInt32 roleId;
	Byte proID;
	UInt32 wpnItemID;		//武器 ItemID
	UInt32 coatID;					//

	
//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(roleId)
		SERIAL_ENTRY(proID)
		SERIAL_ENTRY(wpnItemID)
		SERIAL_ENTRY(coatID)
	END_SERIAL_MAP()

};
class ArchvroleEuip
	:public BaseArchive
{
	public:

	ArchvroleEuip()
		:BonusAttrID(0),Type(0),Value(0)
		{}
	public:
		UInt16 BonusAttrID;
		UInt16 Type;
		UInt32 Value;
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(BonusAttrID)
		SERIAL_ENTRY(Type)
		SERIAL_ENTRY(Value)
	END_SERIAL_MAP()
		
	
};
class ArchvroleBonuschange
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvroleBonuschange()
	:BonusAttrID(0),Num(0)
	{}
	
public:
	//成员变量
	UInt16 BonusAttrID;
	UInt32 Num;

	
//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(BonusAttrID)
		SERIAL_ENTRY(Num)
	END_SERIAL_MAP()

};



#endif

