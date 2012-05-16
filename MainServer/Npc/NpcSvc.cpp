#include "NpcSvc.h"
#include "MainSvc.h"
#include "ArchvDialogItem.h"
#include "DBOperate.h"
#include "NpcLua.h"
#include "ArchvNpc.h"
#include "ArchvTask.h"
#include "CoreData.h"
#include "Role.h"
#include "../Task/TaskSvc.h"

using namespace NpcLua;

NpcSvc::NpcSvc(void* service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;

}

NpcSvc::~NpcSvc()
{
}

void NpcSvc::OnProcessPacket(Session& session,Packet& packet)
{
	
DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 601: //Npc对话
			ProcessNpcTalk(session,packet);
			break;

		case 602: //Npc商店物品查询
			ProcessGetNpcShopItem(session,packet);
			break;

		default:
			ClientErrorAck(session,packet,ERR_SYSTEM_PARAM);
			LOG(LOG_ERROR,__FILE__,__LINE__,"MsgType[%d] not found",packet.MsgType);
			break;
		}
}


//执行任务接收条件脚本
//@return 0 成功,任务可接   非0 执行脚本失败，或者条件不满足
int NpcSvc::ExecCanAcceptScript( UInt32 roleID, const string& script )
{

	if( script.size() == 0 )
		return 0;

	//执行脚本
	lua_State* state = _luaState.GetState(RegisterNpcCAPI);

	SET_TASKSVC(state, _mainSvc->GetTaskSvc());
	SET_NPCSVC(state,this);
	SET_ROLEID(state, roleID );
	if(LuaState::Do(state, script))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"exec lua error, roleID[%d], script[%s], errorMsg[%s]",			roleID, script.c_str(), LuaState::GetErrorMsg(state));
		return -1;
	}

	return GetLuaRetCode(state);

}



//客户端错误应答
//@param  session 连接对象
//@param	packet 请求包
//@param	RetCode 返回errorCode 值
void NpcSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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

//@brief	获取角色在npc头顶状态
//	每个角色在每个npc会有多个任务状态(未接,未完成,已完成)
//	但是只能显示一个状态,优先级: 已完成-->未完成-->未接
//@param	status	返回的任务状态
//							 0 没有任务	1 可接 2 未完成 3 已完成
//@return 	0 成功   非0 失败
int NpcSvc::GetNpcStatus( UInt32 roleID, UInt32 level, UInt32 npcID, Byte& status )
{
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	Byte taskType = 0;
	status = 0;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());


	//查找角色在该npc的已完成任务
 	sprintf( szSql, "select 3 as TaskStatus \
								from Task \
								where ExitNpcID = %d \
									and TaskID in (  select TaskID \
																					from RoleTask \
																					where RoleID = %d \
																						and FinishStatus = 1 \
																			) ", npcID, roleID  );

	iRet = dbo.QuerySQL(szSql);
 	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}

	while(dbo.HasRowData())
	{
		//获取数据
		status = dbo.GetIntField(0);
		return 0;

		//记录集下一条记录
		dbo.NextRow();
	}

	//查找角色在该npc的未完成任务
 	sprintf( szSql, "select 2 as TaskStatus \
								from Task \
								where ExitNpcID = %d \
									and TaskID in (  select TaskID \
																					from RoleTask \
																					where RoleID = %d \
																						and FinishStatus = 0 \
																			) ", npcID, roleID  );

	iRet = dbo.QuerySQL(szSql);
 	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}

	while(dbo.HasRowData())
	{
		//获取数据
		status = dbo.GetIntField(0);
		return 0;

		//记录集下一条记录
		dbo.NextRow();
	}
	

	//查找角色在该npc的未接任务
	//	查找已接已完成，未达到上限的日常任务
	sprintf( szSql, "select 1 as TaskStatus, CanAcceptScript \
									from Task \
									where EntryNpcID = %d \
										and MinLevel <= %d \
										and TaskID not in (  select TaskID \
																						from RoleTask  \
																						where RoleID = %d \
																				) \
									union \
									select 1 as TaskStatus, t.CanAcceptScript \
									from Task t, RoleTask rt \
									where t.EntryNpcID = %d \
										and t.MinLevel <= %d \
										and t.MaxLevel >= %d \
										and t.TaskType = 3 \
										and rt.RoleID = %d \
										and t.TaskID = rt.TaskID \
										and rt.FinishStatus = 2 \
										and t.MaxAcceptNum > rt.DeliverNum \
										", npcID, level, roleID,
										 npcID, level, level, roleID );
	iRet = dbo.QuerySQL(szSql);
 	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}
	
	while(dbo.HasRowData())
	{
		//脚本执行，判断是否满足可接条件
		iRet = ExecCanAcceptScript( roleID, dbo.GetStringField(1));
		if( 0 == iRet )
		{
			//获取数据
			status = dbo.GetIntField(0);
			return 0;
		}

		//记录集下一条记录
		dbo.NextRow();
	}

	return 0;
}


void NpcSvc::ProcessNpcTalk(Session& session,Packet& packet)
{

	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	UInt32 npcID;
	UInt32 roleID = packet.RoleID;
	//获取角色信息
	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	ArchvTaskInfoBrief taskInfoBrief;
	List<ArchvDialogItem> ldi;
	string canAcceptScript;

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>npcID;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	if( 0 == pRole->ID() )
	{
		RetCode = ERR_SYSTEM_DATANOTEXISTS;
		LOG(LOG_ERROR,__FILE__,__LINE__,"roleID[%d] not exists", roleID  );
		goto EndOf_Process;
	}


	//查找角色在该npc的未接任务
	//	查找已接已完成，未达到上限的日常任务
	sprintf( szSql, "select   TaskID, TaskName, TaskType, 1 as TaskStatus, CanAcceptScript, ifnull(AdviceLevel ,0) \
									from Task \
									where EntryNpcID = %d \
										and MinLevel <= %d \
										and TaskID not in (  select TaskID \
																						from RoleTask  \
																						where RoleID = %d \
																				) \
									union \
									select   t.TaskID, t.TaskName, t.TaskType, 1 as TaskStatus, t.CanAcceptScript, ifnull(t.AdviceLevel, 0)  \
									from Task t, RoleTask rt \
									where t.EntryNpcID = %d \
										and t.MinLevel <= %d \
										and t.MaxLevel >= %d \
										and t.TaskType = 3 \
										and rt.RoleID = %d \
										and t.TaskID = rt.TaskID \
										and rt.FinishStatus = 2 \
										and t.MaxAcceptNum > rt.DeliverNum \
										", npcID, pRole->Level(), roleID,
										 npcID, pRole->Level(), pRole->Level(), roleID );
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

DEBUG_PRINTF1( " ------CanAcceptScript[%s] \n", dbo.GetStringField(4) );

		iRet = ExecCanAcceptScript( roleID, dbo.GetStringField(4));

DEBUG_PRINTF1( " ------iRet[%d] \n", iRet );

		if( 0 == iRet )
		{
			//插入数据
			taskInfoBrief.taskID= dbo.GetIntField(0);
			taskInfoBrief.taskName = dbo.GetStringField(1);
			taskInfoBrief.taskType = dbo.GetIntField(2);
			taskInfoBrief.taskStatus= dbo.GetIntField(3);
			taskInfoBrief.adviceLevel = dbo.GetIntField(5);

			ldi.push_back( ArchvDialogItem( 2, taskInfoBrief ) );
		}

		//记录集下一条记录
		dbo.NextRow();
	}


	//查找角色在该npc的已接任务
 	sprintf( szSql, "select   TaskID, TaskName, TaskType, 2 as TaskStatus, ifnull(AdviceLevel, 0) \
								from Task \
								where ExitNpcID = %d \
									and TaskID in (  select TaskID \
																					from RoleTask \
																					where RoleID = %d \
																						and FinishStatus = 0 \
																			) \
								union select   TaskID, TaskName,TaskType, 3 as TaskStatus, ifnull(AdviceLevel,0) \
								from Task \
								where ExitNpcID = %d \
									and TaskID in (  select TaskID \
																					from RoleTask \
																					where RoleID = %d \
																						and FinishStatus = 1 \
																			) \
									", npcID, roleID, npcID, roleID  );

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
		taskInfoBrief.taskID= dbo.GetIntField(0);
		taskInfoBrief.taskName = dbo.GetStringField(1);
		taskInfoBrief.taskType = dbo.GetIntField(2);
		taskInfoBrief.taskStatus= dbo.GetIntField(3);
		taskInfoBrief.adviceLevel = dbo.GetIntField(4);
		
 
		ldi.push_back( ArchvDialogItem( 2, taskInfoBrief ) );
 
		//记录集下一条记录
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
	{//RetCode 为0 才会返回包体剩下内容
		s<<npcID<<ldi;
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


 


void NpcSvc::ProcessGetNpcShopItem(Session& session,Packet& packet)
{

	List<UInt32> lItemID;
/*
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	UInt32 RoleID;
	string strDisplay;
	

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>RoleID>>strDisplay;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
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
//		s<<ari;
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
		//发送广播
		NotifyScreenDisplay( RoleID, strDisplay );
 	}

 	*/

	return;
	
}



