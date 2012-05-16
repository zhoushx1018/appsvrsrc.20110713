/**
 *	Mail 序列化用的类
 *	
 */

#ifndef ARCHVMAIL_H
#define ARCHVMAIL_H

#include "Serializer.h"
#include "BaseArchive.h"
#include "DebugData.h"
#include "List.h"


//


class ArchvMailQueryItem
	  :public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvMailQueryItem()
	:MailID(0),IsRead(0),MailType(2),\
	Money(0),ItemID(0),Num(0),Durability(0)
	{}
	
public:
	//成员变量
   UInt32	MailID;			//邮件ID
   Byte     MailType;       //邮件类型 1 系统邮件，2 用户邮件
   Byte     IsRead;         //是否阅读  
   string   SendRoleName;	//发件人RoleID
   string	SendTime;		//发送时间
   string   Content;	    //邮件内容
   UInt32   Money;		    //附件金钱数
   Byte     MoneyType;      //金钱类型
   UInt32   ItemID;			//物品id
   UInt16   Num;			//物品数量
   UInt16   Durability;		//耐久度
  

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
	SERIAL_ENTRY(MailID)
	SERIAL_ENTRY(MailType)
	SERIAL_ENTRY(IsRead)
	SERIAL_ENTRY(SendRoleName)
	SERIAL_ENTRY(SendTime)
	SERIAL_ENTRY(Content)
	SERIAL_ENTRY(Money)
	SERIAL_ENTRY(MoneyType)
	SERIAL_ENTRY(ItemID)
	SERIAL_ENTRY(Num)
	SERIAL_ENTRY(Durability)
	END_SERIAL_MAP()

};

class ArchvSystemMailItem
	  :public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvSystemMailItem()
	:Money(0),MoneyType(0),ItemID(0),Num(0),EntityID(0)
	{}
	
public:
	//成员变量
   string   RecvRoleName;   //收件人姓名
   string   Content;        //内容
   UInt32   Money;		    //附件金钱数
   Byte     MoneyType;      //金钱类型 1 银币， 2 金币
   UInt32   ItemID;			//物品id
   UInt16   Num;			//物品数量
   UInt32   EntityID;       //实物ID
  

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
	SERIAL_ENTRY(RecvRoleName)
	SERIAL_ENTRY(Content)
	SERIAL_ENTRY(Money)
	SERIAL_ENTRY(MoneyType)
	SERIAL_ENTRY(ItemID)
	SERIAL_ENTRY(Num)
	SERIAL_ENTRY(EntityID)
	END_SERIAL_MAP()

};

class ArchvMailItem
	:public BaseArchive
{
	
public:
	//成员变量赋初值
	//	非 string类型、非List模板类型的成员变量，建议都赋上初值
	ArchvMailItem()
	:mailID(0),recvRoleID(0),sendRoleID(0),
	money(0),itemID(0),num(0),entityID(0)
	{}
	
public:
	//成员变量
	UInt32 mailID;
	UInt32 recvRoleID;
	UInt32 sendRoleID;
	string sendTime;
	UInt32 money;
	UInt16 cellIndex;
	UInt32 itemID;
	UInt32 num;
	UInt32 entityID;
	string Content;

//成员变量序列化操作，Serializer类将按字段顺序进行序列化
	BEGIN_SERIAL_MAP()
	SERIAL_ENTRY(mailID)
	SERIAL_ENTRY(recvRoleID)
	SERIAL_ENTRY(sendRoleID)
	SERIAL_ENTRY(sendTime)
	SERIAL_ENTRY(money)
	SERIAL_ENTRY(cellIndex)
	SERIAL_ENTRY(itemID)
	SERIAL_ENTRY(num)
	SERIAL_ENTRY(entityID)
	SERIAL_ENTRY(Content)
	END_SERIAL_MAP()

};




#endif

