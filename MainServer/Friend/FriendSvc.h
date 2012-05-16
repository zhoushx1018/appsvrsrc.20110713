#ifndef FRIENDSVC_H
#define FRIENDSVC_H
#include "GWProxy.h"
#include "ConnectionPool.h"
#include "LuaState.h"
#include "IniFile.h"
#include "Serializer.h"
#include "ArchvFriend.h"

#ifndef FRIEND_TYPE_NON_CHANGE
#define FRIEND_TYPE_NON_CHANGE 3001
#endif

#ifndef MAX_CHAT_CONTENT
#define MAX_CHAT_CONTENT 300
#endif

#ifndef MAX_FRIEND_NUM
#define MAX_FRIEND_NUM 40
#endif

class MainSvc;

class FriendSvc
{
public:
	//构造函数
	FriendSvc(void *sever,ConnectionPool *cp);

	//析构函数
	~FriendSvc();

	void OnProcessPacket(Session& session,Packet& packet);
	//处理包
	void ClientErrorAck(Session& session, Packet& packet, UInt32 RetCode);

	void ProcessPacket(Session& session, Packet& packet);

	//客户端错误应答s
	void ClientErrorAck(Session& session, Packet& packet);

	//[MsgType:1201]查询好友
	void ProcessGetFriend(Session& session, Packet& packet);

	//[MsgType:1202]添加好友
	void ProcessAddFriend(Session& session, Packet& packet);

	//[MsgType:1203]删除好友
	void ProcessDeleteFriend(Session& session, Packet& packet);

	//[MsgType:1204]修改好友类型
	void ProcessReviseFriendType(Session& session, Packet& packet);

	//[MsgType:1205]同意增加好友
	void ProcessAgreeFriend(Session& session, Packet& packet);

	//[MsgType:1206]好友私聊
    void ProcessFriendPrivateChat(Session& session, Packet& packet);


	void OnFriendOnLine(UInt32 roleID);
	void OnFriendOffLine(UInt32 roleID);

	//============================ s - c ack ============================

	// [MsgType:1201] 请求加为好友
	void NotifyFriendRole(UInt32 friendRoleID, string&roleName);

	// [MsgType:1202] 删除好友成功
	void NotifyDeleteFriend(UInt32 RoleID,UInt32 frinedID,Byte friendType);

	// [MsgType:1203] 添加好友成功
	void NotifyAddFriendSuccessd(UInt32 RoleID, ArchvFriendAddPro &friendItem);

	// [MsgType:1204]修改好友类型
	void NotifyAlterFriendType(UInt32 RoleID, ArchvFriendAddPro &friendItem);

	// [MsgType:1205]好友私聊
	void NotifyFriendPrivateChat(UInt32 friendRoleID,UInt32 roleID,string& name,string &chatContent);

	// [MsgType:1206]好友上线、下线
	void NotifyFriendOnAndOffLine(UInt32 friendRoleID,UInt32 roleID,Byte friendState);


private:

	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;
};

#endif

