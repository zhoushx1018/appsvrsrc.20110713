#ifndef MAILSVC_H
#define MAILSVC_H

#include "GWProxy.h"
#include "ConnectionPool.h"
#include "LuaState.h"
#include "IniFile.h"
#include "Serializer.h"
#include "ArchvMail.h"
#include "ArchvBagItemCell.h"



#ifndef MAIL_MAX_NUM
#define MAIL_MAX_NUM 101   //邮件总数
#endif

#ifndef PER_MAIL_COST 
#define PER_MAIL_COST 100  //每封邮件扣除的金钱
#endif

#ifndef MAX_MAIL_CONTENT
#define MAX_MAIL_CONTENT 600    //邮件内容最大字符数
#endif

#ifndef MAIL_SAVE_TIME
#define MAIL_SAVE_TIME 100000000       //邮件保存时间
#endif


#ifndef ERROR_LACK_MONTY
#define ERROR_LACK_MONTY 2001
#endif

#ifndef ERROR_MAIL_FULL 
#define ERROR_MAIL_FULL 2002
#endif

#ifndef ERROR_ITEM_NOT_FIND
#define ERROR_ITEM_NOT_FIND 2003
#endif

class MainSvc;

class MailSvc
{
public:
	//构造函数	
	MailSvc(void *sever,ConnectionPool *cp);
	
	//析构函数
	~MailSvc();

	void OnProcessPacket(Session& session,Packet& packet);
	//处理包
	void ClientErrorAck(Session& session, Packet& packet, UInt32 RetCode);
	
	void ProcessPacket(Session& session, Packet& packet);

	//客户端错误应答s
	void ClientErrorAck(Session& session, Packet& packet);

	//[MsgType:1501]邮件查询
	void ProcessGetRoleMails(Session& session, Packet& packet);

    //[MsgType:1502]发送邮件
	void ProcessSendRoleMails(Session& session, Packet& packet);

     //[MsgType:1503]删除邮件
	void ProcessDeleteRoleMails(Session& session, Packet& packet);

	 //[MsgType:1504]附件移到背包
	void ProcessAttachToBag(Session& session, Packet& packet);
	 
	//系统邮件，金钱类型 1 银币(非绑定铜币)，2 金币(元宝)
	//返回 0 成功，非 0 失败
	int OnSendSystemMail(ArchvSystemMailItem sysMail);
	 //===================== s - c  ack =================================
	 void NotifyNewMail(UInt32 roleID,ArchvMailQueryItem &newMail);

private:
	int InitMoneyAttach(UInt32 &mailid, UInt32 &roleid);
	int InitItemAttach(UInt32 &mailid, UInt32 &roleid);


private:
	
	MainSvc * _mainSvc;
	//IniFile _file;
	ConnectionPool *_cp;
};


#endif

