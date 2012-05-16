/**
 *	任务 序列化用的类
 *
 */

#ifndef ARCHVTASK_H
#define ARCHVTASK_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"


//简要任务信息
class ArchvTaskInfoBrief
	:public BaseArchive
{

public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvTaskInfoBrief():taskID(0),taskType(0),taskStatus(0),adviceLevel(0)
	{}
	virtual ~ArchvTaskInfoBrief(){}

public:
	//成员变量
	UInt32	taskID;
	string	taskName;
	Byte		taskType;
	Byte		taskStatus;
	Byte		adviceLevel;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(taskID)
		SERIAL_ENTRY(taskName)
		SERIAL_ENTRY(taskType)
		SERIAL_ENTRY(taskStatus)
		SERIAL_ENTRY(adviceLevel)

	END_SERIAL_MAP()

};



//任务明细项
class ArchvTaskDetail
	:public BaseArchive
{

public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvTaskDetail():goalType(0),goalID(0),isFinish(0)
		,goalNum(0),finishNum(0)
	{}
	virtual ~ArchvTaskDetail(){}

public:
	//成员变量
	Byte		goalType;
	UInt32	goalID;
	Byte		isFinish;
	UInt32	goalNum;
	UInt32	finishNum;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(goalType)
		SERIAL_ENTRY(goalID)
		SERIAL_ENTRY(isFinish)
		SERIAL_ENTRY(goalNum)
		SERIAL_ENTRY(finishNum)
	END_SERIAL_MAP()

};


//任务信息
class ArchvTaskInfo
	:public BaseArchive
{

public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvTaskInfo():taskID(0),taskType(0),taskStatus(0)
	{}
	virtual ~ArchvTaskInfo(){}

public:
	//成员变量
	UInt32	taskID;
	string	taskName;
	Byte		taskType;
	Byte		taskStatus;
	List<ArchvTaskDetail> ltd;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(taskID)
		SERIAL_ENTRY(taskName)
		SERIAL_ENTRY(taskType)
		SERIAL_ENTRY(taskStatus)
		SERIAL_ENTRY(ltd)
	END_SERIAL_MAP()

};

class ArchvUnfinishedTask
	:public BaseArchive
{
 public:
 	ArchvUnfinishedTask():roleID(0)
 		{}
 	virtual ~ArchvUnfinishedTask(){}

 public:

	UInt32 roleID;
	list<UInt32>licTaskID;

	BEGIN_SERIAL_MAP()
	  SERIAL_ENTRY(roleID)
	END_SERIAL_MAP()
};

#endif
