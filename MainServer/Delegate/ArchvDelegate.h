/**
 *	Delegate 序列化用的类
 *	
 */

#ifndef ARCHVDELEGATE_H
#define ARCHVDELEGATE_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"


class ArchvRoleDelegateInfo
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvRoleDelegateInfo()
	:taskID(0),maxDelegateNum(0),delegateTime(0),
	delegateCost(0),isStarted(0),finishNum(0),elapseTime(0)
	{}
	
public:
	//成员变量
	UInt32 taskID;
	UInt16 maxDelegateNum;
	UInt32 delegateTime;
	UInt32 delegateCost;
	Byte	 isStarted;
	UInt16 finishNum;
	UInt32 elapseTime;
	
//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
	SERIAL_ENTRY(taskID)
	SERIAL_ENTRY(maxDelegateNum)
	SERIAL_ENTRY(delegateTime)
	SERIAL_ENTRY(delegateCost)
	SERIAL_ENTRY(isStarted)
	SERIAL_ENTRY(finishNum)
	SERIAL_ENTRY(elapseTime)
	END_SERIAL_MAP()

};



#endif

