#include "TaskSvc.h"
#include "MainSvc.h"
#include "TaskLua.h"
#include "DBOperate.h"
#include "ArchvTask.h"
#include "CoreData.h"
#include "Role.h"
#include "./Bag/BagSvc.h"
#include "./RoleInfo/RoleInfoSvc.h"

using namespace TaskLua;


TaskSvc::TaskSvc(void* service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;
}

TaskSvc::~TaskSvc()
{
}

void TaskSvc::OnProcessPacket(Session& session,Packet& packet)
{

DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 701: //角色任务查询
			ProcessGetRoleTask(session,packet);
			break;

		case 703: //接收任务
			ProcessAcceptTask(session,packet);
			break;

		case 704: //交付任务
			ProcessDeliverTask(session,packet);
			break;

		case 705: //放弃任务
			ProcessAbandonTask(session,packet);
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
void TaskSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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



//@brief	事件处理:打怪结束
//				goalType 1
//@param	roleID				角色ID
//@param	monsterType		怪类型
//@param	killNum				杀怪数量
//@return	空
void TaskSvc::OnAfterKillMonster( UInt32 roleID, UInt32 monsterType, UInt32 killNum )
{
	UpdateTaskDetailStatusByNum( roleID, 1, monsterType, killNum );
}


//@brief	事件处理:背包物品增删
//				goalType 2
//@param	roleID				角色ID
//@param	itemID				物品ID
//@return	空
void TaskSvc::OnBagItemAddOrDelete( UInt32 roleID, UInt32 itemID )
{
	UpdateTaskDetailStatusByHoldItem( roleID, itemID );
}


//@brief	事件处理:物品使用
//				goalType 3
//@param	roleID				角色ID
//@param	itemID				物品ID
//@return	空
void TaskSvc::OnUseItem( UInt32 roleID, UInt32 itemID )
{
	UpdateTaskDetailStatusByNum( roleID, 3, itemID , 1 );
}

//@brief	事件处理:穿装备
//				goalType 4
//@param	roleID				角色ID
//@param	itemID				物品ID
//@return	空
void TaskSvc::OnRoleDress( UInt32 roleID, UInt32 itemID )
{
	UpdateTaskDetailStatusByNum( roleID, 4, itemID , 1 );
}


//@brief	事件处理:召唤宠物
//				goalType 5
//@param	roleID				角色ID
//@param	petType				宠物类型
//@return	空
void TaskSvc::OnCallPet( UInt32 roleID, UInt32 petType )
{
	UpdateTaskDetailStatusByNum( roleID, 5, petType , 1 );
}

//@brief	事件处理:学习技能
//				goalType 6
//@param	roleID				角色ID
//@return	空
void TaskSvc::OnRoleLearnSkill( UInt32 roleID )
{
	UpdateTaskDetailStatusByNum( roleID, 6, 0 , 1 );
}



//@brief	事件处理:装备合成
//				goalType 7
//@param	roleID				角色ID
//@return	空
void TaskSvc::OnComposeEquip( UInt32 roleID )
{
	UpdateTaskDetailStatusByNum( roleID, 7, 0 , 1 );
}

//@brief	事件处理:添加好友
//				goalType 8
//@param	roleID				角色ID
//@return	空
void TaskSvc::OnRoleAddFriend( UInt32 roleID )
{
	UpdateTaskDetailStatusByNum( roleID, 8, 0 , 1 );
}


//@brief	事件处理:组队参与
//				goalType 9
//@param	roleID				角色ID
//@return	空
void TaskSvc::OnRoleAddTeam( UInt32 roleID )
{
	UpdateTaskDetailStatusByNum( roleID, 9, 0 , 1 );
}


//执行任务接收条件脚本
//@return 0 成功,任务可接   非0 执行脚本失败，或者条件不满足
int TaskSvc::ExecCanAcceptScript( UInt32 roleID, const string& script )
{

	if( script.size() == 0 )
		return 0;

	//执行脚本
	lua_State* state = _luaState.GetState(RegisterTaskCAPI);

	SET_TASKSVC(state,this);
	SET_ROLEID(state, roleID );
	if(LuaState::Do(state, script))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"exec lua error, roleID[%d], script[%s], errorMsg[%s]",			roleID, script.c_str(), LuaState::GetErrorMsg(state));
		return -1;
	}

	return GetLuaRetCode(state);

}

//执行任务to-do脚本
//@return 0 成功,任务可接   非0 执行脚本失败，或者条件不满足
int TaskSvc::ExecToDoScript( UInt32 roleID, UInt32 taskID, const string& script )
{

	if( script.size() == 0 )
		return 0;

	//执行脚本
	lua_State* state = _luaState.GetState(RegisterTaskCAPI);

	SET_TASKSVC(state,this);
	SET_ROLEID(state, roleID );
	SET_TASKID(state, taskID );
	if(LuaState::Do(state, script))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"exec lua error, roleID[%d], script[%s], errorMsg[%s]",			roleID, script.c_str(), LuaState::GetErrorMsg(state));
		return -1;
	}

	return GetLuaRetCode(state);

}




//执行任务交付后的脚本
//@return 0 成功,任务可接   非0 执行脚本失败，或者条件不满足
int TaskSvc::ExecAfterDeliverScript( UInt32 roleID, const string& script )
{

	if( script.size() == 0 )
		return 0;

	//执行脚本
	lua_State* state = _luaState.GetState(RegisterTaskCAPI);

	SET_TASKSVC(state,this);
	SET_ROLEID(state, roleID );
	if(LuaState::Do(state, script))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"exec lua error, roleID[%d], script[%s], errorMsg[%s]",			roleID, script.c_str(), LuaState::GetErrorMsg(state));
		return -1;
	}

	return GetLuaRetCode(state);

}

//@brief	根据角色背包物品,更新相关任务的状态
//@param	roleID				角色ID
//@return	空
void TaskSvc::CheckAllItemTypeTaskStatus(UInt32 roleID)
{
	char szSql[1024];
	Connection con,conSub;
	DBOperate dbo,dboSub;
	int iRet = 0;
	list<UInt32> lItemID;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//查找背包物品列表
	sprintf( szSql, "select distinct(ItemID) \
									from Bag \
									where RoleID = %d ", roleID );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		return;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return;
 	}

 	while(dbo.HasRowData())
	{
		lItemID.push_back( dbo.GetIntField(0) );

	 	//记录集下一条记录
		dbo.NextRow();
	}

	list<UInt32>::iterator it;
	for( it = lItemID.begin(); it != lItemID.end(); it++ )
	{
//LOG(LOG_ERROR,__FILE__,__LINE__,"UpdateTaskDetailStatusByHoldItem  roleID[%d],itemID[%d] ", roleID, *it);
		UpdateTaskDetailStatusByHoldItem( roleID, *it );
	}

}


//@brief	根据背包物品数量(物品持有),更新任务细项状态
//@param	roleID
//@param	itemID
//@return	空
void TaskSvc::UpdateTaskDetailStatusByHoldItem( UInt32 roleID, UInt32 itemID )
{
	char szSql[1024];
	Connection con,conSub;
	DBOperate dbo,dboSub;
	int iRet = 0;
	UInt32 bagItemNum = 0;

	TaskDetail td;
	list<TaskDetail> ltd;
	list<TaskDetail>::iterator it;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	conSub = _cp->GetConnection();
	dboSub.SetHandle(conSub.GetHandle());

DEBUG_PRINTF2( "UpdateTaskDetailStatusByHoldItem ----- roleID[%d], itemID[%d] \n", roleID, itemID );

	//物品相关的任务明细
	sprintf( szSql, "select DetailID, TaskID, GoalNum \
									from RoleTaskDetail \
									where RoleID = %d \
										and GoalType = 2 \
										and GoalID = %d \
									order by DetailID \
								", roleID, itemID );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		//不存在物品相关的任务,直接返回
		return;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return;
 	}

	while(dbo.HasRowData())
	{
		td.detailID = dbo.GetIntField(0);
		td.taskID = dbo.GetIntField(1);
		td.goalNum = dbo.GetIntField(2);

		ltd.push_back(td);

	 	//记录集下一条记录
		dbo.NextRow();
	}


	//将该角色该物品相关的任务细表,置为初始状态
	//	即状态为未完成,完成数量为0
	sprintf( szSql, "update RoleTaskDetail \
									set IsFinish = 0, \
										FinishNum = 0 \
									where RoleID = %d \
										and GoalType = 2 \
										and GoalID = %d ", roleID, itemID );
	iRet = dbo.ExceSQL(szSql);
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return;
 	}

	//获取背包中该物品的总数量
	sprintf( szSql, "select Num \
									from Bag \
									where RoleID = %d \
										and ItemID = %d ", roleID, itemID );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		return;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return;
 	}

	while(dbo.HasRowData())
	{
		bagItemNum += dbo.GetIntField(0);

	 	//记录集下一条记录
		dbo.NextRow();
	}



LOG(LOG_ERROR,__FILE__,__LINE__,"bagItemNum[%d] ", bagItemNum);

	//将物品数量,从新分配给任务细项
	for( it = ltd.begin(); it != ltd.end(); it++ )
	{
		if( bagItemNum < it->goalNum )
		{//物品数量不足,更新该任务细项后,停止分配
 			sprintf( szSql, "update RoleTaskDetail \
											set IsFinish = 0, \
												FinishNum = %d \
											where DetailID = %d ", bagItemNum, it->detailID );
			iRet = dbo.ExceSQL(szSql);
			if( iRet < 0 )
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				return;
		 	}

			break;
		}
		else
		{//更新该任务细项为已完成
			sprintf( szSql, "update RoleTaskDetail \
											set IsFinish = 1, \
												FinishNum = %d \
											where DetailID = %d ", it->goalNum, it->detailID );
			iRet = dbo.ExceSQL(szSql);
			if( iRet < 0 )
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				return;
		 	}

		 	bagItemNum -= it->goalNum;
		}

		//调用 UpdateTaskStatus
		UpdateTaskStatus( roleID, it->taskID );

	}

	//任务状态通知
	for( it = ltd.begin(); it != ltd.end(); it++ )
	{
		NotifyTaskStatus( roleID, it->taskID );
	}

}






//@brief	根据任务目标的完成次数, 更新任务细项状态
//@param	roleID				角色ID
//@param	goalType			目标类型
//@param	goalID				目标ID
//@param	inputNum			最新完成的次数
//@return	空
void TaskSvc::UpdateTaskDetailStatusByNum( UInt32 roleID, Byte goalType, UInt32 goalID, UInt32 inputNum )
{
	char szSql[1024];
	Connection con,conSub;
	DBOperate dbo,dboSub;
	int iRet = 0;
	UInt32 detailID = 0;
	UInt32 taskID = 0;
	UInt32 goalNum = 0;
	UInt32 finishNum = 0;
	list<UInt32> lTaskID;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	conSub = _cp->GetConnection();
	dboSub.SetHandle(conSub.GetHandle());

DEBUG_PRINTF4( "UpdateTaskDetailStatusByNum ----- roleID[%d], goalType[%d], goalID[%d],inputNum[%d]  \n", roleID, goalType, goalID, inputNum );

	//数据校验
	if( 0 == inputNum )
		return;

	//查找相关的任务明细
	sprintf( szSql, "select DetailID, TaskID, GoalNum, FinishNum \
									from RoleTaskDetail rtd \
									where RoleID = %d \
										and GoalType = %d \
										and GoalID = %d \
										and IsFinish = 0 ", roleID, goalType, goalID);
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		return;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return;
 	}

 	while(dbo.HasRowData())
	{
		detailID = dbo.GetIntField(0);
		taskID = dbo.GetIntField(1);
		goalNum = dbo.GetIntField(2);
		finishNum = dbo.GetIntField(3);

		lTaskID.push_back(taskID);

		//是否达到目标
		if( (finishNum+inputNum) < goalNum )
		{
			//仅仅更新完成数量
			sprintf( szSql, " update RoleTaskDetail \
												set finishNum = %d \
												where DetailID = %d "
												, finishNum+inputNum, detailID );
			iRet = dboSub.ExceSQL(szSql);
			if( iRet < 0 )
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(conSub.GetHandle()), szSql);
				return;
		 	}
		}
		else
		{
			//更新任务细项为已完成
			sprintf( szSql, " update RoleTaskDetail \
												set IsFinish = 1, \
													finishNum = %d \
												where DetailID = %d "
												, goalNum, detailID );
			iRet = dboSub.ExceSQL(szSql);
			if( iRet < 0 )
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(conSub.GetHandle()), szSql);
				return;
		 	}

			//调用 UpdateTaskStatus
			UpdateTaskStatus( roleID, taskID);
		}

	 	//记录集下一条记录
		dbo.NextRow();
	}

	//任务状态通知
	list<UInt32>::iterator it;
	for( it = lTaskID.begin(); it != lTaskID.end(); it++ )
		NotifyTaskStatus( roleID, *it );
}



//@brief 任务状态更新
//	根据角色任务细表 RoleTaskDetail 的完成情况
//	更新任务主表 RoleTask 的任务状态为 '未完成' 或 '已完成' 或 '已交付'
//@param	roleID	角色ID
//@param	taskID	任务ID
//@return 空
void TaskSvc::UpdateTaskStatus( UInt32 roleID, UInt32 taskID )
{
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	Byte status = 0;
	int	hasNoTaskDetail = 0;
	int isChange2Finished = 0;				//任务状态,变成已完成
	UInt32 exitNpcID = 0;
	Byte finishStatus = 0;
	Byte addDeliverNum = 0;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//任务状态查询
	iRet = GetRoleTaskStatus( roleID, taskID, status );
	if( iRet )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"GetRoleTaskStatus error" );
		return;
	}

	//任务状态判断
	//	状态不能为未接 或 已交付
	if( status <= 1 || status >= 4 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"role status error, status[%d]", status );
		return;
	}

	//取任务明细
	sprintf( szSql, "select   RoleID, TaskID \
									from RoleTaskDetail \
									where RoleID = %d \
										and TaskID = %d ", roleID, taskID  );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		hasNoTaskDetail = 1;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return;
 	}

 	//取交付 Npc
 	sprintf( szSql, "select ExitNpcID \
									from Task \
									where TaskID = %d ", taskID  );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s]; task not exist!!, taskID[%d] " , szSql, taskID);
		return;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return;
 	}
 	exitNpcID = dbo.GetIntField(0);

 	//是否有任务明细
 	if( hasNoTaskDetail )
 	{//没有任务明细,属于谈话型任务

		if( 0 == exitNpcID )
		{//无需交付 npc的任务，直接置为已交付
			finishStatus = 2;
			addDeliverNum = 1;
		}
		else
		{//需交付 npc的任务，直接置为已完成
			finishStatus = 1;
			addDeliverNum = 0;
		}

 		sprintf( szSql, "update RoleTask \
		 									set FinishStatus = %d, \
		 											DeliverNum = DeliverNum + %d \
											where RoleID = %d \
												and TaskID = %d ", finishStatus, addDeliverNum, roleID, taskID );
		iRet = dbo.ExceSQL(szSql);
		if( iRet < 0 )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			return;
	 	}
 	}
 	else
 	{//有任务明细,判断根据明细完成情况，修改 任务状态
 		int isAllDetailFinish = 0;
		sprintf( szSql, "select DetailID \
									from RoleTaskDetail \
									where RoleID = %d \
										and TaskID = %d \
										and IsFinish = 0 ", roleID, taskID  );
		iRet = dbo.QuerySQL(szSql);
		if( iRet < 0 )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			return;
	 	}
	 	if( 1 == iRet )
		{
			isAllDetailFinish = 1;
		}
		else
		{
			isAllDetailFinish = 0;
		}

		//所有任务明细已完成
		if( isAllDetailFinish )
		{
			if( 0 == exitNpcID )
			{//无需交付 npc的任务，直接置为已交付
				finishStatus = 2;
				addDeliverNum = 1;
			}
			else
			{//需交付 npc的任务，直接置为已完成
				finishStatus = 1;
				addDeliverNum = 0;
			}

			sprintf( szSql, "update RoleTask \
		 									set FinishStatus = %d, \
		 											DeliverNum = DeliverNum + %d \
											where RoleID = %d \
												and TaskID = %d ", finishStatus, addDeliverNum, roleID, taskID );
			iRet = dbo.ExceSQL(szSql);
			if( iRet < 0 )
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				return;
		 	}
		}

 	}

 	//取任务状态结果
	sprintf( szSql, "select FinishStatus \
									from RoleTask \
									where RoleID = %d \
										and TaskID = %d ", roleID, taskID  );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s]; task not exist!!, taskID[%d] " , szSql, taskID);
		return;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return;
 	}
 	status = dbo.GetIntField(0);

	//状态为 已完成则发送 S_C 通知
	if( 1 == status )
	{
		_mainSvc->GetCoreData()->NotifyNpcStatus(roleID);	//[MsgType:0204](单播) Npc头顶状态
	}

}


//@brief 查找角色的职业
//@param	roleID	角色ID
//@param	proID	返回的角色职业
//@return 0 成功  非0 失败
int TaskSvc::GetRoleProID( UInt32 roleID, UInt32&proID )
{
	proID = 0;
	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	if(pRole->ID() == 0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error! roleID[%d] ", roleID );
		return -1;
	}

	proID = pRole->ProID();

	return 0;
}


//@brief 查找角色的任务状态
//@param	roleID	角色ID
//@param	taskID	任务ID
//@param	status	返回的 任务状态	1 未接  2  未完成  3 已完成  4 已交付
//@return 0 成功  非0 失败
int TaskSvc::GetRoleTaskStatus( UInt32 roleID, UInt32 taskID, Byte &status )
{
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	Byte taskType = 0;
	UInt32 maxAcceptNum = 0;
	UInt32 maxLevel = 0;


	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//初始化
	status = 0;

	//查看任务是否存在
	sprintf( szSql, "select   TaskType, MaxAcceptNum, MaxLevel \
									from Task \
									where TaskID = %d ", taskID  );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s]; task not exist!!, taskID[%d] " , szSql, taskID);
		return -1;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}

 	while(dbo.HasRowData())
	{
	 	taskType = dbo.GetIntField(0);
	 	maxAcceptNum = dbo.GetIntField(1);
	 	maxLevel = dbo.GetIntField(2);

	 	//记录集下一条记录
		dbo.NextRow();
	}


	//查找角色任务
	sprintf( szSql, "select FinishStatus, DeliverNum \
									from RoleTask \
									where RoleID = %d \
										and TaskID = %d ", roleID, taskID );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		status = 1;	//任务未接
		return 0;
 	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}

	while(dbo.HasRowData())
	{
		int finishStatus = dbo.GetIntField(0);
		int deliverNum = dbo.GetIntField(1);

		if( TASKFINISHSTATUS_NOTFINISHED == finishStatus )
		{
			status = 2;			//未完成
		}
		else if( TASKFINISHSTATUS_FINISHED == finishStatus )
		{
			status = 3;			//已完成
		}
		else if( TASKFINISHSTATUS_DELIVERED == finishStatus )
		{
			if( TASKTYPE_DAILY == taskType )
			{//日常任务

				//查角色信息
				RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
				if(pRole->ID()==0)
				{
					LOG(LOG_ERROR,__FILE__,__LINE__,"role not found! roleID[%d] ", roleID );
					return -1;
				}

				//是否可返回为未接状态
				//	未达日接收上限,且没有超出最大级别					返回未接
				//	否则返回已交付
				if( deliverNum < maxAcceptNum &&
						pRole->Level() <= maxLevel )
					status = 1;		//未接
				else
					status = 4;			//已交付
			}
			else
				status = 4;			//已交付
		}

		//记录集下一条记录
		dbo.NextRow();
	}


	return 0;
}


//@brief 查找角色任务细项
//@return 0 成功  非0 失败
int TaskSvc::GetRoleTaskDetail( UInt32 roleID, UInt32 taskID, List<ArchvTaskDetail>& ltd )
{
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	Byte taskType = 0;
	ArchvTaskDetail taskDetail;


	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//获取数据
	sprintf( szSql, " select GoalType, GoalID, IsFinish, GoalNum, FinishNum \
											from RoleTaskDetail \
											where RoleID = %d \
											and TaskID = %d ",
											roleID, taskID );
	iRet = dbo.QuerySQL(szSql);
 	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}

	while(dbo.HasRowData())
	{
		//插入数据
		taskDetail.goalType = dbo.GetIntField(0);
		taskDetail.goalID = dbo.GetIntField(1);
		taskDetail.isFinish = dbo.GetIntField(2);
		taskDetail.goalNum = dbo.GetIntField(3);
		taskDetail.finishNum = dbo.GetIntField(4);

		ltd.push_back( taskDetail );

		//记录集下一条记录
		dbo.NextRow();
	}

	return 0;
}


//@brief 增加任务明细
//	供 C API for lua  调用
//@param	roleID	角色ID
//@param	taskID	任务ID
//@goalType		目标类型 1 打怪  2 物品获取  3 游戏级别 4  捕获宠物  5 穿装备 6 物品使用 7商城购买
//@goalID			目标ID
//			       跟目标类型相关，怪的类型ID、物品ID、游戏级别
//@goalNum		目标数量
//@return 0 成功  非0 失败
int TaskSvc::AddRoleTaskDetail( UInt32 roleID, UInt32 taskID, Byte goalType, UInt32 goalID, UInt32 goalNum )
{
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//插入明细
	sprintf( szSql, " insert into RoleTaskDetail(DetailID, RoleID, TaskID, \
											GoalType, GoalID, IsFinish, GoalNum, FinishNum ) \
										values(NULL, %d, %d, %d, %d, 0, %d, 0) "
										, roleID, taskID, goalType, goalID, goalNum );
	iRet = dbo.ExceSQL(szSql);
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}


	return 0;
}


void TaskSvc::ProcessGetRoleTask(Session& session,Packet& packet)
{
 	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	UInt32 roleID = packet.RoleID;
	ArchvTaskInfo taskInfo;
	List<ArchvTaskInfo> lti;
	List<ArchvTaskInfo>::iterator it;
	ArchvTaskDetail taskDetail;

	//序列化类
	Serializer s(packet.GetBuffer());
//	s>>RoleID;

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
	if( 0 == pRole->ID() )
	{
		RetCode = ERR_APP_ROLENOTEXISTS;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error,role not exists, roleID[%d]", roleID  );
		goto EndOf_Process;
	}

	//查找角色所有的未接任务
	//	查找已接已完成，未达到上限的日常任务
	sprintf( szSql, "select   TaskID, TaskName, TaskType, 1 as TaskStatus, CanAcceptScript \
									from Task \
									where MinLevel <= %d \
										and TaskID not in (  select TaskID \
																						from RoleTask  \
																						where RoleID = %d \
																				) \
									union \
									select   t.TaskID, t.TaskName, t.TaskType, 1 as TaskStatus, t.CanAcceptScript  \
									from Task t, RoleTask rt \
									where t.TaskType = 3 \
										and t.MinLevel <= %d \
										and t.MaxLevel >= %d \
										and rt.RoleID = %d \
										and t.TaskID = rt.TaskID \
										and rt.FinishStatus = 2 \
										and t.MaxAcceptNum > rt.DeliverNum \
										", pRole->Level(), roleID,
										pRole->Level(), pRole->Level(), roleID	);
	iRet = dbo.QuerySQL(szSql);
 	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}

	while(dbo.HasRowData())
	{
		//脚本执行，判断是否满足可接条件
		iRet = ExecCanAcceptScript( roleID, dbo.GetStringField(4));
		if( 0 == iRet )
		{
			//插入数据
			taskInfo.taskID= dbo.GetIntField(0);
			taskInfo.taskName = dbo.GetStringField(1);
			taskInfo.taskType = dbo.GetIntField(2);
			taskInfo.taskStatus= dbo.GetIntField(3);

			lti.push_back( taskInfo );
		}

		//记录集下一条记录
		dbo.NextRow();
	}


	//查找角色的已接任务
 	sprintf( szSql, "select   TaskID, TaskName, TaskType, 2 as TaskStatus \
								from Task \
								where  TaskID in (  select TaskID \
																					from RoleTask \
																					where RoleID = %d \
																						and FinishStatus = 0 \
																			) \
								union select   TaskID, TaskName,TaskType, 3 as TaskStatus \
								from Task \
								where TaskID in (  select TaskID \
																					from RoleTask \
																					where RoleID = %d \
																						and FinishStatus = 1 \
																			) \
									", roleID, roleID  );

	iRet = dbo.QuerySQL(szSql);
 	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}

	while(dbo.HasRowData())
	{
		//插入数据
		taskInfo.taskID= dbo.GetIntField(0);
		taskInfo.taskName = dbo.GetStringField(1);
		taskInfo.taskType = dbo.GetIntField(2);
		taskInfo.taskStatus= dbo.GetIntField(3);

		lti.push_back( taskInfo );

		//记录集下一条记录
		dbo.NextRow();
	}

	//查任务明细
	for( it = lti.begin(); it != lti.end(); it++ )
	{
		sprintf( szSql, " select GoalType, GoalID, IsFinish, GoalNum, FinishNum \
												from RoleTaskDetail \
												where RoleID = %d \
												and TaskID = %d ",
												roleID, it->taskID );
		iRet = dbo.QuerySQL(szSql);
	 	if( iRet < 0 )
		{
			RetCode = ERR_SYSTEM_DBERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			goto EndOf_Process;
	 	}

		while(dbo.HasRowData())
		{
			//插入数据
			taskDetail.goalType = dbo.GetIntField(0);
			taskDetail.goalID = dbo.GetIntField(1);
			taskDetail.isFinish = dbo.GetIntField(2);
			taskDetail.goalNum = dbo.GetIntField(3);
			taskDetail.finishNum = dbo.GetIntField(4);

			it->ltd.push_back( taskDetail );

			//记录集下一条记录
			dbo.NextRow();
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


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode 为0 才会返回包体剩下内容
		s<<lti;
	}

	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	return;

}




void TaskSvc::ProcessAcceptTask(Session& session,Packet& packet)
{
 	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	UInt32 roleID = packet.RoleID;
	UInt32 taskID;
	UInt32	taskType = 0;
	UInt32	minLevel = 0;
	string canAcceptScript;
	string toDoScript;
	string afterDeliverScript;
	int dateDiff = 0;
	Byte status = 0;
	UInt32 maxAcceptNum = 0;
	UInt32 maxLevel = 0;
	int iHasRecord = 0;					//该角色 在RoleTask 是否有该任务记录

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>taskID;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());


	//获取任务描述信息
	sprintf( szSql, "select TaskType, MinLevel, CanAcceptScript, TodoScript, AfterDeliverScript, MaxAcceptNum, MaxLevel \
									from Task \
									where TaskID = %d ", taskID );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
 	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}

	//数据获取
 	taskType = dbo.GetIntField(0);
 	minLevel = dbo.GetIntField(1);
 	canAcceptScript = dbo.GetStringField(2);
 	toDoScript = dbo.GetStringField(3);
 	afterDeliverScript = dbo.GetStringField(4);

 	maxAcceptNum = dbo.GetIntField(5);
 	maxLevel = dbo.GetIntField(6);

	//获取角色信息
	if( 0 == pRole->ID() )
	{
		RetCode = ERR_APP_ROLENOTEXISTS;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error,role not exists, roleID[%d]", roleID  );
		goto EndOf_Process;
	}

	//任务等级校验
	if( pRole->Level() < minLevel || pRole->Level() > maxLevel )
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"level error!roleLevel[%d],minLevel[%d], maxLevle[%d]", pRole->Level(), minLevel, maxLevel );
		goto EndOf_Process;
	}

	//任务状态获取
	iRet = GetRoleTaskStatus( roleID, taskID, status);
	if(iRet)
	{
		RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"GetRoleTaskStatus error !!!!roleID[%d],taskID[%d]", roleID, taskID );
		goto EndOf_Process;
	}

	//任务状态校验,状态必须为 未接
	if( 1 != status )
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"taskStatus error!! taskStatus[%d] must be 1 !roleID[%d],taskID[%d]", status, roleID, taskID );
		goto EndOf_Process;
	}

	//满足可接条件,则接收任务
	iRet = ExecCanAcceptScript( roleID, canAcceptScript );
	if( 0 == iRet )
	{
		//检查该角色的任务记录是否已经存在
		sprintf( szSql, "select   TaskID \
										from RoleTask \
										where RoleID = %d \
											and TaskID = %d \
											", roleID, taskID );
		iRet = dbo.QuerySQL(szSql);
	 	if( iRet < 0 )
		{
			RetCode = ERR_SYSTEM_DBERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			goto EndOf_Process;
	 	}
	 	if( 1 == iRet )
	 		iHasRecord = 0;
	 	else
	 		iHasRecord = 1;


		//清理该角色在 RoleTaskDetail 所有的冗余
		//	防止异常数据干扰后续所有的任务相关操作
		sprintf( szSql, " delete from RoleTaskDetail \
											where RoleID = %d \
												and not exists ( select TaskID \
																					from RoleTask \
																					where RoleID = %d \
																				)",
										roleID, taskID );
		iRet = dbo.ExceSQL(szSql);
		if( iRet < 0 )
		{
			RetCode = ERR_SYSTEM_DBERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			goto EndOf_Process;
	 	}

		//任务to-do脚本的执行，用以向 RoleTaskDetail 增加记录
		iRet = ExecToDoScript( roleID, taskID, toDoScript );
		if( iRet )
		{
			RetCode = ERR_APP_OP;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExecToDoScript error, roleID[%d], taskID[%d], toDoScript[%s]", roleID, taskID, toDoScript.c_str() );
			goto EndOf_Process;
		}

		//非日常任务, RoleTask 如果有记录，直接删除
		if( TASKTYPE_DAILY != taskType && iHasRecord )
		{
			//清理 RoleTask 表
			//	防止异常数据干扰
		 	sprintf( szSql, " delete from RoleTask \
												where RoleID = %d \
													and TaskID = %d ",
											roleID, taskID );
			iRet = dbo.ExceSQL(szSql);
			if( iRet < 0 )
			{
				RetCode = ERR_SYSTEM_DBERROR;
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				goto EndOf_Process;
		 	}
		}

		//根据任务类型 RoleTask 表 增加或者更新记录
		if( iHasRecord )
		{
			//有记录的 日常任务,直接更新
			sprintf( szSql, " update RoleTask \
												set FinishStatus = 0 \
												where RoleID = %d \
													and TaskID = %d ",
													roleID, taskID );
			iRet = dbo.ExceSQL(szSql);
			if( iRet < 0 )
			{
				RetCode = ERR_SYSTEM_DBERROR;
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				goto EndOf_Process;
		 	}
		}
		else
		{
 			//插入 RoleTask 表
			sprintf( szSql, " insert into RoleTask( RoleId, TaskID, FinishStatus, DeliverNum) \
												values( %d, %d, 0, 0 ) ",
													roleID, taskID );
			iRet = dbo.ExceSQL(szSql);
			if( iRet < 0 )
			{
				RetCode = ERR_SYSTEM_DBERROR;
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				goto EndOf_Process;
		 	}
	 	}
	}

	//根据背包物品,更新任务细项状态
	CheckAllItemTypeTaskStatus(roleID);

	//满足完成条件，则任务状态改变
	UpdateTaskStatus( roleID, taskID );

	//最新的任务状态获取,用于返回
	iRet = GetRoleTaskStatus( roleID, taskID, status);
	if(iRet)
	{
		RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"GetRoleTaskStatus error !!!!roleID[%d],taskID[%d]", roleID, taskID );
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


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode 为0 才会返回包体剩下内容
		s<<taskID<<status;
	}

	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	//---------------------------------S_C 通知-----------------------------------------
	//成功处理，则发送广播
	if( 0 == RetCode )
	{
		//任务状态
		NotifyTaskStatus( roleID, taskID);

		//npc头顶状态
		_mainSvc->GetCoreData()->NotifyNpcStatus(roleID);
	}


	return;

}





void TaskSvc::ProcessDeliverTask(Session& session,Packet& packet)
{
 	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	UInt32 roleID = packet.RoleID;
	UInt32 taskID =0;
	UInt32 npcID = 0;
	Byte	taskType = 0;
	UInt32 exitNpcID = 0;
	string	canAcceptScript;
	string	toDoScript;
	string	afterDeliverScript;
	Byte finishStatus = 0;
	Byte status = 0;

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>taskID>>npcID;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//获取任务描述信息
	sprintf( szSql, "select TaskType, ExitNpcID, CanAcceptScript, TodoScript, AfterDeliverScript \
									from Task \
									where TaskID = %d ", taskID );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
 	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}

	//数据获取
 	taskType = dbo.GetIntField(0);
 	exitNpcID = dbo.GetIntField(1);
 	canAcceptScript = dbo.GetStringField(2);
 	toDoScript = dbo.GetStringField(3);
 	afterDeliverScript = dbo.GetStringField(4);

 	//npc校验
 	if( npcID != exitNpcID )
 	{
 		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"not the right NPC, req npcID[%d], taskID[%d],  exitNpcID[%d]", npcID, taskID, exitNpcID );
		goto EndOf_Process;
 	}

	//任务状态获取
	iRet = GetRoleTaskStatus( roleID, taskID, status);
	if(iRet)
	{
		RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"GetRoleTaskStatus error !!!!roleID[%d],taskID[%d]", roleID, taskID );
		goto EndOf_Process;
	}

	//任务状态校验,状态必须为已完成
	if( 3 != status )
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"error:task is not Finished!!!!roleID[%d],taskID[%d]", roleID, taskID );
		goto EndOf_Process;
	}

 	//删除 RoleTaskDetail表
	sprintf( szSql, " delete from RoleTaskDetail \
										where RoleID = %d \
 											and TaskID = %d ",
										roleID, taskID );
	iRet = dbo.ExceSQL(szSql);
	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}

 	//修改任务状态
 	sprintf( szSql, " update RoleTask \
 										set FinishStatus = 2, \
 											DeliverNum = DeliverNum + 1 \
										where RoleID = %d \
 											and TaskID = %d ",
										roleID, taskID );
	iRet = dbo.ExceSQL(szSql);
	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}

	//任务交付脚本执行
	iRet = ExecAfterDeliverScript( roleID, afterDeliverScript );
	if( iRet )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExecAfterDeliverScript error, roleID[%d], taskID[%d], afterDeliverScript[%s]", roleID, taskID, afterDeliverScript.c_str() );
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
	{//RetCode 为0 才会返回包体剩下内容
		s<<taskID;
	}

	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	//---------------------------------S_C 通知-----------------------------------------
	//成功处理，则发送广播
	if( 0 == RetCode )
	{
		_mainSvc->GetCoreData()->NotifyNpcStatus(roleID);	//Npc头顶状态更新

		NotifyTaskCanAccept(roleID);		//角色可接任务更新通知
	}

	return;

}


void TaskSvc::ProcessAbandonTask(Session& session,Packet& packet)
{
 	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	UInt32 roleID = packet.RoleID;
	UInt32 taskID =0;
	Byte	taskType = 0;
	Byte	finishStatus = 0;
	int dateDiff = 0;
	Byte status = 0;

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>taskID;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//任务状态获取
	iRet = GetRoleTaskStatus( roleID, taskID, status);
	if(iRet)
	{
		RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"GetRoleTaskStatus error !!!!roleID[%d],taskID[%d]", roleID, taskID );
		goto EndOf_Process;
	}

	//任务状态校验, 状态必须为 2未完成 或者  3已完成
	if( status!= 2 && status != 3 )
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"role taskStatus error!roleID[%d],taskID[%d],status[%d]", roleID, taskID, status );
		goto EndOf_Process;
	}

	//获取任务描述信息
	sprintf( szSql, "select TaskType \
									from Task \
									where TaskID = %d ", taskID );
	iRet = dbo.QuerySQL(szSql);
 	if( iRet )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}

	//数据获取
 	taskType = dbo.GetIntField(0);


 	//删除 RoleTaskDetail表
	sprintf( szSql, " delete from RoleTaskDetail \
										where RoleID = %d \
 											and TaskID = %d ",
										roleID, taskID );
	iRet = dbo.ExceSQL(szSql);
	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}

	//根据任务类型,删除或者更新记录
	if( TASKTYPE_DAILY != taskType )
	{
		//删除 RoleTask表记录
		sprintf( szSql, " delete from RoleTask \
											where RoleID = %d \
													and TaskID = %d ",
											roleID, taskID );
		iRet = dbo.ExceSQL(szSql);
		if( iRet < 0 )
		{
			RetCode = ERR_SYSTEM_DBERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			goto EndOf_Process;
		}
	}
	else
	{
		//日常任务,则更新 RoleTask表 的任务状态为 已交付
		sprintf( szSql, " update RoleTask \
											set FinishStatus = 2 \
											where RoleID = %d \
													and TaskID = %d ",
											roleID, taskID );
		iRet = dbo.ExceSQL(szSql);
		if( iRet < 0 )
		{
			RetCode = ERR_SYSTEM_DBERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
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


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode 为0 才会返回包体剩下内容
//		s<<ltib;
	}

	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	//---------------------------------S_C 通知-----------------------------------------
	//成功处理，则发送广播
	if( 0 == RetCode )
	{
		_mainSvc->GetCoreData()->NotifyNpcStatus(roleID);
	}

	return;

}

int TaskSvc::AddRoleExp( UInt32 roleID, UInt32 exp )
{
	return _mainSvc->GetCoreData()->RoleExpAdd( roleID,exp);
}

int TaskSvc::AddRoleMoney( UInt32 roleID,UInt32 money )
{
	return _mainSvc->GetBagSvc()->Getmoney(roleID,money);
}


int TaskSvc::AddRoleBindMoney( UInt32 roleID,UInt32 money )
{
	return _mainSvc->GetBagSvc()->GetBindMoney(roleID,money);
}


int TaskSvc::AddRoleItem( UInt32 roleID ,UInt32 ItemID,UInt32 num )
{
	List<ItemList> it;
	ItemList p;
	p.ItemID=ItemID;
	p.num=num;
	it.push_back(p);
	int iRet=0;
	iRet= _mainSvc->GetBagSvc()->RoleGetItem(roleID,it);
	return iRet;
}

int TaskSvc::DeleteItem(UInt32 roleID,UInt32 ItemID,UInt32 num)
{
		List<ItemList> it;
		ItemList p;
		p.ItemID=ItemID;
		p.num=num;
		it.push_back(p);
		int iRet=0;
		iRet= _mainSvc->GetBagSvc()->RoleDropItem(roleID, it);
		return iRet;
}


void TaskSvc::callNotifybag(UInt32 roleID,List < ItemCell > & lic,Byte flag)
{
	_mainSvc->GetBagSvc()->NotifyBag(roleID,lic);
}



//@brief	获取角色当前所有可接任务
//@return 空
void TaskSvc::GetRoleCanAcceptTask(UInt32 roleID, List<ArchvTaskInfo>& lti)
{
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	ArchvTaskInfo taskInfo;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//获取角色信息
	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr( roleID);
	if( 0 == pRole->ID() )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error,role not exists, roleID[%d]", roleID  );
		return;
	}

	//查找角色所有的未接任务
	//	查找已接已完成，未达到上限的日常任务
	sprintf( szSql, "select   TaskID, TaskName, TaskType, 1 as TaskStatus, CanAcceptScript \
									from Task \
									where MinLevel <= %d \
										and TaskID not in (  select TaskID \
																						from RoleTask  \
																						where RoleID = %d \
																				) \
									union \
									select   t.TaskID, t.TaskName, t.TaskType, 1 as TaskStatus, t.CanAcceptScript  \
									from Task t, RoleTask rt \
									where t.TaskType = 3 \
										and t.MinLevel <= %d \
										and t.MaxLevel >= %d \
										and rt.RoleID = %d \
										and t.TaskID = rt.TaskID \
										and rt.FinishStatus = 2 \
										and t.MaxAcceptNum > rt.DeliverNum \
										", pRole->Level(), roleID,
										pRole->Level(), pRole->Level(), roleID	);
	iRet = dbo.QuerySQL(szSql);
 	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return;
 	}

	while(dbo.HasRowData())
	{
		//脚本执行，判断是否满足可接条件
		iRet = ExecCanAcceptScript( roleID, dbo.GetStringField(4));
		if( 0 == iRet )
		{
			//插入数据
			taskInfo.taskID= dbo.GetIntField(0);
			taskInfo.taskName = dbo.GetStringField(1);
			taskInfo.taskType = dbo.GetIntField(2);
			taskInfo.taskStatus= dbo.GetIntField(3);

			lti.push_back( taskInfo );
		}

		//记录集下一条记录
		dbo.NextRow();
	}

	//查任务明细
	List<ArchvTaskInfo>::iterator it;
	ArchvTaskDetail taskDetail;
	for( it = lti.begin(); it != lti.end(); it++ )
	{
		sprintf( szSql, " select GoalType, GoalID, IsFinish, GoalNum, FinishNum \
												from RoleTaskDetail \
												where RoleID = %d \
												and TaskID = %d ",
												roleID, it->taskID );
		iRet = dbo.QuerySQL(szSql);
	 	if( iRet < 0 )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			return;
	 	}

		while(dbo.HasRowData())
		{
			//插入数据
			taskDetail.goalType = dbo.GetIntField(0);
			taskDetail.goalID = dbo.GetIntField(1);
			taskDetail.isFinish = dbo.GetIntField(2);
			taskDetail.goalNum = dbo.GetIntField(3);
			taskDetail.finishNum = dbo.GetIntField(4);

			it->ltd.push_back( taskDetail );

			//记录集下一条记录
			dbo.NextRow();
		}
	}

	return;
}

//@brief : 获取角色已接未完成的任务信息
//@param: roleTask, 角色未完成的任务信息
//return :  返回0 成功，非 0 失败
int TaskSvc::GetRoleUnFinishedTask(list<UInt32>licRoleID,List<ArchvUnfinishedTask>&licRoleTask)
{
	char szSql[1024];
	int iRet = 0;
	UInt32 taskID = 0;
    ArchvUnfinishedTask roleTask;
	list<UInt32>::iterator itor;
	Connection con;
	DBOperate dbo;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	for(itor = licRoleID.begin(); itor != licRoleID.end(); itor++)
	{
		sprintf(szSql,"select TaskID from RoleTaskDetail \
		           where RoleID =%d and GoalType = 2 and IsFinish = 0;",*itor);
		iRet = dbo.QuerySQL(szSql);
		if(iRet == 0)
		{
			roleTask.roleID = *itor;
			while(dbo.HasRowData())
			{
				taskID = dbo.GetIntField(0);
				roleTask.licTaskID.push_back(taskID);
				dbo.NextRow();
			}

			licRoleTask.push_back(roleTask);
		}
		else
		{
			continue;
		}
	}

	if(licRoleTask.size() != 0)
	{
		return 0;
	}

	return -1;

}



//@brief	[MsgType:0701]任务主细项状态通知；仅限主任务状态为未完成、已完成时；
//	单播
void TaskSvc::NotifyTaskStatus( UInt32 roleID, UInt32 taskID )
{
	List<UInt32>lrid;

	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);

	Byte	taskStatus = 0;
	List<ArchvTaskDetail> ltd;
	int iRet = 0;

	//组包头
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 701;
	p.UniqID = 123;

	p.PackHeader();


	//获取任务主项状态
	iRet = GetRoleTaskStatus( roleID, taskID, taskStatus );
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"GetRoleTaskStatus! roleID[%d], taskID[%d] ", roleID, taskID );
		return;
	}

	//获取任务细项数据
	iRet = GetRoleTaskDetail( roleID, taskID, ltd );
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"GetRoleTaskDetail! roleID[%d], taskID[%d] ", roleID, taskID );
		return;
	}

	//写包体
	s<<taskID<<taskStatus<<ltd;
	p.UpdatePacketLength();


	lrid.clear();
	lrid.push_back(roleID);
	if( _mainSvc->Service()->Broadcast( lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size()[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );


}


//[MsgType:0702]角色可接任务更新通知；单播；每次交付任务时发起
void TaskSvc::NotifyTaskCanAccept( UInt32 roleID )
{
	List<UInt32>lrid;

	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);

	List<ArchvTaskInfo> lti;

	//组包头
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 702;
	p.UniqID = 123;

	p.PackHeader();


	//获取任务主项状态
	GetRoleCanAcceptTask( roleID, lti);

	//写包体
	s<<lti;
	p.UpdatePacketLength();


	lrid.clear();
	lrid.push_back(roleID);
	if( _mainSvc->Service()->Broadcast( lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size()[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );


}

