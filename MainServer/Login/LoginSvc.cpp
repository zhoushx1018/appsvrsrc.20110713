#include "LoginSvc.h"
#include "MainSvc.h"
#include "ArchvTask.h"
#include "DBOperate.h"
#include "ArchvTask.h"
#include "CoreData.h"
#include "ArchvRole.h"
#include "SSClientManager.h"
#include "ArchvMap.h"
#include "Role.h"
#include "./OffLineUpdate/OffLineUpdateSvc.h"


LoginSvc::LoginSvc(void* service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;
}

LoginSvc::~LoginSvc()
{
}

void LoginSvc::OnProcessPacket(Session& session,Packet& packet)
{
	
DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 101: //角色登陆		
			ProcessRoleLogin(session,packet);
			break;

		case 104:	//角色登出
			ProcessRoleLogout(session,packet);
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
void LoginSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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



//调整X轴坐标
//	场景坐标-->pk坐标
//@param	X	场景X坐标
//@param  originX 场景pk原点 X坐标
//@return  X坐标
int LoginSvc::AdjustCoordinateX( UInt32 X, UInt32 originX )
{
	int iTmp = X - originX;
	
	if( iTmp <= 0 )
		return 0;
	else if( iTmp > PKSCREEN_XLENGTH )
		return PKSCREEN_XLENGTH;
	else
		return iTmp;
}



//调整Y轴坐标
//	场景坐标-->pk坐标
//@param	Y	场景Y坐标
//@param  originX 场景pk原点 Y坐标
//@return  Y坐标
int LoginSvc::AdjustCoordinateY( UInt32 Y, UInt32 originY )
{
	int iTmp = Y - originY;
	
	if( iTmp <= 0 )
		return 0;
	else if( iTmp > PKSCREEN_YLENGTH )
		return PKSCREEN_YLENGTH;
	else
		return iTmp;
}






void LoginSvc::ProcessRoleLogin(Session& session,Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	
	UInt32	roleID = 0;
	char szPasswd[65];
	UInt64 clientConnID = 0;
	ArchvRoute route;
	//Byte IsAdult=0;
	//Byte Flag=0;
	//UInt32 _lastlogintime=0,_toptime=0;
	//初始化
	memset( szPasswd, 0, sizeof(szPasswd) );
	
	packet.GetBuffer()->Read( &roleID, 4 );
	packet.GetBuffer()->Read( &szPasswd, 40 );
	packet.GetBuffer()->Read( &clientConnID, 8 );

	//序列化类
	Serializer s(packet.GetBuffer());

	//先退出地图,忽略返回值
	//	用以解决短时间重登陆(3秒内), 服务器报"重复登陆地图错误"
	_mainSvc->GetCoreData()->ProcessRoleLogout(roleID);
	
	//业务处理
	iRet = _mainSvc->GetCoreData()->ProcessRoleLogin( roleID, szPasswd, clientConnID );
	if(iRet)
	{
		RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessRoleLogin ++++++ error!!roleID[%d] ", roleID);
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
	//登陆交易比较特殊，不论成功与否，都需要返回包体字段
	//因为包体第二个字段是网关查找客户端连接的关键字
	serbuffer.Write( &roleID, 4 );
	serbuffer.Write( &clientConnID, 8 );

	p.UpdatePacketLength();
	
	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}
	if(RetCode==0)
	{
		//S-C协议
		//NotifyCtAdultLogin(roleID,IsAdult,Flag);
	}
  DEBUG_PRINTF1( "C_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	if(0 == RetCode)
	{
	   //好友上线通知
		 _mainSvc->GetFriendSvc()->OnFriendOnLine(roleID);

	   //bufflist通知，比如经验符文
		// _mainSvc->GetBagSvc()->NotifyBuffUpdate(roleID);
	}
	
}

Byte LoginSvc::GetIfIsAdult(UInt32 roleID,UInt32& TopTime,UInt32& LastloginTime)
{
	char szSql[256];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	Byte is=0;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	sprintf( szSql, "select IsAdult from Account where AccountID=(select AccountID from Role where RoleID=%d)",roleID );
	iRet=dbo.QuerySQL(szSql);
	if( 0 == iRet )
	{
		is=dbo.GetIntField(0);
	}
	else
	{
			LOG(LOG_ERROR,__FILE__,__LINE__,"GetAdult erro" );
	}
		sprintf( szSql, "select LastloginTime,TopTime from Role where RoleID=%d;",roleID );
		iRet=dbo.QuerySQL(szSql);
		if( 0 == iRet )
		{
			LastloginTime=dbo.GetIntField(0);
			TopTime=dbo.GetIntField(1);
		}
		else
		{
				LOG(LOG_ERROR,__FILE__,__LINE__,"GetTopTime erro" );
		}
	
	return is;
}



void LoginSvc::ProcessRoleLogout(Session& session,Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	UInt32 roleID = packet.RoleID;
	
	//序列化类
	Serializer s(packet.GetBuffer());
//	s>>roleID;
	if( s.GetErrorCode()!= 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		return;
	}

	//业务处理
	iRet = _mainSvc->GetCoreData()->ProcessRoleLogout(roleID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessRoleLogout----- error!!roleID[%d] ", roleID);
		goto EndOf_Process;
	}

	

EndOf_Process:

	//广播放到地图中处理	
	;;
	
}

//s-c
	void LoginSvc::NotifyCtAdultLogin(UInt32 roleID,Byte IsAdult,Byte Flag)
	{	
			DataBuffer	serbuffer(1024);
			Serializer s( &serbuffer );
			Packet p(&serbuffer);
			serbuffer.Reset();
			p.Direction = DIRECT_S_C_REQ;
			p.SvrType = 1;
			p.SvrSeq = 1;
			p.MsgType = 9903;
			p.UniqID = 218;
			List<UInt32> it;
			p.PackHeader();
			p.UpdatePacketLength();
			it.push_back(roleID);
			if(_mainSvc->Service()->Broadcast(it, &serbuffer))
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
			}
			DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, it.size() );
			DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
	
	
	}


