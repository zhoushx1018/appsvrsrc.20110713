#include "ChatSvc.h"
#include "MainSvc.h"
#include "DBOperate.h"
#include "CoreData.h"
#include "Role.h"
#include "../Bag/BagSvc.h"

#define ERR_APP_ROLEHASNOITEM 901

ChatSvc::ChatSvc(void* service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;

}

ChatSvc::~ChatSvc()
{

}
//数据包信息
void ChatSvc::OnProcessPacket(Session& session,Packet& packet)
{
DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 901: //聊天
		ProcessChat(session,packet);
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
void ChatSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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

void ChatSvc::ProcessChat(Session& session,Packet& packet)//msgtype 901 聊天
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	Byte Falg;
	string Name;
	string Message;
	List < UInt32 > lrid;
	UInt32 ID=0,roleID1=0;
	UInt32 MapID;
	UInt32 roleID = packet.RoleID;
	//获取角色信息
	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

		//序列化类
	Serializer s(packet.GetBuffer());
	s>>Falg>>Name>>Message;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	if(pRole->ID() == 0 )
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error" );
		goto EndOf_Process;
	}
	
	if(Falg==1 || Falg == 7)
	{
	//1为世界聊天（全服）
		_mainSvc->GetCoreData()->GetRoleIDs( roleID,lrid );
	}
	if(Falg==2)
	{
	//2为同条线的聊天
		_mainSvc->GetCoreData()->GetRoleIDs( roleID,lrid );
	}
	if(Falg==3)
	{
	//3为场景聊天
		_mainSvc->GetCoreData()->GetMapRoleIDs( pRole->MapID(), roleID, lrid);
	}
	if(Falg==4)
	{
	//4为组队聊天
		if(pRole->TeamID()==0)
		{
			//找不到队伍
			RetCode = ERR_SYSTEM_DATANOTEXISTS;
			LOG(LOG_ERROR,__FILE__,__LINE__,"cant find the team!!!" );
			goto EndOf_Process;
		}
		else
		{
			GetTeamRoleIDs(pRole->TeamID(),roleID,lrid);
		}
	}
	if(Falg==5)
	{
	//5为公会聊天
	}
	if(Falg==6)
	{
		//6为私聊
		ID=_mainSvc->GetCoreData()->FromNameToFindID(roleID,Name);
		if(ID==0)
		{//没有找到，角色不存在，或者不在线
			RetCode = ERR_APP_ROLENOTEXISTS;
		//	LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
			goto EndOf_Process;
		}
		else
		{
			lrid.push_back(ID);
		}
		roleID1=ID;
	}
/*
	if (Falg == 7)
	{
		List <ItemList> items;
		List <ItemCell> itemCells;
		ItemList item;
		item.ItemID = 1304;//喊话筒
		item.num = 1;
		items.push_back(item);
		if (_mainSvc->GetBagSvc()->SelectIfhasItems(roleID, items, itemCells) > 0)
		{
			_mainSvc->GetCoreData()->GetRoleIDs( roleID,lrid );
			if (_mainSvc->GetBagSvc()->DropItems(roleID, items, itemCells) > 0)
			{
				_mainSvc->GetBagSvc()->NotifyBag(roleID, itemCells);
			}
			else
			{
				RetCode = ERR_APP_ROLEHASNOITEM;
				goto EndOf_Process;
			}
		}
		else
		{
			RetCode = ERR_APP_ROLEHASNOITEM;
			LOG(LOG_ERROR,__FILE__,__LINE__,"cant find the item!!!" );
			goto EndOf_Process;
		}
	}
*/	
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
	s<<roleID1;
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
		NotifyChat(Falg,roleID,lrid,Message);
	}
	return;

}

void ChatSvc::GetTeamRoleIDs(UInt32 teamID,UInt32 roleID,list<UInt32>& itor)//不包括自己
{
		Team roleteam;
		list<TeamRole> teamRole;
		list<TeamRole>::iterator itor1;
		roleteam=_mainSvc->GetCoreData()->GetTeams(teamID);
		teamRole=roleteam.GetMemberRoleID();

		for(itor1=teamRole.begin();itor1!=teamRole.end();itor1++)
		{
			if(itor1->roleId!=roleID)
			{
				itor.push_back(itor1->roleId);
			}
		}
		
}

void ChatSvc::NotifyChat(Byte Falg,UInt32 roleID,List<UInt32> lrid,string& Message)
{
	DataBuffer	serbuffer(1024);
	string Name;
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 901;
	p.UniqID = 212;
	p.PackHeader();
	Name=_mainSvc->GetCoreData()->ProcessGetRolePtr(roleID)->Name();
	s<<Falg<<Name<<roleID<<Message;
	p.UpdatePacketLength();
	if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
	

}


