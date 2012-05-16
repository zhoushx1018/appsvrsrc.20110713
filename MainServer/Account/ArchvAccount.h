/**
 *	账号 序列化用的类
 *	
 */

#ifndef ARCHVACCOUNT_H
#define ARCHVACCOUNT_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "List.h"

//角色移动描述
class ArchvAccountRoleInfo
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvAccountRoleInfo():roleID(0),proID(0),level(0)
	{
	}
	
	virtual ~ArchvAccountRoleInfo(){}

public:
	//成员变量
	UInt32				roleID;
	string				roleName;
	Byte					proID;
	UInt32				level;
	
	
//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
		SERIAL_ENTRY(roleID)
		SERIAL_ENTRY(roleName)
		SERIAL_ENTRY(proID)
		SERIAL_ENTRY(level)
	END_SERIAL_MAP()

};



#endif


