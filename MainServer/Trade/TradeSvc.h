
#ifndef TRADESVC_H
#define TRADESVC_H

#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"
#include "ArchvTrade.h"

#ifndef   BIND_ITEM
#define  BIND_ITEM 1399
#endif



class MainSvc;
 
class TradeSvc
{
public:
	TradeSvc(void* service, ConnectionPool * cp);
	~TradeSvc();
	//void SetService( MainSvc * svc );

 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);



	//===================主业务====================================
	
	
	 
	//请求交易[MsgType:1301]
	void ProcessRequestTrade(Session& session,Packet& packet);
	//S-C到对方，某个人给你发起邀请交易
	//对方收到S-C 有两个选择
	//a.接受，b,拒决

	//接受他人发起的交易[MsgType:1302]
	//回答后，s-c答案，某个人接受了你的邀请, 向两个人都发送，表示建立起来交易
	void ProcessAnswerTrade(Session& session,Packet& packet);
	
    //物品信息传送[MsgType:1303]
	void ProcessItemPast(Session& session,Packet& packet);//物品信息转发
	//传过来CELLIndex

    //金钱信息传送[MsgType:1305]
	void ProcessMoney(Session& session,Packet& packet);//金钱

	//锁定物品[MsgType:1304]
	void ProcessLockTrade(Session& session,Packet& packet);//锁定物品，锁定物品的时候最后发送一次物品信息，全部物品的信息
	//传送给对方，后台短暂存储，并且记录状态,暂时后台不短暂存储

	//交易完成[MsgType:1306]
	void ProcessTrade(Session& session,Packet& packet);//交易OK

	// 取消交易 [MsgType:1307]
	void ProcessCancelTrade(Session& session,Packet& packet);

	
	//==============================  S-C Ack ===============================================
	void NotifyRequest(UInt32 roleID,string Name,UInt32 toRoleID);
	void NotifyRequestResult(UInt32 roleID,string Name,UInt32 ToroleID,Byte type);	

	void NotifyToRoleBagItem(UInt32 roleID,UInt32 ID,ItemCell lic,Byte type);//给角色传送物品信息，只是一个信息而已
		
	void NotifyToRoleMoney(UInt32 roleID,UInt32 ID,UInt32 Money,UInt32 Gold);
	void NotifyLockTrade(UInt32 roleID,UInt32 toRoleID);
	void NotifyCancelTrade(UInt32 toRoleID,UInt32 roleID,string roleName,Byte result);

	void NotifyTradeOk(UInt32 toRoleID,Byte TradeResult,UInt32 RoleID,string RoleName);//S-C点击OK的情况


	//==================================
	//纯粹的金钱交易
	int TradeMoney(TradeItem& lic1,TradeItem& lic2);

	//角色是否在交易中 返回值 0 为在交易，1 正在交易， -1 失败
	int IsOnTrade(UInt32 roleID);

	//角色是否锁定了交易 返回值 0 未锁定，1 已经锁定, -1 失败
	int IsLockTrade(UInt32 roleID);

	//角色交易结束后，交易信息设置成初始化状态
	//返回值 0 成功，非 0 失败
	int InitTradeItem(UInt32 roleID);

	//test
	void Test(TradeItem trade,UInt32 roleID);
	

private:
	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;

};


#endif


