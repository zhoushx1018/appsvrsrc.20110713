#include "DelegateSvc.h"
#include "MainSvc.h"
#include "DBOperate.h"
#include "CoreData.h"
#include "Role.h"
#include "ArchvDelegate.h"
#include "./Task/TaskSvc.h"
#include "NewTimer.h"

DelegateSvc::DelegateSvc(void* service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;

}

DelegateSvc::~DelegateSvc()
{
	for (std::list<NewTimer*>::iterator it = _timerList.begin(); it != _timerList.end(); ++it)
	{
		delete *it;
	}
	_timerList.clear();
}
//数据包信息
void DelegateSvc::OnProcessPacket(Session& session,Packet& packet)
{
DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 1901: //[MsgType:1901]查询进度
			ProcessGetRoleDelegate(session,packet);
			break;

		case 1902: //[MsgType:1902]请求开始
			ProcessBegRoleDelegate(session,packet);
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
void DelegateSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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


void DelegateSvc::ProcessGetRoleDelegate(Session& session,Packet& packet)
{

	UInt32	RetCode = 0;
	DataBuffer	serbuffer(2048);
	char szSql[4096];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	UInt32 roleID = packet.RoleID;
	
	ArchvRoleDelegateInfo rdi;
	List<ArchvRoleDelegateInfo> lrdi;
	UInt32 delegateID = 0;

	DateTime dateTime(DateTime::Now());
	string strToday(dateTime.StringDate());
	string strNow(dateTime.StringDateTime());
	UInt32 now = (UInt32)dateTime.GetTime();

	//序列化类
	Serializer s(packet.GetBuffer());
//	s>>stepID;
	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//获取角色信息
	if(pRole->ID()== 0)
	{
		RetCode = ERR_APP_ROLENOTEXISTS;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole ,roleID[%d] ", roleID );
		goto EndOf_Process;
 	}


	//删除所有非本日开始的 已完成的委托
	sprintf( szSql, " delete from RoleDelegate \
										where BegDate != '%s' \
											and IsFinish = 1 ", strToday.c_str());
	iRet = dbo.ExceSQL(szSql);
	if(iRet)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}
 
	//查找角色本日相关委托任务
	//	角色本日已接已完成的委托任务
	//	角色本日已接未完成的委托任务
	//	角色本日未接的委托任务
	sprintf( szSql, "select rd.DelegateID, t.TaskID, t.MaxDelegateNum, \
										 t.DelegateTime, t.DelegateCost, 1 as IsStarted, \
										 rd.FinishNum, rd.BegTime \
									 from Task t, RoleDelegate rd \
									 where t.TaskType = 3 \
									 	 and rd.RoleID = %d \
									 	 and rd.TaskID = t.TaskID \
 									 	 and rd.IsFinish = 0 \
									 	 and rd.BegDate = '%s' \
									 union \
									 select rd.DelegateID, t.TaskID, t.MaxDelegateNum, \
										 t.DelegateTime, t.DelegateCost, 0 as IsStarted, \
										 rd.FinishNum, rd.BegTime \
									 from Task t, RoleDelegate rd \
									 where t.TaskType = 3 \
									 	 and rd.RoleID = %d \
									 	 and rd.TaskID = t.TaskID \
 									 	 and rd.IsFinish = 1 \
									 	 and rd.BegDate = '%s' \
									 union \
									 select 0 as DelegateID, t.TaskID, t.MaxDelegateNum, \
										 t.DelegateTime, t.DelegateCost, 0 as IsStarted, \
										 0 as FinishNum, 0 as BegTime \
									 from Task t \
									 where t.TaskType = 3 \
									 	 and t.MaxLevel >= %d \
									 	 and t.TaskID not in ( select TaskID \
																		 	 		from RoleDelegate \
																		 	 		where RoleID = %d \
																		 	 		  and BegDate = '%s' \
																		 	 	 ) \
									 	 ",
									 	 roleID, strToday.c_str(),
									 	 roleID, strToday.c_str(),
									 	 pRole->Level(), roleID, strToday.c_str() );

	iRet = dbo.QuerySQL(szSql);
	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}

 	while(dbo.HasRowData())
 	{

		delegateID = dbo.GetIntField(0);
		rdi.taskID = dbo.GetIntField(1);
		rdi.maxDelegateNum = dbo.GetIntField(2);
		rdi.delegateTime = dbo.GetIntField(3);
		rdi.delegateCost = dbo.GetIntField(4);
		rdi.isStarted = dbo.GetIntField(5);
		rdi.finishNum = dbo.GetIntField(6);
		rdi.elapseTime = dbo.GetIntField(7);

		//开始时间校验
		if(now < rdi.elapseTime)
		{
			RetCode = ERR_APP_DATA;
			LOG(LOG_ERROR,__FILE__,__LINE__,"rdi.begtime error! rdi.begtime[%d],now[%d] ", rdi.elapseTime, now );
			goto EndOf_Process;
		}

		//计算流逝的时间
		rdi.elapseTime = now-rdi.elapseTime;

		lrdi.push_back(rdi);

 		dbo.NextRow();
 	}


EndOf_Process:
	
	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();
	
	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();

		
	s<<RetCode;
	
	if( 0 == RetCode )
	{//RetCode 为0 才会返回包体剩下内
		s<<lrdi;
	}	

	p.UpdatePacketLength();
	
	//发送应答数据
	//该交易不发送应答
 	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}
	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	return;
	
}



void DelegateSvc::ProcessBegRoleDelegate(Session& session,Packet& packet)
{

	UInt32	RetCode = 0;
	DataBuffer	serbuffer(2048);
	char szSql[4096];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	UInt32 roleID = packet.RoleID;

	UInt32 taskID = 0;
	UInt16 delegateNum = 0;
	ArchvRoleDelegateInfo rdi;
	UInt32 delegateID = 0;
	UInt32 maxLevel = 0;
	//获取角色信息
	RolePtr role = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	UInt32 gold = 0;
	DateTime dateTime(DateTime::Now());
	string strToday(dateTime.StringDate());
	string strNow(dateTime.StringDateTime());
	UInt32 now = (UInt32)dateTime.GetTime();
	NewTimer *pTimer = 0;
	//序列化类
	Serializer s(packet.GetBuffer());
	s>>taskID>>delegateNum;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());


	if(role->ID()== 0)
	{
		RetCode = ERR_APP_ROLENOTEXISTS;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole ,roleID[%d] ", roleID );
		goto EndOf_Process;
 	}

	//删除所有非本日开始的 已完成的委托
	sprintf( szSql, " delete from RoleDelegate \
										where BegDate != '%s' \
											and IsFinish = 1 ", strToday.c_str());
	iRet = dbo.ExceSQL(szSql);
	if(iRet)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}

 	//查找角色该日该任务明细
 	//	该任务委托状态存在三种可能: 已接已完成, 已接未完成, 未接	
	sprintf( szSql, "select rd.DelegateID, t.TaskID, t.MaxDelegateNum, \
									 t.DelegateTime, t.DelegateCost, 1 as IsStarted, \
									 rd.FinishNum, rd.BegTime \
								 from Task t, RoleDelegate rd \
								 where t.TaskID = %d \
  								 and t.TaskType = 3 \
								 	 and rd.RoleID = %d \
								 	 and rd.TaskID = t.TaskID \
									 	 and rd.IsFinish = 0 \
								 	 and rd.BegDate = '%s' \
								 union \
								 select rd.DelegateID, t.TaskID, t.MaxDelegateNum, \
									 t.DelegateTime, t.DelegateCost, 0 as IsStarted, \
									 rd.FinishNum, rd.BegTime \
								 from Task t, RoleDelegate rd \
								 where t.TaskID = %d \
								 	 and t.TaskType = 3 \
								 	 and rd.RoleID = %d \
								 	 and rd.TaskID = t.TaskID \
									 	 and rd.IsFinish = 1 \
								 	 and rd.BegDate = '%s' \
								 union \
								 select 0 as DelegateID, t.TaskID, t.MaxDelegateNum, \
									 t.DelegateTime, t.DelegateCost, 0 as IsStarted, \
									 0 as FinishNum, 0 as BegTime \
								 from Task t \
								 where t.TaskID = %d \
								   and t.TaskType = 3 \
								 	 and t.MaxLevel >= %d \
								 	 and t.TaskID not in ( select TaskID \
																	 	 		from RoleDelegate \
																	 	 		where RoleID = %d \
																	 	 		  and BegDate = '%s' \
																	 	 	 ) \
								 	 ",
								 	 taskID, roleID, strToday.c_str(),
								 	 taskID, roleID, strToday.c_str(),
								 	 taskID, role->Level(), roleID, strToday.c_str() );
									 	 
	iRet = dbo.QuerySQL(szSql);
	if(iRet)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}

	delegateID = dbo.GetIntField(0);
	rdi.taskID = dbo.GetIntField(1);
	rdi.maxDelegateNum = dbo.GetIntField(2);
	rdi.delegateTime = dbo.GetIntField(3);
	rdi.delegateCost = dbo.GetIntField(4);
	rdi.isStarted = dbo.GetIntField(5);
	rdi.finishNum = dbo.GetIntField(6);
	rdi.elapseTime = dbo.GetIntField(7);

 	//剩余次数判断
 	if( (rdi.finishNum+delegateNum)> rdi.maxDelegateNum )
 	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"error! finishNum[%d] >=  maxDelegateNum[%d] ", rdi.finishNum, rdi.maxDelegateNum);
		goto EndOf_Process;
 	}

 	//查询金钱
 	sprintf( szSql, "select Gold \
 										from RoleMoney \
 										where RoleID = %d ", roleID );
	iRet = dbo.QuerySQL(szSql);
	if(iRet)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}
 	gold = dbo.GetIntField(0);
 	
 	//是否有充足金钱
	if( delegateNum*rdi.delegateCost > gold )
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"error! not enough gold,delegateNum[%d],delegateCost[%d], gold[%d]", delegateNum, rdi.delegateCost, gold );
		goto EndOf_Process;
	}
 	

	//该日该任务是否有委托记录
	if(delegateID)
	{//该日该任务已有委托记录
	
		//委托进行中,不允许另行开始
		if(1 == rdi.isStarted)
		{
			RetCode = ERR_APP_DATA;
			LOG(LOG_ERROR,__FILE__,__LINE__,"error! Delegate isStarted !!isStarted[%d]", rdi.isStarted );
			goto EndOf_Process;
	 	}

		//更新委托记录
		sprintf( szSql, " update RoleDelegate \
											set IsFinish = 0, \
												  BegTime = %d, \
												  BegTimeDesc = '%s', \
												  DelegateNum = %d \
											where DelegateID = %d ",
											now, strNow.c_str(), delegateNum, delegateID );
		iRet = dbo.ExceSQL(szSql);
		if(iRet)
		{
			RetCode = ERR_SYSTEM_DBERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			goto EndOf_Process;
	 	}
	}
	else
	{//该日该任务没有委托记录

		//增加委托记录
		sprintf( szSql, " insert into RoleDelegate( DelegateID, RoleID, TaskID, IsFinish, \
												BegDate, BegTime, BegTimeDesc, FinishNum, DelegateNum) \
					            values( NULL, %d, %d, 0, \
					            	'%s', %d, '%s', 0, %d ) ",
												roleID, taskID,
												strToday.c_str(), now, strNow.c_str(), delegateNum );
		iRet = dbo.ExceSQL(szSql);
		if(iRet)
		{
			RetCode = ERR_SYSTEM_DBERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			goto EndOf_Process;
	 	}

	 	delegateID = dbo.LastInsertID();
	}

	//扣除金钱
	sprintf( szSql, " update RoleMoney \
										set Gold = Gold - %d \
										where RoleID = %d ",
										delegateNum * rdi.delegateCost, roleID );
	iRet = dbo.ExceSQL(szSql);
	if(iRet)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}

 	//委托任务回调
 	pTimer = new NewTimer;
	if (pTimer)
	{
		pTimer->Type(NewTimer::OnceTimer);
		iRet = pTimer->SetCallbackFun( TimerCBDelegate, this, &delegateID, sizeof(delegateID));
		if(iRet)
		{
			RetCode = ERR_APP_OP;
			LOG(LOG_ERROR,__FILE__,__LINE__,"SetCallbackFun error!! roleID[%d], delegateID[%d]", roleID, delegateID);
			goto EndOf_Process;
		}
		pTimer->Interval(delegateNum*rdi.delegateTime);
		_mainSvc->_tm.AddTimer(pTimer);	
	}
 	
 	_timerList.push_back(pTimer);

	
 
EndOf_Process:
	
	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();
	
	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();

		
	s<<RetCode;
	
	if( 0 == RetCode )
	{//RetCode 为0 才会返回包体剩下内
		s<<taskID;
	}	

	p.UpdatePacketLength();
	
	//发送应答数据
	//该交易不发送应答
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}
	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	return;
	
}


//@brief 委托时间超时事件:委托任务完结处理
void DelegateSvc::OnDelegateTimeout(UInt32 delegateID )
{
	char szSql[4096];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	UInt32 roleID = 0;
	UInt32 taskID = 0;
	Byte  isFinish = 0;
	string begDate;
	UInt32 delegateNum = 0;
	string afterDeliverScript;
	DateTime dateTime(DateTime::Now());
	string strToday(dateTime.StringDate());

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

DEBUG_PRINTF1("111111111111delegateID[%d]", delegateID);

	//查询该委托是否存在,相关任务是否存在
	sprintf( szSql, "select rd.RoleID, rd.TaskID, rd.IsFinish, rd.BegDate, \
									rd.DelegateNum, t.AfterDeliverScript \
								 from RoleDelegate rd, Task t  \
								 where rd.DelegateID = %d \
								   and rd.IsFinish = 0 \
								   and t.TaskID = rd.TaskID \
								 	 and t.TaskType = 3 ",
								 	 delegateID );
	iRet = dbo.QuerySQL(szSql);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return;
 	}
	roleID = dbo.GetIntField(0);
	taskID = dbo.GetIntField(1);
	isFinish = dbo.GetIntField(2);
	begDate = dbo.GetStringField(3);
	delegateNum = dbo.GetIntField(4);
	afterDeliverScript = dbo.GetStringField(5);


 	//更新角色委托记录的状态
	sprintf( szSql, "update RoleDelegate \
									set IsFinish = 1, \
											FinishNum = FinishNum + %d, \
											DelegateNum = 0 \
									where DelegateID = %d ",
						 	 delegateNum, delegateID );
	iRet = dbo.ExceSQL(szSql);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return;
 	}

	//执行任务奖励脚本
	//	执行成功与否,都继续往下处理
	iRet = _mainSvc->GetTaskSvc()->ExecAfterDeliverScript(roleID, afterDeliverScript);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExecAfterDeliverScript afterDeliverScript[%s]", afterDeliverScript.c_str());
 	}

	//委托开始日非本日,则删除角色委托记录
	//删除所有非本日开始的 已完成的委托
	sprintf( szSql, " delete from RoleDelegate \
										where BegDate != '%s' \
											and IsFinish = 1 ", strToday.c_str());
	iRet = dbo.ExceSQL(szSql);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
 	}

	// S_C 1901 通知
 	NotifyEndOfRoleDelegate(roleID, taskID);
 	
	
}

//@brief 委托时间超时回调
void DelegateSvc::TimerCBDelegate( void * obj, void * arg, int argLen )
{
	DelegateSvc * currObj = (DelegateSvc*)obj;
	UInt32 delegateID = *((int*)arg);
	
	currObj->OnDelegateTimeout(delegateID);
}


//@brief [MsgType:1901] （单播）委托任务完成
void DelegateSvc::NotifyEndOfRoleDelegate(UInt32 roleID,UInt32 taskID)
{
	DataBuffer	serbuffer(1024);
	Int16 iRet = 0;
	List<UInt32> lrid;
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 1901;
	p.UniqID = 111;
	p.PackHeader();
	
	s<<taskID;
	p.UpdatePacketLength();
	lrid.push_back(roleID);

	if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}


