/**
 *	Friend 序列化用的类
 *	
 */

#ifndef ARCHVFRIEND_H
#define ARCHVFRIEND_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"


//
class ArchvFriend
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvFriend():friendType(0),IsOnline(0)
	{}
	
public:
	//成员变量
	string friendName;
	UInt32 friendRoleID;
	Byte friendType; // 1 好友，2 仇人，3 黑名单
	Byte IsOnline;   //是否在线 1 是，0 否

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
	SERIAL_ENTRY(friendName)
	SERIAL_ENTRY(friendRoleID)
	SERIAL_ENTRY(friendType)
	SERIAL_ENTRY(IsOnline)
	END_SERIAL_MAP()

};
class ArchvFriendAddPro
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvFriendAddPro():friendType(0),IsOnline(0),ProID(0)
	{}
	
public:
	//成员变量
	string friendName;
	UInt32 friendRoleID;
	Byte friendType; // 1 好友，2 仇人，3 黑名单
	Byte IsOnline;   //是否在线 1 是，0 否
	Byte ProID;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
	SERIAL_ENTRY(friendName)
	SERIAL_ENTRY(friendRoleID)
	SERIAL_ENTRY(friendType)
	SERIAL_ENTRY(IsOnline)
	SERIAL_ENTRY(ProID)
	END_SERIAL_MAP()

};


#endif

