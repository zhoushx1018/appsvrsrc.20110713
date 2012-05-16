/**
 *	npc 对话项类
 *	与一般的 BaseArchive 子类定义、处理有所不同
 */

#ifndef ARCHVDIALOGITEM_H
#define ARCHVDIALOGITEM_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"

#include "ArchvTask.h"
#include "OurDef.h"


class ArchvDialogItem
	:public BaseArchive
{
public:
	ArchvDialogItem();
	
//	ArchvDialogItem(Byte type,void* data);

	ArchvDialogItem(Byte type, UInt32 input);

	ArchvDialogItem(Byte type, const ArchvTaskInfoBrief &input);

	~ArchvDialogItem();

	void FreeData();

	Serializer& Serialize(Serializer &serializer,bool isLoading);

private:
	unsigned char _type;
//	void* _data;

	UInt32	_valueID;
	ArchvTaskInfoBrief	_taskInfoBrief;
	
	
};


#endif
