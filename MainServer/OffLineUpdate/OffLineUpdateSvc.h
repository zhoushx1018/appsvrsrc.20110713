#ifndef OFFLINEUPDATESVC_H
#define OFFLINEUPDATESVC_H

#include "GWProxy.h"
#include "ConnectionPool.h"
#include "LuaState.h"
#include "IniFile.h"
#include "Serializer.h"
#include "ArchvBagItemCell.h"
#include "MainSvc.h"


class OffLineUpdateItem
{

public:
	//成员变量
	UInt32 UpdateID;
	UInt32 RoleID;
	string BegDate;
	Byte   IsFinish;
	Byte   MoneyType;
	UInt32 LockMoney;
	UInt32 BegTime;
	UInt32 DesSec;
	UInt32 DesExp;
	UInt32 DesLevel;
	UInt32 DesNum;
	UInt32 BilledNum;
	double BilledMoney;
	UInt32 FinishedSec;
	UInt32 SpeedUpSec;
};



class OffLineUpdateSvc;

class OffLineUpdateSvc
{
public:
	//构造函数
	OffLineUpdateSvc(void *sever,ConnectionPool *cp);

	//析构函数
	~OffLineUpdateSvc();

	//处理包
	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32 RetCode);

	void ProcessPacket(Session& session, Packet& packet);

	//客户端错误应答s
	void ClientErrorAck(Session& session, Packet& packet);


	//============================c_S Ack==================================

	//[MsgType:1601]升级数据查询
	void ProcessUpdateDataQuery(Session& session, Packet& packet);

    //[MsgType:1602]开始修炼升级
	void ProcessBeginUpdate(Session& session, Packet& packet);

	 //[MsgType:1603]金币加速挂机
	void ProcessSpeedupUpdate(Session& session, Packet& packet);

	//[MsgType:1604]停止修炼升级
	void ProcessStopUpdate(Session& session, Packet& packet);

	//[MsgType:1605] 剩余挂机的时间
	 void ProcessLeftUpdateHour(Session& session, Packet& packet);

	 //[MsgType:16099]前台触发扣费
	void ProcessLostBilling(Session& session, Packet& packet);


	 //=======================s-c ack===================
	 void OnRoleLoginNotify(UInt32 roleID);

	 //[MsgType:1601]挂机数据
	 void NotifyStopUpdate(UInt32 roleID,UInt32 desExp, UInt32 desLevl,UInt32 finishSec,UInt32 desSec);

	 //[MsgType:1602]加速挂机10分钟的扣费
	 void NotifySpeedUpCast(UInt32 roleID, UInt32 speedupCast);

public:

	//@brief	角色上线 补充扣费
	//@return 0 成功  非0 失败
	int AddBill( UInt32 roleID, UInt32& desExp, UInt32& desLevel, UInt32& finishSec, UInt32& desSec );

	//@brief 计算获得经验, 目标等级, 花费
    //@return	0 成功  非0 失败
    int CalcCostAndGainsBySilver( UInt32 RoleID, Byte hangType, double& costSum, UInt32& expSum, UInt32& desLevel);

	int CalcCostPerMinite(UInt32 RoleID, UInt32 inputLevel, UInt32 inputExp, double& retCost, UInt32& retAddExp, UInt32& retFinalLevel, UInt32& retFinalExp,Byte vipLevl);


	//角色是否有这么多金钱, 1 银币，2 金币。 返回 0有，1 没有，-1失败
	int IfHasSoMoney(UInt32 roleID, UInt32 money,Byte moneyType);

	//扣除并锁定金钱 ,0 成功，1 失败
	int TakeoffAndLockMoney(UInt32 roleID, UInt32 money,Byte moneyType,const OffLineUpdateItem& offHang);

private:

	//加速10分钟的扣费 返回 0 成功 非0 失败
	int SpeedTenMiniteCast(UInt32 roleID,UInt32 &speedTenMinute);

	//vip 挂机加成
	int VipExtraExp(UInt32 roleID,Byte vipLevl,UInt32 &tmpExp);

private:
	 MainSvc * _mainSvc;
	//IniFile _file;
	ConnectionPool *_cp;
};


#endif
