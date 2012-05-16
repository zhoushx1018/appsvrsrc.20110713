
#include "MainSvc.h"
#include "DBOperate.h"
#include "CoreData.h"
#include "Role.h"
#include "TeamSvc.h"	
#include "ArchvTeam.h"
#include "ArchvRole.h"

TeamSvc::TeamSvc(void* service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;

}

TeamSvc::~TeamSvc()
{

}
//数据包信息
void TeamSvc::OnProcessPacket(Session& session,Packet& packet)
{
DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 1401: //申请
			ProcessRequestjoin(session,packet);
		break;			
		case 1402:
			ProcessAnswerRequest(session,packet);
		break;
		case 1403:
			ProcessChangeCaptain(session,packet);
		break;
		case 1404:
			ProcessCallMember(session,packet);
		break;
		case 1405:
			ProcessCapt_Member(session,packet);
		break;
		case 1406:
			ProcessMember_OUt(session,packet);
		break;
		case 1407:
			ProcessBackOrleave(session,packet);
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
void TeamSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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

void TeamSvc::ProcessRequestjoin(Session& session,Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);

	UInt32 roleID = packet.RoleID;
	UInt32 ID=0;//申请对象的角色ID

	string Name;
	List<UInt32> it;
	UInt32 teamID=0;
	int iRet=0;
	Team team;
	UInt32 RoleID1=0;

		//序列化类
	Serializer s(packet.GetBuffer());
	s>>ID;
	
	RolePtr pRole=_mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}
	if(ID==0||ID==roleID)
	{
			RetCode = 199;//向自己申请组队
			LOG(LOG_ERROR,__FILE__,__LINE__,"there are some erro in the ID %d vs ResustID%d",ID,roleID );
			goto EndOf_Process;
	}
	//邀请者判断	

	
	if(pRole->ID()!=0)
	{
		Name=pRole->Name();
	}
	else
	{
		 RetCode = ERR_SYSTEM_DBNORECORD;
		 LOG(LOG_ERROR,__FILE__,__LINE__,"there are some erro in the ID %d vs ResustID%d",ID,roleID );
		 goto EndOf_Process;
	}
	
	if(pRole->TeamFlag()==0)
	{
		//没有组队

		//对方，看看有没有组队
			RoleID1=_mainSvc->GetCoreData()->ProcessGetRolePtr(ID)->LeaderRoleID();
			if(RoleID1==0)
			{
				//没有找到，组队起来,都没有组队
				//roleID为队长
			}
			else
			{
				ID=RoleID1;
				//找到
				//向队长邀请组队
			}
	}
	else if(pRole->TeamFlag()==2)
	{
		//队长
		//发起组队的邀请
			if(_mainSvc->GetCoreData()->ProcessGetRolePtr(ID)->TeamID()!=0)
			{
				 RetCode = 200;//对方有组队
				 LOG(LOG_ERROR,__FILE__,__LINE__,"there are some erro in the ID %d vs ResustID%d",ID,roleID );
				 goto EndOf_Process;
			}
	}
	else
	{	
		//有组队，但是不是队长，无权操作
		//向队长发起组队申请
		RetCode = 201;//有组队，不是队长
		LOG(LOG_ERROR,__FILE__,__LINE__,"you are not the leader");
		 goto EndOf_Process;
	}
	//判断另外一个人队伍情况
	DEBUG_PRINTF( "C_S ProcessProcessChat sucess!!!!!!!!\n" );	


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
		it.push_back(ID);
	  NotifyReceive(roleID,Name,it);
	}
	return;

}


void TeamSvc::ProcessAnswerRequest(Session& session,Packet& packet)
{
	//交易的应答,同样谁的邀请

		UInt32	RetCode = 0;
		DataBuffer	serbuffer(1024);

		UInt32 roleID = packet.RoleID;
		UInt32 ID=0;//应答对象的角色ID
		Byte type=0;
		List<UInt32> it;
		Team team;
		Team team1;
		list<TeamRole>::iterator itor;
		list<TeamRole> li;
		TeamRole r;
		int iRet=0;
		UInt32 ItemID=0;
		Byte flag=0;
			//序列化类
		Serializer s(packet.GetBuffer());
		s>>ID>>type;
		if( s.GetErrorCode()!= 0 )
		{
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
			goto EndOf_Process;
		}
		if(type==1)
		{
				//同意组队
				team1=_mainSvc->GetCoreData()->GetTeamsFromRoleID(roleID);
				team=_mainSvc->GetCoreData()->GetTeamsFromRoleID(ID);
				if(team1.GetLeaderRoleID()==roleID)
				{
						//队长同意了对方的申请
						if(team.GetTeamID()==0)
						{
							iRet=_mainSvc->GetCoreData()->AddTeamRole(team1.GetTeamID(),ID);
							
								flag=1;
								if(iRet==-1)
								{
									//
									RetCode = 202;//你是队长。但是找不到你的队伍
									LOG(LOG_ERROR,__FILE__,__LINE__,"find the team erro!" );
									goto EndOf_Process;
								}
						}
						else
						{//对方已经在队伍中
									RetCode = 203;
									LOG(LOG_ERROR,__FILE__,__LINE__,"find the team erro!" );
									goto EndOf_Process;
						}
					
				}
				else if(team1.GetTeamID()==0)//没有组队
				{
				
						if(team.GetTeamID()==0)//没有队伍
						{
							flag=2;
							_mainSvc->GetCoreData()->CreateTeam(ID,roleID);
						}
						else
						{
								if(team.GetLeaderRoleID()!=ID)//对方不是队长
								{
										RetCode = 204;
										LOG(LOG_ERROR,__FILE__,__LINE__,"The Role iS not the Leader" );
										goto EndOf_Process;
								}
								else
								{
									flag=3;
									iRet=_mainSvc->GetCoreData()->AddTeamRole(team.GetTeamID(),roleID);//对方一定得是队长
										if(iRet==-1)
										{
											//
											RetCode = ERR_SYSTEM_SERERROR;
											LOG(LOG_ERROR,__FILE__,__LINE__,"find the time erro!" );
											goto EndOf_Process;
										}
										
								}//end if  else


								
						}//end if else


						
				}//end 
				

			
		}//end type==1

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
			if(type==1)
			{
				//同意组队
				
					if(flag==1)
					{//队长同意没有组队的加入队伍(自己是队长)
						li=team1.GetMemberRoleID();
						for(itor=li.begin();itor!=li.end();itor++)
						{
							it.push_back(itor->roleId);
						}
						r.roleId=ID;
						r.status=1;//ID自己的状态的设置
						li.push_back(r);
						NotifyTeamChang(ID,1, it);//向所有队员发送S-C,对新加入的成员进行
						NotifyTeamInfo(ID,li);
					}
					if(flag==2)
					{
						r.roleId=ID;
						r.status=2;
						li.push_back(r);
						r.roleId=roleID;
						r.status=1;
						li.push_back(r);
						NotifyTeamInfo(ID,li);
						NotifyTeamInfo(roleID,li);
					//两个都没有组队
						
					}
					if(flag==3)
					{//自身没有队伍，队长要求
						li=team.GetMemberRoleID();
						for(itor=li.begin();itor!=li.end();itor++)
						{
							it.push_back(itor->roleId);
						}
						r.roleId=roleID;
						r.status=1;//ID自己的状态的设置
						li.push_back(r);
						NotifyTeamChang(roleID,1,it);
						NotifyTeamInfo(roleID,li);
					}
			}
			else
			{
				NotifyRefuse(roleID,ID);
				//不同意
			}
		}
		return;
}
void TeamSvc::ProcessChangeCaptain(Session& session,Packet& packet)//提升为队长
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);

	UInt32 roleID = packet.RoleID;
	UInt32 ID=0;//申请对象的角色ID
	string Name;
	List<UInt32> it;
	UInt32 teamID=0;
	int iRet=0;
	Team team;

	list<TeamRole>::iterator itor;
	list<TeamRole> li;
	
		//序列化类
	Serializer s(packet.GetBuffer());
	s>>ID;
	
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}
	if(ID==0||ID==roleID)
	{
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"there are some erro in the ID %d vs ResustID%d",ID,roleID );
			goto EndOf_Process;
	}
	//邀请者判断	
	team=_mainSvc->GetCoreData()->GetTeamsFromRoleID(roleID);
	teamID=team.GetTeamID();
	if(team.GetLeaderRoleID()!=roleID)
	{
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"you are not the leader !" );
			goto EndOf_Process;
	}
	
	
	iRet=_mainSvc->GetCoreData()->ChangtoBeLeader(teamID,ID);
	if(iRet!=0)
	{
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"RoleID is the leader  (%d) or Not find the team ",ID );
			goto EndOf_Process;
	}
	


	

	DEBUG_PRINTF( "C_S ProcessProcessChat sucess!!!!!!!!\n" );	


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
		li=team.GetMemberRoleID();
		for(itor=li.begin();itor!=li.end();itor++)
		{
					it.push_back(itor->roleId);
		}
	  NotifyTeamChang(ID,2,it);//成为队长,还得对其他成员
	  NotifyTeamChang(roleID,1,it);
	}
	return;
}

//踢掉队员
void TeamSvc::ProcessCapt_Member(Session& session,Packet& packet)//踢出队伍
{
		UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);

	UInt32 roleID = packet.RoleID;
	UInt32 ID=0;//申请对象的角色ID
	string Name;
	List<UInt32> it;
	UInt32 teamID=0;
	int iRet=0;
	Team team;

	list<TeamRole>::iterator itor;
	list<TeamRole> li;
	
		//序列化类
	Serializer s(packet.GetBuffer());
	s>>ID;
	
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}
	if(ID==0||ID==roleID)
	{
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"there are some erro in the ID %d vs ResustID%d",ID,roleID );
			goto EndOf_Process;
	}
	//邀请者判断	
	team=_mainSvc->GetCoreData()->GetTeamsFromRoleID(roleID);
	teamID=team.GetTeamID();
	if(team.GetLeaderRoleID()!=roleID)
	{
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"you are not the leader !" );
			goto EndOf_Process;
	}
	iRet=_mainSvc->GetCoreData()->DeleteTeamRole(teamID,ID);
	if(iRet==-1)
	{
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"there are some erro!in itw !" );
			goto EndOf_Process;
	}
	
	
	//踢掉队员
	


	

	DEBUG_PRINTF( "C_S ProcessProcessChat sucess!!!!!!!!\n" );	


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
		li=team.GetMemberRoleID();
		for(itor=li.begin();itor!=li.end();itor++)
		{
					it.push_back(itor->roleId);
		}
	  NotifyTeamChang(ID,5,it);//s-c还得对其他人进行,所有成员,include self
	}
	return;
}

//召唤队员
void TeamSvc::ProcessCallMember(Session& session,Packet& packet)
{
			UInt32	RetCode = 0;
			DataBuffer	serbuffer(1024);

			UInt32 roleID = packet.RoleID;
			UInt32 ID=0;//申请对象的角色ID
			string Name;
			List<UInt32> it;
			UInt32 teamID=0;
			int iRet=0;
			int flag=0;
			Team team;
			list<TeamRole>::iterator itor;
			list<TeamRole> t;
				//序列化类
			Serializer s(packet.GetBuffer());
			s>>ID;
			
			if( s.GetErrorCode()!= 0 )
			{
				RetCode = ERR_SYSTEM_SERERROR;
				LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
				goto EndOf_Process;
			}
			if(ID==0||ID==roleID)
			{
					RetCode = ERR_SYSTEM_SERERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"there are some erro in the ID %d vs ResustID%d",ID,roleID );
					goto EndOf_Process;
			}
			//判断是否是队长
			team=_mainSvc->GetCoreData()->GetTeamsFromRoleID(roleID);
			teamID=team.GetTeamID();
			if(team.GetLeaderRoleID()!=roleID)
			{
					RetCode = ERR_SYSTEM_SERERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"you are not the leader !" );
					goto EndOf_Process;
			}
			//邀请者判断	,判断是否在队伍内部
			t=team.GetMemberRoleID();
			for(itor= t.begin();itor!=t.end();itor++)
			{
				if(itor->roleId==roleID)
				{
					flag=1;
				}
			}
			if(flag==0)
			{
				//不是队伍内成员
					RetCode = ERR_SYSTEM_SERERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"you are not the leader !" );
					goto EndOf_Process;
			}

			

			DEBUG_PRINTF( "C_S ProcessProcessChat sucess!!!!!!!!\n" );	


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
				it.push_back(ID);
			  NotifyCaptionCall(it);//ok
			}
			return;
}
//队员自己请求退出的
void TeamSvc::ProcessMember_OUt(Session& session,Packet& packet)//退出队伍
{
			UInt32	RetCode = 0;
			DataBuffer	serbuffer(1024);

			UInt32 roleID = packet.RoleID;
			string Name;
			List<UInt32> it;
			UInt32 teamID=0;
			int iRet=0;
			int flag=0;
			Team team;
			UInt32 ID=0;

			list<TeamRole>::iterator itor;
			list<TeamRole> li;
				//序列化类
			Serializer s(packet.GetBuffer());
			
			
			if( s.GetErrorCode()!= 0 )
			{
				RetCode = ERR_SYSTEM_SERERROR;
				LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
				goto EndOf_Process;
			}

			team=_mainSvc->GetCoreData()->GetTeamsFromRoleID(roleID);
			teamID=team.GetTeamID();

			li=team.GetMemberRoleID();
			if(teamID==0)
			{
				//没有队伍
					RetCode = ERR_SYSTEM_SERERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"there are some erro in the ID %d vs ResustID%d",ID,roleID );
					goto EndOf_Process;
				
			}
			//判断是否是队长
			if(team.GetLeaderRoleID()==roleID&&li.size()>1)
			{
				
				//ID,为新提的队长
					for(itor=li.begin();itor!=li.end();itor++)
					{
						if(itor->roleId!=roleID)
						{
							ID=itor->roleId;
							break;
						}
					}
					if(ID!=0)
					{
						_mainSvc->GetCoreData()->ChangtoBeLeader(teamID,ID);
					}
					//换个队长，先换个队长，然后去掉队员
			}
			//去掉队员

			iRet=_mainSvc->GetCoreData()->DeleteTeamRole(teamID,roleID);
			
			

			DEBUG_PRINTF( "C_S ProcessProcessChat sucess!!!!!!!!\n" );	


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
				li.clear();
				li=team.GetMemberRoleID();
				for(itor=li.begin();itor!=li.end();itor++)
				{
						if(roleID!=itor->roleId)
							it.push_back(itor->roleId);
				}
				//it.push_back(ID);
				 NotifyTeamChang(roleID,5,it);
			  if(ID!=0)
			  {
			  	NotifyTeamChang(ID,2,it);
			  }
			}
			return;
}
void TeamSvc::ProcessBackOrleave(Session& session,Packet& packet)//归队或者离队
{
			UInt32	RetCode = 0;
			DataBuffer	serbuffer(1024);

			UInt32 roleID = packet.RoleID;
			UInt32 ID=0;//申请对象的角色ID
			string Name;
			List<UInt32> it;
			UInt32 teamID=0;
			int iRet=0;
			int flag=0;
			Team team;
			Byte oPflag=0;
			list<TeamRole>::iterator itor;
			list<TeamRole> li;
				//序列化类
			Serializer s(packet.GetBuffer());
			s>>oPflag;
			
			if( s.GetErrorCode()!= 0 )
			{
				RetCode = ERR_SYSTEM_SERERROR;
				LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
				goto EndOf_Process;
			}

			team=_mainSvc->GetCoreData()->GetTeamsFromRoleID(roleID);
			teamID=team.GetTeamID();
			if(oPflag==1)//归队
			{
				_mainSvc->GetCoreData()->ChangtoFlag(teamID,roleID,1);
			}
			else if(oPflag==2)//离队	
			{
				_mainSvc->GetCoreData()->ChangtoFlag(teamID,roleID,2);
			}
		
			

			DEBUG_PRINTF( "C_S ProcessProcessChat sucess!!!!!!!!\n" );	


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
				li=team.GetMemberRoleID();
				for(itor=li.begin();itor!=li.end();itor++)
				{
							
							if(oPflag!=0)
							{
								it.push_back(itor->roleId);
								LOG(LOG_ERROR,__FILE__,__LINE__,"ID %d     RoleID %d",itor->roleId,itor->status);
							}
									
				}
			  if(oPflag==0)//离线
			  {
			  	NotifyTeamChang(roleID,4,it);
			  }
			  if(oPflag==1)//归队
			  {
			  	NotifyTeamChang(roleID,1,it);
			  }
			  if(oPflag==2)//离队
			  {
			  	NotifyTeamChang(roleID,3,it);
			  }
			}
			return;
}
void TeamSvc::NotifyReceive(UInt32 RoleID,string Name,List<UInt32>& it)
{
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );	
	Packet p(&serbuffer);
	//组包头
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 1401;
	p.UniqID = 123;

	p.PackHeader();


	//写包体
	s<<RoleID<<Name;
	p.UpdatePacketLength();

	if( _mainSvc->Service()->Broadcast( it, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size()[%d]  \n", p.MsgType, it.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
	
}

void TeamSvc::NotifyTeamChang(UInt32 RoleID,Byte changeType,List<UInt32>& it)
{
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );	
	Packet p(&serbuffer);

	//组包头
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 1402;
	p.UniqID = 123;
	p.PackHeader();

	//写包体
	s<<RoleID<<changeType;
	p.UpdatePacketLength();


	if( _mainSvc->Service()->Broadcast( it, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size()[%d]  \n", p.MsgType, it.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
	
}

void TeamSvc::NotifyCaptionCall(List<UInt32>& it)
{
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );	
	Packet p(&serbuffer);

	//组包头
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 1403;
	p.UniqID = 123;

	p.PackHeader();
	//写包体,
	//包体空的
	p.UpdatePacketLength();

	if( _mainSvc->Service()->Broadcast( it, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size()[%d]  \n", p.MsgType, it.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
	
}
 	void TeamSvc::NotifyRefuse(UInt32 RoleID,UInt32 ID)//谁拒绝谁，RoleID是指谁ID是收到的人
	{
			DataBuffer	serbuffer(8196);
			Serializer s( &serbuffer );	
			Packet p(&serbuffer);
			List<UInt32> it;
			//组包头
			serbuffer.Reset();
			p.Direction = DIRECT_S_C_REQ;
			p.SvrType = 1;
			p.SvrSeq = 1;
			p.MsgType = 1404;
			p.UniqID = 123;

			p.PackHeader();
			//写包体,
			s<<RoleID;
			//包体空的
			p.UpdatePacketLength();
			it.push_back(ID);
			if( _mainSvc->Service()->Broadcast( it, &serbuffer))
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
			}

		DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size()[%d]  \n", p.MsgType, it.size() );
			DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
			
		
	}

	
	void TeamSvc::NotifyTeamInfo(UInt32 RoleID, list<TeamRole>& l)
	{
			DataBuffer	serbuffer(8196);
			Serializer s( &serbuffer );	
			Packet p(&serbuffer);
			list<TeamRole>::iterator itor;
			List<TeamRole> li;
			TeamRole r;
			
			List<UInt32> it;
			//组包头
			serbuffer.Reset();
			p.Direction = DIRECT_S_C_REQ;
			p.SvrType = 1;
			p.SvrSeq = 1;
			p.MsgType = 1405;
			p.UniqID = 123;

			p.PackHeader();
			//写包体,
			//包体空的
			for(itor=l.begin();itor!=l.end();itor++)
			{
				r=(*itor);
				li.push_back(r);
			}
			s<<li;
			p.UpdatePacketLength();
			it.push_back(RoleID);
			
			if( _mainSvc->Service()->Broadcast( it, &serbuffer))
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
			}

		DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size()[%d]  \n", p.MsgType, it.size() );
			DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
			
		
	}

