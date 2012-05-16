// S_S 业务处理  pk
//

#ifndef ACCOUNTSVCSS_H
#define ACCOUNTSVCSS_H

#include "SSServer.h"
#include "ConnectionPool.h"
#include <list>
#include "List.h"
#include "Role.h"

class MainSvc;
class Role;

class AccountSvcSS
{
public:

	AccountSvcSS(MainSvc * mainSvc, ConnectionPool *cp );
	~AccountSvcSS();

	void OnProcessPacket(Session& session,Packet& packet);

protected:

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);

	//------------S_S 交易-----------------------------------------

	//[MsgType:0301] Account注册
	void	ProcessAccountReg(Session& session,Packet& packet);

	//[MsgType:0302] Account登陆
	void	ProcessAccountLogin(Session& session,Packet& packet);

	//[MsgType:0303] 查询Account、Roles映射
	void	ProcessGetAccountRoles(Session& session,Packet& packet);

	//[MsgType:0304] 增加Role
	void	ProcessAddRole(Session& session,Packet& packet);

	//[MsgType:0305] 删除Role
	void	ProcessDelRole(Session& session,Packet& packet);

	//[MsgType:0306] 身份验证通知（在每次C_S登陆前发送）
	void	ProcessIdNotify(Session& session,Packet& packet);

	//[MsgType:0307] 防沉迷检查 
	void	ProcessPlayTimeCheck(Session& session,Packet& packet);

	//[MsgType:0308] 身份证注册
	void	ProcessIDCardReg(Session& session,Packet& packet);

	//[MsgType:0399] Session验证
	void	ProcessCheckSession(Session& session,Packet& packet);

	//------------子业务处理 -----------------------------------------
	
private:
	int SetProAttr( Byte proID, RolePtr& role );
	int AddRole( UInt32 accountID, const string& passwd, const string& roleName, Byte proID, UInt32& roleID );

private:
	MainSvc * _mainSvc;
	ConnectionPool *_cp;
};


#endif

