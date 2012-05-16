/**
 *	Npc 序列化用的类
 *	
 */

#ifndef ARCHVNPC_H
#define ARCHVNPC_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"






// Npc 信息
class ArchvNpcInfo
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvNpcInfo():npcID(0),mapID(0),posX(0),posY(0)
	{}
	
public:
	//成员变量
	UInt32	npcID;
	string	npcName;
	Byte		mapID;
	UInt16	posX;
	UInt16	posY;
	string	greeting;
	
//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(npcID)
		SERIAL_ENTRY(npcName)
		SERIAL_ENTRY(mapID)
		SERIAL_ENTRY(posX)
		SERIAL_ENTRY(posY)
	END_SERIAL_MAP()

};

#endif
