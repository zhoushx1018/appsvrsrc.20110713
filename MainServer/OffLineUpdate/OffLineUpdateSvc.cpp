#include "OffLineUpdateSvc.h"
#include "DBOperate.h"
#include "OffLineUpdateSvc.h"
#include "Role.h"
#include "CoreData.h"
#include "../Bag/BagSvc.h"
#include "DateTime.h"
#include <math.h>
#include <time.h>



OffLineUpdateSvc::OffLineUpdateSvc(void *service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;

}

OffLineUpdateSvc::~OffLineUpdateSvc()
{
	//
}

//数据包信息
void OffLineUpdateSvc::OnProcessPacket(Session& session,Packet& packet)
{
	DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 1601: //[MsgType:1601]升级数据查询
			ProcessUpdateDataQuery(session, packet);
		break;

    case 1602: //[MsgType:1602]开始修炼升级
			ProcessBeginUpdate(session,  packet);
			break;

		case 1603: //[MsgType:1603]金币加速挂机
	    ProcessSpeedupUpdate(session, packet);
		  break;

		case 1604: //[MsgType:1604]停止修炼升级
	    ProcessStopUpdate(session, packet);
		  break;

		case 1605:  //[MsgType:1605] 剩余挂机的时间
	    ProcessLeftUpdateHour(session, packet);
	    break;

		case 1699: //[MsgType:1699]前台触发扣费
      ProcessLostBilling(session, packet);
      break;

		default:
		ClientErrorAck(session,packet,ERR_SYSTEM_PARAM);
		LOG(LOG_ERROR,__FILE__,__LINE__,"MsgType[%d] not found",packet.MsgType);
		break;
	}
}

//客户端错误应答
//@param  session 连接对象
//@param	packet 请求包
//@param	RetCode 返回errorCode 值
void OffLineUpdateSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
{
	//组应答数据
	DataBuffer	serbuffer(1024);
	Packet p(&serbuffer);
	Serializer s(&serbuffer);
	serbuffer.Reset();
	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();
	s<<RetCode;
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF1( "C_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

}

//角色是否有这么多金钱, 1 绑定银，2 金币。
//返回 0成功，-1 失败 ,1 没有
int OffLineUpdateSvc::IfHasSoMoney(UInt32 roleID, UInt32 money,Byte moneyType)
{
  char szSql[1024];
  char szSql1[1024];
  char szTemp[50];
  char szMoney[50];
  int iRet;
	UInt32 roleMoney;
  Connection con;
	DBOperate dbo;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	if(1== moneyType)	//银币
	{
		sprintf(szSql,"select %s from RoleMoney where RoleID = %d;","Money",roleID);
		sprintf(szSql1,"select %s from RoleMoney where RoleID = %d;","BindMoney",roleID);

		iRet = dbo.QuerySQL(szSql);
		if(iRet < 0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"DB operate failed !");
			return -1;
		}

		roleMoney = dbo.GetIntField(0);

		iRet = dbo.QuerySQL(szSql1);
		if(iRet < 0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"DB operate failed !");
			return -1;
		}
		roleMoney += dbo.GetIntField(0);
	}
	if(2 == moneyType) //金币
	{
		sprintf(szSql,"select %s from RoleMoney where RoleID = %d;","Gold",roleID);

		iRet = dbo.QuerySQL(szSql);

		if(iRet < 0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"DB operate failed !");
			return -1;
		}
		roleMoney = dbo.GetIntField(0);
	}

	if (iRet == 0) {
		if (roleMoney < money) {
			return -1;
		}
	}

	return 0;
}

//扣除并锁定金钱
//返回值 0 成功，1 失败
int OffLineUpdateSvc::TakeoffAndLockMoney(UInt32 roleID, UInt32 money,Byte moneyType,const OffLineUpdateItem& offHang)
{
	char szSql[1024];
	UInt32	RetCode = 0;
  int iRet;
  char szTemp[100];
	Connection con;
	DBOperate dbo;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	LOG(LOG_ERROR,__FILE__,__LINE__,"money[%d]",money);

	if(1 == moneyType)
	{
		sprintf(szSql,"select %s from RoleMoney where RoleID = %d;","BindMoney",roleID);

		iRet = dbo.QuerySQL(szSql);
		if(iRet < 0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"DB operate failed !");
			return -1;
		}

		int bindMoney = dbo.GetIntField(0);
		int decreaseMoney = bindMoney - money;
		if(bindMoney >= money){
			  sprintf(szSql,"update RoleMoney set \
			                 BindMoney = %d where RoleID = %d;",decreaseMoney,roleID);

				iRet = dbo.ExceSQL(szSql);
				if(0 != iRet)
				{
					LOG(LOG_ERROR,__FILE__,__LINE__,"DB operate failed !");
					return 1;
				}
		}else{
			sprintf(szSql,"update RoleMoney set \
						                 BindMoney = %d where RoleID = %d;",0,roleID);
			iRet = dbo.ExceSQL(szSql);
			if(0 != iRet)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"DB operate failed !");
				return 1;
			}

			sprintf(szSql,"update RoleMoney set \
						                 Money = Money + %d where RoleID = %d;",decreaseMoney,roleID);
			iRet = dbo.ExceSQL(szSql);
			if(0 != iRet)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"DB operate failed !");
				return 1;
			}

		}

	}
	if(2 == moneyType)
	{
	  sprintf(szSql,"update RoleMoney set\
	                 Gold = Gold - %d where RoleID = %d;",money,roleID);
	  	//LOG(LOG_ERROR,__FILE__,__LINE__,"szSql[%s]",szSql);
		iRet = dbo.ExceSQL(szSql);
		if(0 != iRet)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"DB operate failed !");
			return 1;
		}
	}

	//LOG(LOG_ERROR,__FILE__,__LINE__,"offHang.BilledMoney[%f]",offHang.BilledMoney);

	sprintf(szSql,"insert OffLineUpdate(RoleID,\
										BegDate,\
										IsFinish,\
	                                    MoneyType,\
	                                    LockMoney,\
	                                    BegTime,\
	                                    DesSec,\
	                                    DesExp,\
	                                    DesLevel,\
	                                    DesNum,\
	                                    BilledNum,\
	                                    BilledMoney,\
	                                    FinishedSec,\
	                                    SpeedUpSec)\
	                                    value(%d,'%s',%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%d,%d);",\
	                                        offHang.RoleID,\
	                                        offHang.BegDate.c_str(),\
	                                        offHang.IsFinish,\
	                                        offHang.MoneyType,\
	                                        offHang.LockMoney,\
	                                        offHang.BegTime,\
	                                        offHang.DesSec,\
	                                        offHang.DesExp,\
	                                        offHang.DesLevel,\
	                                        offHang.DesNum,\
	                                        offHang.BilledNum,\
	                                        offHang.BilledMoney,\
	                                        offHang.FinishedSec,\
	                                        offHang.SpeedUpSec\
	                                      	);

  iRet = dbo.ExceSQL(szSql);
  if(0 != iRet)
  {
  	LOG(LOG_ERROR,__FILE__,__LINE__,"DB operate failed !");
		return 1;
  }

  return RetCode;

}

//[MsgType:1601]升级数据查询
void OffLineUpdateSvc::ProcessUpdateDataQuery(Session& session, Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	UInt32 roleID= packet.RoleID;

	Byte hangType,moneyType;

	int iRet;
	UInt32 silver = 0,gold = 0;
	UInt32 speedCast = 0;
	double costSum = 0.0;
	UInt32 expSum = 0, desLevel = 0;

 	double tmpCost = 0.0;
	UInt32 tmpExp = 0,tmpLevel = 0;


	//序列化类
	Serializer s(packet.GetBuffer());
  s>>hangType>>moneyType;
	if( s.GetErrorCode()!= 0)
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}


	 //LOG(LOG_ERROR,__FILE__,__LINE__,"===============UpdateDataQuery===============");
	 //LOG(LOG_ERROR,__FILE__,__LINE__,"hangType[%d]",hangType);
	 //LOG(LOG_ERROR,__FILE__,__LINE__,"moneyType[%d]",moneyType);

	//根据当前等级,经验,挂机类型
	//	计算预计获得经验, 目标等级,扣除的总银币,总金币
	//	如果是金币,获得经验有加成

	iRet = CalcCostAndGainsBySilver( roleID, hangType, costSum, expSum,desLevel);
	if(iRet)
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"CalcCostAndGainsBySilver error !" );
		goto EndOf_Process;
	}

	silver = (int)(ceil(costSum));
	gold = (int)(ceil(costSum / 300 * 2));

	LOG(LOG_ERROR,__FILE__,__LINE__,"gold[%d]",gold);

  if(moneyType == 2)
  {
     expSum += expSum / 10;
  }

	//加速10分钟的扣费
	iRet = CalcCostAndGainsBySilver( roleID, 1, tmpCost, tmpExp,tmpLevel);
	if(iRet)
	{
	  RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"CalcCostAndGainsBySilver error !" );
		goto EndOf_Process;
	}

	//speedCast = (int)(ceil(tmpCost / 300 * 5 /3));
		speedCast = (int)ceil(tmpCost / 300 * 2 * 5 /6);

	//LOG(LOG_ERROR,__FILE__,__LINE__,"expSum[%d]",expSum);
	//LOG(LOG_ERROR,__FILE__,__LINE__,"desLevel[%d]",desLevel);
	LOG(LOG_ERROR,__FILE__,__LINE__,"speedCast[%d]",speedCast);

EndOf_Process:
	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();
	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();

	LOG(LOG_ERROR,__FILE__,__LINE__,"UpdateQuery: RetCode[%d]",RetCode);
	s<<RetCode;
	if( 0 == RetCode )
	{
		//RetCode 为0 才会返回包体剩下内容
		s<<expSum;
		s<<desLevel;
		s<<silver;
		s<<gold;
		s<<speedCast;
	}
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
}

//[MsgType:1602]开始修炼升级
void OffLineUpdateSvc::ProcessBeginUpdate(Session& session, Packet& packet)
{
	char szSql[1024];
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	UInt32 roleID= packet.RoleID, Money = 0,totalTime;
	int iRet;
	DateTime nowTime,tmpDate;
	string strDate;
	Byte hangType, moneyType;

	OffLineUpdateItem offupdate;
	UInt32 curLevel;
	double costSum = 0.0;
	UInt32 expSum,desLevel,cost = 0;
	double billedMoney = 0.0;
	UInt32 tmpExp, tmpLevel,exp;
	double tmpCost;
	string strTime;
	Byte isFinish = 0,offlineState = 0;

	Connection con;
	DBOperate dbo;


	//序列化类
	Serializer s(packet.GetBuffer());
    s>>hangType>>moneyType;

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr( roleID);
	LOG(LOG_DEBUG,__FILE__,__LINE__,"hangtype[%d]---moneytype[%d]",hangType,moneyType);
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}


	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//判断级别
	//	10级以下不允许挂机
	if(0 == pRole->ID())
	{
	 	RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error! roleID[%d]", roleID );
		goto EndOf_Process;
	}

	if(pRole->Level() <= 10)
	{
	  RetCode = ERR_ROLE_LEVELNOTENOUGH;
		LOG(LOG_ERROR,__FILE__,__LINE__," curLevel <= 10 !" );
		goto EndOf_Process;
	}


	strTime = nowTime.Now().StringDate();
    sprintf(szSql,"select IsFinish from OffLineUpdate \
  	               where RoleID = %d and BegDate = '%s';",roleID,strTime.c_str());
    iRet = dbo.QuerySQL(szSql);
    if(iRet == 0)
    {
	  while(dbo.HasRowData())
	  {
		isFinish = dbo.GetIntField(0);
		if(isFinish == 1)
		{
			offlineState = 0;
		}
		else
		{
			offlineState = 1;
		}
		dbo.NextRow();
	  }
    }
    else if(iRet == 1)
    {
  	   offlineState = 0;
    }
    else
    {
	   RetCode = ERR_APP_OP;
  	   LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error !" );
  	   goto EndOf_Process;
    }

    LOG(LOG_ERROR,__FILE__,__LINE__,"offlineState[%d]",offlineState);
    if(offlineState == 0)
    {
		pRole->IsOfflineUpdate(0);
    }


	//开始挂机的开关是否开启
	if(pRole->IsOfflineUpdate())
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"role.IsOfflineUpdate() is true ");
		goto EndOf_Process;
	}

  //今天修炼的总时间是否超过24小时
	strDate = tmpDate.Now().StringDate();

	sprintf(szSql,"select ifnull( sum(FinishedSec) ,0),ifnull(sum(SpeedUpSec),0)\
								 from OffLineUpdate \
	               where RoleID = %d and BegDate = '%s';",roleID ,strDate.c_str());
	iRet = dbo.QuerySQL(szSql);
	if(iRet < 0)
	{
	  RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
	if(1 == iRet)
	{
	  totalTime = 0;
	}

	totalTime = dbo.GetIntField(0) + dbo.GetIntField(1);
	//LOG(LOG_ERROR,__FILE__,__LINE__,"totalTime[%d]",totalTime);

	//判断剩余时间是否大于挂机时间
	totalTime += hangType * 3600;

	if(totalTime > 3600 * 24)
	{
	 	RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"totalTime > 24h");
		goto EndOf_Process;
	}

  //查看是否有其他日期的修炼记录,如果有则删除
  strTime = nowTime.Now().StringDate();
  //LOG(LOG_ERROR,__FILE__,__LINE__,"strTime[%s]" , strTime.c_str());
	sprintf(szSql,"delete from OffLineUpdate \
	       					where RoleID = %d \
	       					and BegDate <> '%s' ;",roleID,strTime.c_str());
	iRet = dbo.ExceSQL(szSql);
	if(iRet)
	{
		RetCode = ERR_SYSTEM_DBERROR;
    LOG(LOG_ERROR,__FILE__,__LINE__,"DB OP Failed !");
		goto EndOf_Process;
	}

  //获取目标级数、经验、扣费
  iRet = CalcCostAndGainsBySilver( roleID, hangType, costSum, expSum,desLevel);
  if(iRet)
  {
		RetCode = ERR_APP_OP;
    LOG(LOG_ERROR,__FILE__,__LINE__,"CalcCostAndGainsBySilver Failed !");
		goto EndOf_Process;
  }

  //加成及其他计算
  if(1 == moneyType)
  {
		cost = (int)ceil(costSum);			//四舍五入
  }
  else if(2 == moneyType)
  {
		expSum = expSum + expSum / 10;  //金币挂机经验有加成
		cost = (int)ceil(costSum / 300 * 2 );		//四舍五入
  }
  else
  {
  	RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"moneyType error!moneyType[%d]! ", moneyType );
		goto EndOf_Process;
  }

   LOG(LOG_ERROR,__FILE__,__LINE__,"cost[%d]",cost);
  //金钱是否足够
	iRet = IfHasSoMoney(roleID, cost, moneyType);
	if(iRet)
	{
		RetCode = ERR_ROLE_NOMONEY;
		LOG(LOG_ERROR,__FILE__,__LINE__,"You have no so much money[%d]",cost);
		goto EndOf_Process;
	}

	//扣除并锁定金钱
	offupdate.RoleID = roleID;
  offupdate.BegDate = strTime;
  offupdate.IsFinish = 0;
  offupdate.MoneyType = moneyType;
  offupdate.LockMoney = cost;
  offupdate.BegTime = time(NULL);
  offupdate.DesSec = hangType * 3600;
  offupdate.DesExp = expSum;
  offupdate.DesLevel = desLevel;
  offupdate.DesNum = hangType * 60;
  offupdate.BilledNum = 0;
  offupdate.BilledMoney = billedMoney;
  offupdate.FinishedSec = 0;
  offupdate.SpeedUpSec = 0;

	iRet = TakeoffAndLockMoney(roleID, cost,moneyType,offupdate);
	if(iRet)
	{
	  RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"TakeoffAndLockMoney failed ! ");
		goto EndOf_Process;
	}

	pRole->IsOfflineUpdate(1);

EndOf_Process:

	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();
	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();
	LOG(LOG_ERROR,__FILE__,__LINE__,"BeginUpdate: RetCode[%d]",RetCode);
	s<<RetCode;
	if( 0 == RetCode )
	{
		//RetCode 为0 才会返回包体剩下内容
	}
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	if(0==RetCode)
	{
		//发送 S_C 金钱变动通知
		_mainSvc->GetBagSvc()->NotifyMoney(roleID);

	}


}

//[MsgType:1699]前台触发扣费
void OffLineUpdateSvc::ProcessLostBilling(Session& session, Packet& packet)
{
	char szSql[1024];
	UInt32	RetCode = 0,iRet;
	DataBuffer	serbuffer(1024);
	UInt32 roleID = packet.RoleID;

	double addCost = 0.0;
	Byte isOfflineUpdate = 0,moneyType  = 0;
	UInt32 addExp = 0,desLevel = 0,totalExp = 0;
	UInt32 updateID = 0,desNum = 0,billedNum = 0;
	Connection con;
	DBOperate dbo;
	Byte isFinish = 0,vipLevl = 0;
	UInt32 finalExp = 0;

	UInt32 startLevel = 0,lastLevel = 0,speedupSec = 0,speedMinute = 0;

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	//序列化类
	Serializer s(packet.GetBuffer());
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//LOG(LOG_ERROR,__FILE__,__LINE__,"===============LostBilling===============");

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());


	//判断是否在离线挂机
	if(0 == pRole->ID())
	{
	    RetCode = ERR_APP_OP;
      LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole[%d] error !" , roleID);
			goto EndOf_Process;
	}

	if(!pRole->IsOfflineUpdate())
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"role.IsOfflineUpdate() is false! roleID[%d] ", roleID );
		goto EndOf_Process;
	}



	totalExp = pRole->Exp();
	startLevel = pRole->Level();
	vipLevl = pRole->VIP();

	//找到挂机记录
	//	判断是否超过目标扣费次数
	sprintf(szSql,"select UpdateID,DesNum,BilledNum,MoneyType,SpeedUpSec from OffLineUpdate\
								where RoleID = %d \
									and IsFinish = 0 ",roleID);
	iRet = dbo.QuerySQL(szSql);
	if(iRet != 0)
	{
	  RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
	updateID = dbo.GetIntField(0);
	desNum = dbo.GetIntField(1);
	billedNum = dbo.GetIntField(2);
	moneyType = dbo.GetIntField(3);
	speedupSec = dbo.GetIntField(4);

	speedMinute = speedupSec / 60;
	if(billedNum >= (desNum - speedMinute))
	{
		RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"data error!! billedNum[%d], leftNum,[%d], roleID[%d] ", billedNum, desNum - speedMinute, roleID );
		goto EndOf_Process;
	}

	//计算该次扣费
	iRet = CalcCostPerMinite(roleID, pRole->Level(), pRole->Exp(), addCost,addExp, desLevel, finalExp,vipLevl);
	if(iRet)
	{
	  RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"CalcCostPerMinite failed ! ");
		goto EndOf_Process;
	}

	//计算金钱
	if(moneyType == 2)
	{
	   addCost = addCost / 300;
	}
	totalExp += addExp;

	//修改数据库记录相关字段
	if( (billedNum+1) >= desNum - speedMinute)
	{
	   isFinish = 1;  				//挂机结束
	   isOfflineUpdate = 0;		//挂机开关置为关
	}

	else
	{
	   isFinish = 0;
	   isOfflineUpdate = 1;		//挂机开关置为开
	}


	sprintf(szSql,"update OffLineUpdate \
								set \
									BilledNum = BilledNum + 1, \
									BilledMoney = BilledMoney + %f,\
									FinishedSec = FinishedSec + 60 ,\
									IsFinish = %d \
								where UpdateID = %d \
									and IsFinish = 0;",
								addCost, isFinish, updateID );
	iRet = dbo.ExceSQL(szSql);
	if(iRet != 0)
	{
	  RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"DB OP error ! ");
		goto EndOf_Process;
	}

	//跟新角色表中离线挂机状态
  if(isOfflineUpdate == 0)
  {
    pRole->IsOfflineUpdate(0);
  }

  //LOG(LOG_ERROR,__FILE__,__LINE__,"startLevel[%d]",startLevel);

	//触发角色属性变化
	iRet = _mainSvc->GetCoreData()->RoleExpAdd(roleID, addExp);
	if(iRet < 0)
	{
	  RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"RoleExpAdd [%d] error !",roleID);
		goto EndOf_Process;
	}

	EndOf_Process:
	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();
	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	LOG(LOG_ERROR,__FILE__,__LINE__,"LostBilling: RetCode [%d]",RetCode);
	/*LOG(LOG_ERROR,__FILE__,__LINE__,"addExp [%d]",addExp);
	LOG(LOG_ERROR,__FILE__,__LINE__,"totalExp [%d]",totalExp);*/

	s<<RetCode;
	if( 0 == RetCode )
	{
		//RetCode 为0 才会返回包体剩下内容
		s<<addExp;
	  s<<totalExp;
	}
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	//升级之后加速挂机的扣费发生变化
	if(0 == RetCode)
	{
	 	 RolePtr tmpRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	 	 if(tmpRole->ID() == 0)
	 	 {
	 	    LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error! roleID[%d]", roleID );
	 	    return ;
	 	 }
	   lastLevel = tmpRole->Level();
	   if(startLevel < lastLevel)
	   {
	   			UInt32 castTenMinute = 0;
	   			iRet = SpeedTenMiniteCast(roleID,castTenMinute);
	   			if(iRet)
	   			{
	   			  LOG(LOG_ERROR,__FILE__,__LINE__,"SpeedTenMiniteCast failed !" );
	   			  return ;
	   			}
	        NotifySpeedUpCast(roleID,castTenMinute);

	   }

	}

}

//[MsgType:1603]金币加速挂机
void OffLineUpdateSvc::ProcessSpeedupUpdate(Session& session, Packet& packet)
{

	char szSql[1024];
	UInt32	RetCode = 0, iRet;
	DataBuffer	serbuffer(1024);
	UInt32 roleID= packet.RoleID;
	Byte speedupType,isOfflineUpdate = 0;

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	UInt32 exp = 0,cost = 0,level = 0,speedupSec = 0,billNum = 0;
	UInt32 finishedSec = 0,isFinish = 0,addFinishSec = 0,speedSec = 0;
	UInt32 updateID = 0,desNum = 0 ;
	UInt32 totalTime = 0,desSec = 0,speedCast = 0;
	double tmpCost = 0.0;

	UInt32 startLevel = 0,lastLevel = 0;
	Connection con;
	DBOperate dbo;

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>speedupType;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	// LOG(LOG_ERROR,__FILE__,__LINE__,"speedupType[%d]" , speedupType);

   //获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//判断是否在挂机中

	if(0 == pRole->ID())
	{
	    RetCode = ERR_APP_OP;
      LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole[%d] error !" , roleID);
			goto EndOf_Process;
	}

	if(!pRole->IsOfflineUpdate())
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"role.IsOfflineUpdate() is false ");
		goto EndOf_Process;
	}

	 startLevel = pRole->Level();

	//计算加速需要的金币和获得的经验、加速后总的经验
	iRet = CalcCostAndGainsBySilver(roleID, 1,tmpCost,exp, level);
	if(iRet)
	{
	  RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"CalcCostAndGainsBySilver failed !" );
		goto EndOf_Process;
	}


	speedCast = (int)ceil(tmpCost / 300 * 2 * 5 /6);
  if(1 == speedupType)
  {//加速10分钟的经验、金币、扣费次数
    exp = (exp + exp / 10) /6;
    //tmpCost = tmpCost / 300 * 2 * 5 /6;
    speedupSec = 10 * 60;
  }
  if(2 == speedupType)
  {//加速60分钟的经验、金币、扣费次数
    exp = exp + exp / 10;
    //tmpCost = tmpCost / 300 * 2 * 5;
    speedupSec = 60 * 60;
    speedCast = speedCast * 6;
  }

  //LOG(LOG_ERROR,__FILE__,__LINE__,"speedupType[%d]--speedCast[%d]" ,speedupType,speedCast);

  //检查是否有足够的金币，如果有则扣除金币
  sprintf(szSql,"select Gold from RoleMoney where RoleID = %d;",roleID);
  iRet = dbo.QuerySQL(szSql);
  if(iRet != 0)
  {
     RetCode = ERR_SYSTEM_DBERROR;
     LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		 goto EndOf_Process;
  }

  cost = speedCast;
  if(cost > dbo.GetIntField(0))
  {
     RetCode = ERR_APP_DATA;
     LOG(LOG_ERROR,__FILE__,__LINE__,"Role[%d] hasn't so much gold !" ,roleID);
		 goto EndOf_Process;
  }

	//找到状态为未完成的挂机记录
	sprintf(szSql,"select UpdateID, FinishedSec, DesSec,SpeedUpSec \
								from OffLineUpdate\
								where RoleID = %d \
									and IsFinish = 0 ",roleID);
	iRet = dbo.QuerySQL(szSql);
	if(iRet)
	{
	  RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"DB OP error");
		goto EndOf_Process;
	}
	updateID = dbo.GetIntField(0);
	totalTime = dbo.GetIntField(1);
	desSec = dbo.GetIntField(2);
	speedSec = dbo.GetIntField(3);
	addFinishSec = totalTime;
	totalTime += speedupSec + speedSec;

	//扣除加速挂机的金币
	sprintf(szSql,"update RoleMoney set \
								Gold = Gold - %d where RoleID = %d;",cost,roleID);
	iRet = dbo.ExceSQL(szSql);
	if(iRet != 0)
	{
	   RetCode = ERR_SYSTEM_DBERROR;
     LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		 goto EndOf_Process;
	}

	//计算该次加速,是否会导致该次挂机结束
	if(totalTime < desSec)
	{
	  isFinish = 0;
	  isOfflineUpdate = 1;
	  addFinishSec = speedupSec;

	}
	else
	{
	  isFinish = 1;    //停止离线挂机
	  isOfflineUpdate = 0;   //离线挂机开关置为关
	  addFinishSec = desSec - addFinishSec;
	}

		//LOG(LOG_ERROR,__FILE__,__LINE__,"isOfflineUpdate[%d] " , isOfflineUpdate);
	//修改离线挂机表中已完成秒数、扣费次数、已扣除的钱、以加速的秒数
  sprintf(szSql,"update OffLineUpdate set \
  																		SpeedUpSec = SpeedUpSec + %d, \
  																		IsFinish = %d \
  																		where UpdateID = %d ",
  																		speedupSec, isFinish, updateID );
  iRet = dbo.ExceSQL(szSql);
  if(iRet != 0)
  {
      RetCode = ERR_SYSTEM_DBERROR;
      LOG(LOG_ERROR,__FILE__,__LINE__,"UpdateRoleAttr RolID[%d] Failed ! " , roleID);
			goto EndOf_Process;
  }

  //跟新角色表中离线挂机状态
  if(isOfflineUpdate == 0)
  {
    pRole->IsOfflineUpdate(0);
  }

	//更新角色经验
	iRet = _mainSvc->GetCoreData()->RoleExpAdd(roleID, exp);
	if(iRet < 0)
	{
	  	RetCode = ERR_APP_OP;
      LOG(LOG_ERROR,__FILE__,__LINE__,"UpdateRoleAttr RolID[%d] Failed ! " , roleID);
			goto EndOf_Process;
	}

EndOf_Process:
	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();
	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();
	LOG(LOG_ERROR,__FILE__,__LINE__,"SpeedUpUpdate: RetCode[%d]",RetCode);

	/*LOG(LOG_ERROR,__FILE__,__LINE__,"exp[%d]" , exp);
	LOG(LOG_ERROR,__FILE__,__LINE__,"role.Exp()+exp[%d]" , role.Exp()+exp);*/
	s<<RetCode;
	if( 0 == RetCode )
	{
		//RetCode 为0 才会返回包体剩下内容
		s<<exp;
		s<<(pRole->Exp()+exp);
	}
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}


	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
	if(0==RetCode)
	{
		//发送 S_C 金钱变动通知
		_mainSvc->GetBagSvc()->NotifyMoney(roleID);
	}

	//升级之后加速挂机的扣费发生变化
	if(0 == RetCode)
	{
	    RolePtr tmpRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	 	 if(tmpRole->ID() == 0)
	 	 {
	 	    LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error! roleID[%d]", roleID );
	 	    return ;
	 	 }
	   lastLevel = tmpRole->Level();
	   if(startLevel < lastLevel)
	   {
	   			UInt32 castTenMinute = 0;
	   			iRet = SpeedTenMiniteCast(roleID,castTenMinute);
	   			if(iRet)
	   			{
	   			  LOG(LOG_ERROR,__FILE__,__LINE__,"SpeedTenMiniteCast failed !" );
	   			  return ;
	   			}
	   			//LOG(LOG_ERROR,__FILE__,__LINE__,"SpeedTenMiniteCast [%d]",castTenMinute );
	        NotifySpeedUpCast(roleID,castTenMinute);

	   }
	}

}

//[MsgType:1604]停止修炼升级
void OffLineUpdateSvc::ProcessStopUpdate(Session& session, Packet& packet)
{
	char szSql[1024];
	DataBuffer	serbuffer(1024);
	UInt32 Money,roleID = packet.RoleID;
	int iRet,RetCode = 0;


	char strTmp[1024];
	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	UInt32 speedupSec = 0;
	UInt32 updateID,desNum ,billedNum,begTime,finishSec,lockMoney,desSec,moneyType;
	UInt32 addSec = 0,addBillNum = 0,addExp = 0,retMoney = 0;
	Int32 elapse = 0;
	double addCost = 0.0,billedMoney = 0.0, tmpCost = 0.0;
	UInt32 tmpExp = 0;
	UInt32 tmpLevel = 0;
	UInt32 currLevel = 0;
	UInt32 currExp = 0;
	UInt32 finalExp = 0;
	Int32	sec2Bill = 0;				//需要补充扣费的时间		单位 秒
	UInt32 startLevel = 0,lastLevel = 0;
	Byte vipLevl = 0;

	Connection con;
	DBOperate dbo;

	//序列化类
	Serializer s(packet.GetBuffer());
	if( s.GetErrorCode()!= 0 )
	{
	 RetCode = ERR_SYSTEM_SERERROR;
	 LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
	 goto EndOf_Process;
	}


	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//判断是否在离线挂机
	if(0 == pRole->ID())
	{
	    RetCode = ERR_APP_OP;
      LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole[%d] error !" , roleID);
			goto EndOf_Process;
	}

	if(!pRole->IsOfflineUpdate())
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"pRole->IsOfflineUpdate() is false ");
		goto EndOf_Process;
	}

	//找到挂机记录
	sprintf(szSql,"select UpdateID,\
												DesNum,\
												BilledNum,\
												BegTime,\
												FinishedSec,\
												BilledMoney,\
												LockMoney,\
												DesSec,\
												MoneyType,\
												SpeedUpSec\
								from OffLineUpdate\
								where RoleID = %d \
									    and IsFinish = 0;",roleID);
	iRet = dbo.QuerySQL(szSql);
	if(iRet != 0)
	{
	  RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}

	updateID = dbo.GetIntField(0);
	desNum = dbo.GetIntField(1);
	billedNum = dbo.GetIntField(2);
	begTime = dbo.GetIntField(3);
	finishSec = dbo.GetIntField(4);
	billedMoney = dbo.GetFloatField(5);
	lockMoney = dbo.GetIntField(6);
	desSec = dbo.GetIntField(7);
	moneyType = dbo.GetIntField(8);
	speedupSec = dbo.GetIntField(9);

	/*LOG(LOG_ERROR,__FILE__,__LINE__,"roleID[%d] " ,roleID);
	LOG(LOG_ERROR,__FILE__,__LINE__,"lockMoney[%d] " , lockMoney);
	LOG(LOG_ERROR,__FILE__,__LINE__,"billedMoney[%f] " , billedMoney);*/

	//计算补充的扣费次数,及扣费金额,补充的完成秒数,补充的增加的经验
  elapse = time(NULL) - begTime;
  if(elapse<0)
  {
		RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"elapse errro![%d] ", elapse);
		goto EndOf_Process;
	}

  //流逝时间的校准
	if(elapse > (desSec - speedupSec))
	  elapse = desSec - speedupSec;
	if(elapse < finishSec)
		elapse = finishSec;

	//流逝时间的校验
	if(elapse<0)
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"elapse  errro! elapse[%d]", elapse );
		goto EndOf_Process;
	}

	//计算补充的 扣费次数
	sec2Bill = elapse - finishSec;
  addBillNum = sec2Bill / 60;
  if(sec2Bill % 60)
     addBillNum++;   //补充的扣费次数

  //补充的经验,扣除的金钱
  currLevel = pRole->Level();
	currExp = pRole->Exp();
	vipLevl = pRole->VIP();
	startLevel = currLevel;

  for(int i = 0; i < addBillNum; i++)
  {
      iRet = CalcCostPerMinite(roleID, currLevel, currExp, tmpCost, tmpExp, tmpLevel, finalExp,vipLevl);
      if(iRet)
      {
        RetCode = ERR_APP_OP;
      	LOG(LOG_ERROR,__FILE__,__LINE__,"CalcCostPerMinite  errro! " );
				goto EndOf_Process;
      }
			addCost += tmpCost;
			addExp += tmpExp;
			currLevel = tmpLevel;
			currExp = finalExp;
  }


  if(2 == moneyType)
  {
  	if(0 == addBillNum)
		{
		  addCost = 0.0;
		}
		else
		{
			if(addBillNum <= 10)
			  addCost = addCost / addBillNum * 30 /300;
			else
			  addCost = addCost / addBillNum * 60 /300;

		}

     addExp = (int)(addExp*1.1);
     sprintf(strTmp,"Gold = Gold +"); //返回金币
  }
  else
  {
     sprintf(strTmp,"BindMoney = BindMoney +"); //返回银币
  }


	//更新离线挂机表

	sprintf(szSql,"update OffLineUpdate set\
	               FinishedSec = FinishedSec + %d,\
	               BilledMoney = BilledMoney + %f,\
	               BilledNum = BilledNum + %d,\
	               IsFinish = 1 \
	               where UpdateID = %d;",
	               elapse,addCost,addBillNum,updateID);
	iRet = dbo.ExceSQL(szSql);
	LOG(LOG_ERROR,__FILE__,__LINE__,"iRet[%d] " , iRet);
	if(iRet != 0)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}

	//退回金钱

	retMoney = (UInt32)(lockMoney - billedMoney - addCost);
	//LOG(LOG_ERROR,__FILE__,__LINE__,"retMoney[%d] " , retMoney);

	if(0 != retMoney)
	{
		  sprintf(szSql,"update RoleMoney set %s %d\
										where RoleID = %d;",strTmp,retMoney,roleID);
			//LOG(LOG_ERROR,__FILE__,__LINE__,"[%s] " , szSql);

			iRet = dbo.ExceSQL(szSql);
			if(iRet != 0)
			{
			    RetCode = ERR_SYSTEM_DBERROR;
		      LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
					goto EndOf_Process;
			}
	}


	 //挂机开关关闭
	pRole->IsOfflineUpdate(0);

	sprintf(szSql,"update Role set IsOfflineUpdate = 0 where RoleID = %d;",roleID);
	iRet = dbo.ExceSQL(szSql);
	if(iRet != 0)
	{
		RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"Excute SQL failed ! szSql[%d]", szSql);
		goto EndOf_Process;
	}

	//如果补充的增加的经验不为 0   ,增加角色经验
	if(addExp)
	{
	    iRet = _mainSvc->GetCoreData()->RoleExpAdd(roleID, addExp);
			if(iRet < 0)
			{
			  	RetCode = ERR_APP_OP;
		      LOG(LOG_ERROR,__FILE__,__LINE__,"UpdateRoleAttr RolID[%d] Failed ! " , roleID);
					goto EndOf_Process;
			}
	}

	EndOf_Process:
	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();
	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();

	LOG(LOG_ERROR,__FILE__,__LINE__,"StopUpdate: RetCode[%d]" , RetCode);
	s<<RetCode;
	if( 0 == RetCode )
	{
		//RetCode 为0 才会返回包体剩下内容
	}
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

 	//金钱通知
	if(0 == RetCode)
	{
		_mainSvc->GetBagSvc()->NotifyMoney(roleID);
	}

	//升级之后加速挂机的扣费发生变化
	if(0 == RetCode)
	{
	   RolePtr tmpRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	 	 if(tmpRole->ID() == 0)
	 	 {
	 	    LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error! roleID[%d]", roleID );
	 	    return ;
	 	 }
	   lastLevel = tmpRole->Level();
	   if(startLevel < lastLevel)
	   {
	   			UInt32 castTenMinute = 0;
	   			iRet = SpeedTenMiniteCast(roleID,castTenMinute);
	   			if(iRet)
	   			{
	   			  LOG(LOG_ERROR,__FILE__,__LINE__,"SpeedTenMiniteCast failed !" );
	   			  return ;
	   			}
	        NotifySpeedUpCast(roleID,castTenMinute);

	   }
	}

}


//[MsgType:1605] 剩余挂机的时间
void OffLineUpdateSvc::ProcessLeftUpdateHour(Session& session, Packet& packet)
{

	char szSql[1024];
	DataBuffer	serbuffer(1024);
	UInt32 roleID = packet.RoleID;
	int iRet,RetCode = 0;
	string strDate;
	DateTime tmpDate;
	UInt32 totalSec = 0;
	Int32 leftUpdateHour = 0;


	Connection con;
	DBOperate dbo;

	//序列化类
	Serializer s(packet.GetBuffer());
	if( s.GetErrorCode()!= 0 )
	{
	 RetCode = ERR_SYSTEM_SERERROR;
	 LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
	 goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

  //获取现在的时间
  strDate = tmpDate.Now().StringDate();
  sprintf(szSql,"select sum(FinishedSec),sum(SpeedUpSec) from OffLineUpdate \
  								where RoleID = %d and BegDate = '%s';",roleID, strDate.c_str());
  iRet = dbo.QuerySQL(szSql);
  if(iRet < 0)
  {
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error !");
		goto EndOf_Process;
  }
  else if(iRet == 1)  				//今天没有挂机记录
  {
    leftUpdateHour = 24;
  }
  else											 //今天有挂机记录
  {
       totalSec += dbo.GetIntField(0) + dbo.GetIntField(1);
       leftUpdateHour = (24 * 3600 - totalSec) / 3600;
       if(leftUpdateHour < 0)
       {
         	RetCode = ERR_APP_DATA;
					LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error !");
					goto EndOf_Process;
       }

       // LOG(LOG_ERROR,__FILE__,__LINE__,"tmpHour[%d]" , tmpHour);
       // LOG(LOG_ERROR,__FILE__,__LINE__,"totalSec[%d]" , totalSec);
  }

  LOG(LOG_DEBUG,__FILE__,__LINE__,"leftUpdateHour[%d]" , leftUpdateHour);

  EndOf_Process:
	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();
	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();

	//LOG(LOG_ERROR,__FILE__,__LINE__,"RetCode[%d]" , RetCode);
	//LOG(LOG_ERROR,__FILE__,__LINE__,"leftUpdateHour[%d]" , leftUpdateHour);
	s<<RetCode;
	if( 0 == RetCode )
	{
		//RetCode 为0 才会返回包体剩下内容
		s<<leftUpdateHour;
	}
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

}



//@brief 计算每分钟获得经验, 目标等级, 花费
//@param	inputLevel	输入的角色当前等级
//@param	inputExp		输入的角色当前经验
//@param	retCost		需要的花费
//@param	retAddExp		增加的经验
//@param	retFinalLevel		最终级别,	该次计算之后角色应升级到的等级
//@param	retFinalExp			最终经验, 该次计算之后角色应升级到的经验, 与 retFinalLevel 对应
//@return	0 成功  非0 失败
int OffLineUpdateSvc::CalcCostPerMinite(UInt32 RoleID, UInt32 inputLevel, UInt32 inputExp, double& retCost, UInt32& retAddExp, UInt32& retFinalLevel, UInt32& retFinalExp,Byte vipLevl)
{
  char szSql[1024];
  UInt32 iRet = 0;
  int iflag = 0;
  UInt32 speedupGoldCast = 0;
	Connection con;
	DBOperate dbo;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//该级别挂机每分钟获得的经验
	sprintf(szSql,"select ExpPerMinute from ExpDescOfflineUpdate \
									where Level = %d;",inputLevel);
	iRet = dbo.QuerySQL(szSql);
	if(iRet != 0)
	{
	   LOG(LOG_ERROR,__FILE__,__LINE__,"[%s]" ,szSql);
		 return -1;
	}

	retAddExp= dbo.GetIntField(0);
	if(vipLevl >= 1 && vipLevl <= 6)
	{
		VipExtraExp(RoleID,vipLevl,retAddExp);
	}
	retCost = (double)(inputLevel * inputLevel) / 1000 * 5;

	//计算结果
  retFinalExp = inputExp+retAddExp;						//该次升级的累积经验
	UInt32 currLevl = inputLevel;
	UInt32 maxExp = 0;

	while(1)
	{
		sprintf(szSql,"select MaxExp \
										from LevelDesc \
										where Level = %d;", currLevl);
		iRet = dbo.QuerySQL(szSql);

		if(1==iRet)
		{
		   LOG(LOG_DEBUG,__FILE__,__LINE__,"DB data not found !" );
			 return -1;
		}
		if(iRet<0)
		{
		   LOG(LOG_ERROR,__FILE__,__LINE__,"DB error !" );
			 return -1;
		}
		maxExp = dbo.GetIntField(0);
//	  LOG(LOG_DEBUG,__FILE__,__LINE__,"maxExp[%d]", maxExp );

		if(retFinalExp<maxExp)
			break;

		retFinalExp -= maxExp;
		++currLevl;

	}

  retFinalLevel = currLevl;

	return 0;

}


//@brief 计算获得经验, 目标等级, 花费
//@return	0 成功  非0 失败
int OffLineUpdateSvc::CalcCostAndGainsBySilver( UInt32 RoleID, Byte hangType, double& costSum, UInt32& expSum, UInt32& desLevel)
{
	UInt32 iMinite = 0;
	int iRet = 0;

  //计算升级所用的分钟数
	iMinite  = 60 *hangType;

	//获取当前等级和经验
	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(RoleID);
	if(0 == pRole->ID())
	{
	   LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole[%d] failed !" ,RoleID);
		 return -1;
	}

  UInt32 begLevel = pRole->Level();
	UInt32 begExp = pRole->Exp();
	Byte vipLevl = pRole->VIP();

	//按分钟计算花费及收益
	costSum = 0;		//累积花费
	expSum = 0;			//累积收益
	UInt32 currLevel = begLevel;
	UInt32 currExp = begExp;
	UInt32 finalExp = 0;
	for( int i = 0; i < iMinite; i++ )
	{
		double tmpCost = 0.0;
		UInt32 tmpExp = 0;
	  iRet = CalcCostPerMinite( RoleID,currLevel, currExp, tmpCost, tmpExp, desLevel, finalExp,vipLevl);
	  if(iRet)
	  {
	     LOG(LOG_ERROR,__FILE__,__LINE__,"CalcCostPerMiniteByCache error !" );
			 return -1;
	  }

		expSum += tmpExp;
		costSum += tmpCost;
		currLevel = desLevel;
		currExp = finalExp;
	}

	return 0;
}

//@brief	S_C [MsgType:1601]挂机数据
void OffLineUpdateSvc::OnRoleLoginNotify(UInt32 roleID)
{
	 UInt32 desExp = 0;
	 UInt32 desLevel = 0;
	 UInt32 finishSec = 0;
	 UInt32 desSec = 0;
	 int iRet ;

    char szSql[1024];
	Connection con;
	DBOperate dbo;
	string strTime;
	DateTime nowTime;
	Byte isFinish = 0;
	Byte offlineState = 0;

	Byte isOfflineUpdate = 0;

	 //获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());



  //获取角色信息
  //	此时角色尚未进入地图,缓存还没有数据
  sprintf(szSql,"select IsOfflineUpdate \
  							from Role \
  							where RoleID = %d;",roleID);
  iRet = dbo.QuerySQL(szSql);
  if(iRet)
  {
  	LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error!!" );
  	return;
  }

  isOfflineUpdate = dbo.GetIntField(0);

	//LOG(LOG_ERROR,__FILE__,__LINE__,"isOfflineUpdate[%d]",isOfflineUpdate);
  //获取挂机开关是否开启
  //	没有挂机,则不作任何处理
  if(0 == isOfflineUpdate)
  {
     return ;
  }


  LOG(LOG_ERROR,__FILE__,__LINE__,"isOfflineUpdate[%d]",isOfflineUpdate);

  iRet = AddBill(roleID,desExp, desLevel, finishSec, desSec );
  if(iRet)
  {
     LOG(LOG_ERROR,__FILE__,__LINE__,"AddBill error!!" );
  	 return ;
  }

	NotifyStopUpdate(roleID,desExp, desLevel,finishSec,desSec);

}

//@brief	角色上线 补充扣费
//				仅供角色未进入地图前的情况调用
//@return 0 成功  非0 失败
int OffLineUpdateSvc::AddBill( UInt32 roleID, UInt32& desExp, UInt32& desLevel, UInt32& finishSec, UInt32& desSec )
{
	char szSql[1024];
	int iRet = 0,isFinish = 0;
	UInt32 elapse = 0,begTime = 0,billNum = 0,billedNum = 0,addFinishSec = 0;
	UInt32 speedupSec = 0,updateID = 0;
	Byte isOfflineUpdate = 0,vipLevl = 0;
	UInt32 retLevel,now = 0;
	Int32	sec2Bill = 0;				//需要补充扣费的时间		单位 秒
	Connection con;
	DBOperate dbo;

	 //获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());


  //获取 DB 的挂机记录
  sprintf(szSql,"select BegTime,FinishedSec,DesSec,DesExp,DesLevel,BilledNum,SpeedUpSec,UpdateID from OffLineUpdate\
  								where RoleID = %d and IsFinish = 0;",roleID);

	iRet = dbo.QuerySQL(szSql);

	if(iRet == 1)
	{
	 	 LOG(LOG_DEBUG,__FILE__,__LINE__,"DB empty set!!" );
  	 return 0;
	}

	if(iRet != 0)
	{
	 	 LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error!!" );
  	 return -1;
	}

	begTime = dbo.GetIntField(0);
	finishSec = dbo.GetIntField(1);
	desSec = dbo.GetIntField(2);
	desExp = dbo.GetIntField(3);
	desLevel = dbo.GetIntField(4);
	billedNum = dbo.GetIntField(5);
	speedupSec = dbo.GetIntField(6);
	updateID = dbo.GetIntField(7);

  //计算时间差
  elapse = time(NULL) - begTime;
  if(elapse<0)
  {
		LOG(LOG_ERROR,__FILE__,__LINE__,"elapse error!!elapse[%d]", elapse);
		return -1;
  }

  //流逝时间的校准
	if(elapse > (desSec - speedupSec))
	  elapse = desSec - speedupSec;
	if(elapse < finishSec)
		elapse = finishSec;

	//流逝时间的校验
	if(elapse<0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"elapse  errro! elapse[%d]", elapse );
		return -1;
	}

	//判断是否挂机结束
	if(elapse >= (desSec - speedupSec - finishSec))
	{
		//挂机已经结束
		isFinish = 1;
		isOfflineUpdate = 0;  //挂机结束
	}
	else
	{
		//挂机未结束,计算完成秒数等等
	  isFinish = 0;
	  isOfflineUpdate = 1;
	}

	//计算补充的 扣费次数
	sec2Bill = elapse - finishSec;
	LOG(LOG_ERROR,__FILE__,__LINE__,"sec2bill[%d]",sec2Bill);
	billNum = sec2Bill / 60;
	if(sec2Bill % 60)
	  billNum++;

	//获取升级前等级和经验
	sprintf(szSql,"select Exp,Level,IsVIP\
  							from Role \
  							where RoleID = %d;", roleID);
  iRet = dbo.QuerySQL(szSql);
  if(iRet != 0)
  {
  	LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error!!" );
  	return -1;
  }

	UInt32 begExp = dbo.GetIntField(0);
	UInt32 begLevel = dbo.GetIntField(1);
	vipLevl = dbo.GetIntField(2);


	//计算补充扣费,补充经验
	UInt32 addExpSum = 0;
	double addCostSum = 0;
	UInt32 currLevel = begLevel;
	UInt32 currExp = begExp;
	UInt32 finalExp = 0;

	for(int i = 0; i < billNum; i++)
	{
		double tmpCost = 0.0;
		UInt32 tmpExp = 0;
		iRet = CalcCostPerMinite(roleID, currLevel, currExp, tmpCost, tmpExp, retLevel,finalExp,vipLevl);
		if(iRet)
		{
				LOG(LOG_ERROR,__FILE__,__LINE__,"CalcCostPerMinite error!!" );
				return -1;
		}

		addExpSum += tmpExp;
		addCostSum += tmpCost;
		currLevel = retLevel;
		currExp = finalExp;
	}

	//LOG(LOG_ERROR,__FILE__,__LINE__,"billNum[%d]", billNum );

	//补充扣费次数不为0,挂机未结束
	finishSec += billedNum * 60;
	finishSec += speedupSec;
	if(billNum)
	{
		  //更新 挂机记录
	  sprintf(szSql,"update OffLineUpdate set\
			  								IsFinish = %d,\
			  								BilledNum = BilledNum + %d,\
			  								BilledMoney = BilledMoney + %f,\
			  								FinishedSec = FinishedSec + %d\
	  								where RoleID = %d and IsFinish = 0;",\
	  								isFinish,billNum,addCostSum, billNum*60 ,roleID);
//	  LOG(LOG_ERROR,__FILE__,__LINE__,"szSql[%s]",szSql);
	  iRet = dbo.ExceSQL(szSql);
	  if(iRet != 0)
	  {
	     LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error!!" );
	  	 return -1;
	  }

	  //更新角色经验
	  //	此时角色尚未进入地图,缓存还没有数据, 直接修改 DB
	  sprintf(szSql,"update Role set Exp = Exp + %d\
	  								where RoleID = %d;", addExpSum,roleID);
	  iRet = dbo.ExceSQL(szSql);
	  if(iRet != 0)
	  {
	     LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error !" );
	  	 return -1;
	  }
	}

	//更新角色表中离线挂机状态
  if(isOfflineUpdate == 0)
  {
    sprintf(szSql,"update Role set IsOfflineUpdate = %d\
	  								where RoleID = %d;", isOfflineUpdate,roleID);
	  iRet = dbo.ExceSQL(szSql);
	  if(iRet != 0)
	  {
	     LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error !" );
	  	 return -1;
	  }

	  sprintf(szSql,"update OffLineUpdate set IsFinish = 1\
	  								where UpdateID = %d;",updateID);
	  iRet = dbo.ExceSQL(szSql);
	  if(iRet != 0)
	  {
	     LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error !" );
	  	 return -1;
	  }
  }


	return 0;
}


//加速10分钟的扣费 返回 0 成功 非0 失败
int  OffLineUpdateSvc::SpeedTenMiniteCast(UInt32 roleID,UInt32 &speedTenMinute)
{
  int iRet = 0;
	double TmpCost = 0;
	UInt32 TmpExp = 0;
	UInt32 TmpLevel = 0;
	iRet = CalcCostAndGainsBySilver( roleID, 1, TmpCost, TmpExp,TmpLevel);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"CalcCostAndGainsBySilver error !" );
		return -1;
	}

	speedTenMinute = (int)ceil(TmpCost / 300 * 2 * 5 /6);

	return 0;
}

//vip 挂机加成
int OffLineUpdateSvc::VipExtraExp(UInt32 roleID,Byte vipLevl,UInt32 &tmpExp)
{
	if(vipLevl < 1 || vipLevl > 6)
	{
		return -1;
	}

	if(1 == vipLevl)
	{
		tmpExp = (int)(1.2 * tmpExp);
	}
	else if(2 == vipLevl)
	{
		tmpExp = (int)(1.3 * tmpExp);
	}
	else if(3 == vipLevl)
	{
		tmpExp = (int)(1.4 * tmpExp);
	}
	else
	{
		tmpExp = (int)(1.5 * tmpExp);
	}

	return 0;

}


void  OffLineUpdateSvc::NotifyStopUpdate(UInt32 roleID,UInt32 desExp, UInt32 desLevl,UInt32 finishSec,UInt32 desSec)
{
  List<UInt32>lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 1601;
	p.UniqID = 214;
	p.PackHeader();
	lrid.push_back(roleID);

	s<<desExp;
	s<<desLevl;
	s<<finishSec;
	s<<desSec;
	p.UpdatePacketLength();

	//LOG(LOG_ERROR,__FILE__,__LINE__,"finishSec [%d]" ,finishSec);
	//LOG(LOG_ERROR,__FILE__,__LINE__,"desSec [%d]" ,desSec);

	if( _mainSvc->Service()->Broadcast(lrid ,&serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}


//[MsgType:1602]挂机数据
 void OffLineUpdateSvc::NotifySpeedUpCast(UInt32 roleID, UInt32 speedupCast)
 {
  List<UInt32>lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 1602;
	p.UniqID = 214;
	p.PackHeader();

	lrid.push_back(roleID);

	s<<speedupCast;

	p.UpdatePacketLength();

	if( _mainSvc->Service()->Broadcast(lrid ,&serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
 }

