#include "MainSvc.h"
#include "DBOperate.h"
#include "FriendSvc.h"
#include "Role.h"
#include "ArchvRole.h"
#include "CoreData.h"
#include "ArchvFriend.h"
#include "../Bag/BagSvc.h"

FriendSvc::FriendSvc(void *service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;

}

FriendSvc::~FriendSvc()
{
	//
}

//数据包信息
void FriendSvc::OnProcessPacket(Session& session,Packet& packet)
{
	DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 1201: //[MsgType:1201]查询好友
			ProcessGetFriend(session,packet);
			break;

		case 1202: //[MsgType:1202]增加好友
			ProcessAddFriend(session,packet);
			break;

		case 1203: //[MsgType:1203]删除好友
			ProcessDeleteFriend(session,packet);
			break;

		case 1204: //[MsgType:1204]修改好友类型
			ProcessReviseFriendType(session,packet);
			break;

		case 1205: //[MsgType:1205]同意增加好友/改为好友的请求
			ProcessAgreeFriend(session,packet);
			break;

		case 1206: //[MsgType:1206]好友私聊
      ProcessFriendPrivateChat(session,packet);
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
void FriendSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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

//[MsgType:1201]查询好友
void FriendSvc::ProcessGetFriend(Session& session, Packet& packet)
{
	char szSql[1024];
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(12288);
	UInt32 friendRoleID = 0,roleID= packet.RoleID;
	int iRet;
	Connection con;
	DBOperate dbo;

	ArchvFriendAddPro af;
	List<ArchvFriendAddPro> lf;

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

	sprintf( szSql, "select FriendName, FriendType, FriendRoleID ,ProID \
										from Friend \
										where RoleID = %d;", roleID );
	iRet=dbo.QuerySQL(szSql);
	if(iRet<0)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
	if(iRet == 0)
	{
	    while(dbo.HasRowData())
		{
			af.friendName= dbo.GetStringField(0);
			af.friendType = dbo.GetIntField(1);
			af.friendRoleID = dbo.GetIntField(2);
			friendRoleID = af.friendRoleID;
			af.ProID=dbo.GetIntField(3);

			iRet = _mainSvc->GetCoreData()->GetRoleMapID(friendRoleID);
			if(iRet==0)
			{
				af.IsOnline = 0;
			}
			else
			{
			  af.IsOnline = 1;
			}



			lf.push_back(af);
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

	LOG(LOG_ERROR,__FILE__,__LINE__,"RetCode[%d]",RetCode);

	s<<RetCode;
	if( 0 == RetCode)
	{
		//RetCode 为0 才会返回包体剩下内容
		s<<lf;
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

//[MsgType:1202]好友添加
void FriendSvc::ProcessAddFriend(Session& session, Packet& packet)
{
	char szSql[1024];
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	UInt32 friendRoleID,roleID= packet.RoleID;
	int iRet,tmpRet;
	Byte ft;
	Connection con;
	DBOperate dbo;
	string friendName,roleName;
	UInt32 friendCnt = 0;// 普通用户好友上线不能超过40
	Byte friendType = 0,fType = 0;
	Byte realFriend = 0;
	ArchvFriendAddPro frstFriendItem;
    ArchvFriendAddPro secFriendItem;
	Byte isVip = 0; //是否是VIP
	UInt32 maxFriendNum = 0;
	//序列化类
	Serializer s(packet.GetBuffer());
	s>>friendName>>friendType;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//LOG(LOG_DEBUG,__FILE__,__LINE__,"friendName[%s]---friendType[%d]",friendName.c_str(),friendType);

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

  //校验好友类型
  if(friendType != 1 && friendType != 2 && friendType != 3)
  {
    RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"friendType[%d] is not existed !",friendType);
		goto EndOf_Process;
  }

  //是否是 vip 用户
  //获取自己的角色名
	sprintf( szSql, "select RoleName,ProID,IsVIP from Role\
										where RoleID = %d;",roleID);
	iRet=dbo.QuerySQL(szSql);
	if(iRet != 0)
	{
		RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_ERROR,__FILE__,__LINE__,"friendRole is not existed !");
		goto EndOf_Process;
	}

	roleName = dbo.GetStringField(0);
	frstFriendItem.ProID=dbo.GetIntField(1);
	isVip = dbo.GetIntField(2);

    if(isVip >= 1 && isVip <= 6)
    {
		maxFriendNum = MAX_FRIEND_NUM + isVip * 10;
	}
	else
	{
		maxFriendNum = MAX_FRIEND_NUM;
	}


  //检查好友上限，不能超过40
  sprintf(szSql,"select count(*) from Friend\
  								where RoleID = %d and FriendType = %d;",roleID,friendType);
  iRet =dbo.QuerySQL(szSql);
  if(iRet < 0)
  {
        RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error !");
		goto EndOf_Process;
  }

  friendCnt = dbo.GetIntField(0);

  if(friendCnt > maxFriendNum)
  {
        RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"Role[%d] FriendType[%d] friend more than [%d] !",roleID,friendType,maxFriendNum);
		goto EndOf_Process;
  }



	//查询好友角色是否存在
	sprintf( szSql, "select RoleID,ProID from Role\
										where RoleName = '%s';",friendName.c_str());
	iRet=dbo.QuerySQL(szSql);
	if(iRet == 1)
	{
		RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_DEBUG,__FILE__,__LINE__,"friendRole[%s] is not existed !",friendName.c_str());
		goto EndOf_Process;
	}
	else if(iRet < 0)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"DB Operate failed !");
		goto EndOf_Process;
	}

	friendRoleID = dbo.GetIntField(0);
	secFriendItem.ProID=dbo.GetIntField(1);

	//好友是否是自己
	if(roleID == friendRoleID)
	{
	  RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"friendRoleID[%d] equal to roleID[%d] ",friendRoleID,roleID);
		goto EndOf_Process;
	}

	//查询是否在线
	iRet = _mainSvc->GetCoreData()->GetRoleMapID(friendRoleID);
	if(iRet==0)
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_DEBUG,__FILE__,__LINE__,"Friend[%d] is not Online !",friendRoleID);
		goto EndOf_Process;
	}

	//增加黑名单和仇人
	if(friendType == 2 || friendType == 3)
	{
	  sprintf(szSql,"select FriendType from Friend\
				where RoleID = %d and FriendRoleID = %d",roleID,friendRoleID);
		iRet = dbo.QuerySQL(szSql);
		if(iRet < 0)
		{
		  RetCode = ERR_SYSTEM_DBERROR;
		  LOG(LOG_DEBUG,__FILE__,__LINE__,"Excute SQL not found ! szSql[%s]",szSql);
		  goto EndOf_Process;
		}
		if(iRet == 0)
		{
			 fType = dbo.GetIntField(0);
		   if(friendType == fType)
		   {
		     RetCode = ERR_APP_DATA;
		  	 LOG(LOG_DEBUG,__FILE__,__LINE__,"friendType[%d] is itsself !",friendType);
		  	 goto EndOf_Process;
		   }

		   //更改friendType
		   sprintf(szSql,"update Friend set FriendType = %d \
		                  where RoleID = %d and FriendRoleID = %d;",\
		                  friendType,roleID,friendRoleID);
		   tmpRet = dbo.ExceSQL(szSql);
		   if(tmpRet != 0)
		   {
		     RetCode = ERR_APP_DATA;
		  	 LOG(LOG_DEBUG,__FILE__,__LINE__,"Excute SQL not found ! szSql[%s]",szSql);
		  	 goto EndOf_Process;
		   }

		   sprintf(szSql,"update Friend set FriendType = %d \
		                  where RoleID = %d and FriendRoleID = %d;",\
		                  friendType,friendRoleID,roleID);
		   tmpRet = dbo.ExceSQL(szSql);
		   if(tmpRet != 0)
		   {
		     RetCode = ERR_APP_DATA;
		  	 LOG(LOG_DEBUG,__FILE__,__LINE__,"Excute SQL not found ! szSql[%s]",szSql);
		  	 goto EndOf_Process;
		   }

		}
		if(iRet == 1)
		{
		  sprintf(szSql,"insert into Friend(\
		  																	RoleID,\
		  																	FriendRoleID,\
		  																	FriendName,\
		  																	FriendType,\
		  																	ProID \
		  																	)\
		  							              value(%d,%d,'%s',%d,%d);",\
		  							              roleID,friendRoleID,\
		  							              friendName.c_str(),friendType,frstFriendItem.ProID);
		   iRet = dbo.ExceSQL(szSql);
		   if(iRet != 0)
		   {
		     RetCode = ERR_SYSTEM_DBERROR;
		  	 LOG(LOG_DEBUG,__FILE__,__LINE__,"Excute SQL not found ! szSql[%s]",szSql);
		  	 goto EndOf_Process;
		   }


		   sprintf(szSql,"insert into Friend(\
		  																	RoleID,\
		  																	FriendRoleID,\
		  																	FriendName,\
		  																	FriendType, \
		  																	ProID \
		  																	)\
		  							              value(%d,%d,'%s',%d);",\
		  							              friendRoleID,roleID,\
		  							              roleName.c_str(),friendType,secFriendItem.ProID);
		   iRet = dbo.ExceSQL(szSql);
		   if(iRet != 0)
		   {
		     RetCode = ERR_SYSTEM_DBERROR;
		  	 LOG(LOG_DEBUG,__FILE__,__LINE__,"Excute SQL not found ! szSql[%s]",szSql);
		  	 goto EndOf_Process;
		   }
		}

		frstFriendItem.friendName = roleName;
		frstFriendItem.friendRoleID = roleID;
		frstFriendItem.friendType = friendType;
		frstFriendItem.IsOnline = 1;

		secFriendItem.friendName = friendName;
		secFriendItem.friendRoleID = friendRoleID;
		secFriendItem.friendType = friendType;
		secFriendItem.IsOnline = 1;

		/*LOG(LOG_DEBUG,__FILE__,__LINE__,"friendName[%s]",roleName.c_str());
    LOG(LOG_DEBUG,__FILE__,__LINE__,"friendRoleID[%d]",roleName.c_str());
    LOG(LOG_DEBUG,__FILE__,__LINE__,"friendType[%d]",roleName.c_str());
		LOG(LOG_DEBUG,__FILE__,__LINE__,"IsOnline[%d]",frstFriendItem.IsOnline);

    LOG(LOG_DEBUG,__FILE__,__LINE__,"===============");
		LOG(LOG_DEBUG,__FILE__,__LINE__,"friendName[%s]",friendName.c_str());
    LOG(LOG_DEBUG,__FILE__,__LINE__,"friendRoleID[%d]",friendRoleID);
    LOG(LOG_DEBUG,__FILE__,__LINE__,"friendType[%d]",friendType);
		LOG(LOG_DEBUG,__FILE__,__LINE__,"IsOnline[%d]",secFriendItem.IsOnline);*/

	}
	if(friendType == 1)
	{
	  sprintf(szSql,"select FriendRoleID, FriendType from Friend\
				where RoleID = %d and FriendRoleID = %d",roleID,friendRoleID);
		iRet = dbo.QuerySQL(szSql);
		if(iRet < 0)
		{
		  RetCode = ERR_SYSTEM_DBERROR;
		  LOG(LOG_DEBUG,__FILE__,__LINE__,"Excute SQL not found ! szSql[%s]",szSql);
		  goto EndOf_Process;
		}
		if(iRet == 0)
		{
		  fType = dbo.GetIntField(1);
		  if(fType == friendType)
		  {
		    RetCode = FRIEND_TYPE_NON_CHANGE;
		  	LOG(LOG_DEBUG,__FILE__,__LINE__,"friendType[%d] is itsself !",friendType);
		  	goto EndOf_Process;
		  }
		}

		realFriend = 1;
	}

EndOf_Process:
	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();
	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();

	LOG(LOG_ERROR,__FILE__,__LINE__,"RetCode[%d]",RetCode);
	s<<RetCode;
	if( 0 == RetCode )
	{
		//RetCode 为0 才会返回包体剩下内容
	}
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	//添加仇人、黑名单
	if(0 == RetCode && 0 == realFriend)
	{
	  //LOG(LOG_DEBUG,__FILE__,__LINE__,"========= Enemy BlackList Notify !");
		NotifyAddFriendSuccessd(roleID, secFriendItem);
		NotifyAddFriendSuccessd(friendRoleID, frstFriendItem);
	}
	if(0 == RetCode && 1 == realFriend)
	{//请求加为好友请求
	  //LOG(LOG_DEBUG,__FILE__,__LINE__,"========= Add Friend  Request Notify !");
	  NotifyFriendRole(friendRoleID, roleName);
	}
}

//[MsgType:1203]删除好友
void FriendSvc::ProcessDeleteFriend(Session& session, Packet& packet)
{
	char szSql[1024];
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	UInt32 friendRoleID = 0,roleID= packet.RoleID;
	int iRet;
	string friendName;
	UInt32 fndFriendType = 0;
	Connection con;
	DBOperate dbo;

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>friendRoleID;

	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//查询好友表
	sprintf( szSql, "select FriendType,FriendName from Friend \
	                 where RoleID=%d and FriendRoleID=%d;", roleID,friendRoleID);
	iRet=dbo.QuerySQL(szSql);
	if(iRet < 0)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
	else if(iRet == 1)
	{
		RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_DEBUG,__FILE__,__LINE__,"RoleID[%d]--FriendRoleID[%d] not exist !",roleID,friendRoleID);
		goto EndOf_Process;
	}

	fndFriendType = dbo.GetIntField(0);
  //friendName = dbo.GetIntField(1);

	//两边都删除好友
	sprintf( szSql, "delete from Friend \
	                where RoleID=%d and FriendRoleID= %d;", roleID,friendRoleID);
	iRet = dbo.ExceSQL(szSql);
	if(iRet != 0)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}

	sprintf(szSql, "delete from Friend \
	               where RoleID=%d and FriendRoleID=%d;", friendRoleID,roleID);

	iRet=dbo.ExceSQL(szSql);
	if(iRet != 0)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
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

	LOG(LOG_ERROR,__FILE__,__LINE__,"RetCode[%d]", RetCode);
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

  if(0 == RetCode)
  {//删除好友
    LOG(LOG_DEBUG,__FILE__,__LINE__,"=== NotifyDeleteFriend ==");
    NotifyDeleteFriend(roleID,friendRoleID,fndFriendType);
    NotifyDeleteFriend(friendRoleID,roleID,fndFriendType);
  }
}

//[MsgType:1204]修改好友类型
void FriendSvc::ProcessReviseFriendType(Session& session, Packet& packet)
{
	char szSql[1024];
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	UInt32 roleID= packet.RoleID;
	int iRet;
	Byte friendType = 0, ft = 0,tmpft = 0,isVip = 0;
	UInt32 friendRoleID = 0,friendCnt = 0;
	string friendRoleName,roleName;
	Byte addFriend = 0;           //是否修改好友类型为好友

	ArchvFriendAddPro frstFriendItem;
	ArchvFriendAddPro secFriendItem;

	Connection con;
	DBOperate dbo;

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>friendRoleID>>friendType;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	/*LOG(LOG_DEBUG,__FILE__,__LINE__,"friendRoleID[%d]",friendRoleID);
	LOG(LOG_DEBUG,__FILE__,__LINE__,"friendType[%d]",friendType);
	LOG(LOG_DEBUG,__FILE__,__LINE__,"roleID[%d]",roleID);*/

	//好友是否是自己
	if(friendRoleID == roleID)
	{
	  RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"friendRoleID[%d] equal to roleID[%d] ",friendRoleID,roleID);
		goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//friendType是否存在
	if(friendType != 1 && friendType != 2 && friendType != 3)
  {
    RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"friendType[%d] is not exist !",friendType);
		goto EndOf_Process;
	}

	//是否是VIP
	sprintf(szSql,"select IsVIP from Role where RoleID = %d;",roleID);
	iRet = dbo.QuerySQL(szSql);
	if(iRet == 0)
	{
		isVip = dbo.GetIntField(0);
	}

	// 角色friendType是否达上限
	sprintf(szSql,"select count(*) from Friend\
								where RoleID = %d and FriendType =%d;",roleID,friendType);
	iRet = dbo.QuerySQL(szSql);
	if(iRet != 0)
	{
	  RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"Excute SQL not found ! szSql[%s]",szSql);
		goto EndOf_Process;
	}

	friendCnt = dbo.GetIntField(0);
	if(friendCnt > (MAX_FRIEND_NUM + isVip * 10))
	{
	  RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"Role[%d] friendType[%d] is more than 300 !" , roleID,friendType);
		goto EndOf_Process;
	}

	sprintf( szSql, "select FriendName,FriendType,ProID from Friend \
									where RoleID=%d and FriendRoleID= %d;",\
									roleID,friendRoleID);

	iRet=dbo.QuerySQL(szSql);
  if(iRet < 0)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or	errro! ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
	if(iRet == 1)
	{
	  RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_ERROR,__FILE__,__LINE__,"DB no record ! RoleID[%d]--FriendRoleID[%d] " ,roleID,friendRoleID);
		goto EndOf_Process;
	}

    friendRoleName = dbo.GetStringField(0);
	ft = dbo.GetIntField(1);
	frstFriendItem.ProID=dbo.GetIntField(2);

	//获取roleName和friendType
	sprintf( szSql, "select FriendName,FriendType,ProID from Friend \
									where RoleId=%d and FriendRoleID= %d;",\
									friendRoleID,roleID);

	iRet=dbo.QuerySQL(szSql);
    if(iRet < 0)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or	errro! ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
	if(iRet == 1)
	{
	  RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or	errro! ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}

  roleName = dbo.GetStringField(0);
  tmpft = dbo.GetIntField(1);
  secFriendItem.ProID=dbo.GetIntField(2);
	//角色表中互为好友的好友类型是否相同
  if(tmpft != ft)
  {
    RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"role[%d] and role[%d] FriendType not the same !",roleID,friendRoleID);
		goto EndOf_Process;
  }

	//不需要移动
	if (ft == friendType)
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"the friendtype[%d] is itself!",friendType);
		goto EndOf_Process;
	}

	if(friendType == 1)
	{
	  //修改好友类型为好友
	  addFriend = 1;
	}
	else
	{
		sprintf( szSql, "update Friend set FriendType=%d \
		                where RoleID=%d and FriendRoleID=%d;",\
		                friendType,roleID,friendRoleID);
		iRet=dbo.ExceSQL(szSql);
		if(iRet != 0)
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or	errro! ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}

		sprintf( szSql, "update Friend set FriendType=%d \
		                where RoleID=%d and FriendRoleID=%d;",\
		                friendType,friendRoleID,roleID);
		iRet=dbo.ExceSQL(szSql);
		if(iRet != 0)
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or	errro! ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}

		frstFriendItem.friendName = roleName;
		frstFriendItem.friendRoleID = roleID;
		frstFriendItem.friendType = friendType;
		frstFriendItem.IsOnline = 1;

		secFriendItem.friendName = friendRoleName;
		secFriendItem.friendRoleID = friendRoleID;
		secFriendItem.friendType = friendType;
		iRet = _mainSvc->GetCoreData()->FindRoleMapID(friendRoleID);
		if(iRet == 0)
		{
			secFriendItem.IsOnline = 0;
		}
		else
		{
			secFriendItem.IsOnline = 1;
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
	LOG(LOG_DEBUG,__FILE__,__LINE__,"RetCode[%d]",RetCode);
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

	if(0 == RetCode && 1 == addFriend)
	{//请求增加好友
	 //LOG(LOG_DEBUG,__FILE__,__LINE__,"AlterFriendType : request add friend !");
	  NotifyFriendRole(friendRoleID, roleName);
	}

	if(0 == RetCode && 0 == addFriend)
	{//修改好友类型，仇人、黑名单
	  //LOG(LOG_DEBUG,__FILE__,__LINE__,"AlterFriendType : BlackList !");
	  NotifyAlterFriendType(friendRoleID, frstFriendItem);
	  NotifyAlterFriendType(roleID, secFriendItem);
	}

}

//[MsgType:1205]同意增加好友/改为好友的请求
void FriendSvc::ProcessAgreeFriend(Session& session, Packet& packet)
{
	char szSql[1024];
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	UInt32 friendRoleID = 0, roleID= packet.RoleID;
	UInt32 friendCnt = 0;
	int iRet,tmpRet;
	Byte friendTpye = 0, fType = 1;
	string friendRoleName,roleName;
	ArchvFriendAddPro agrRoleItem,reqRoleItem;

	Byte alterFriendType = 0; // 如果Friend中记录已存在，则修改好友类型

	Connection con;
	DBOperate dbo;

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>friendRoleName;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

 // LOG(LOG_DEBUG,__FILE__,__LINE__,"friendRoleName[%s]",friendRoleName.c_str());
	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//查询角色表是否存在
   sprintf(szSql ,"select RoleID,ProID from Role \
             where RoleName = '%s';", friendRoleName.c_str());

   iRet = dbo.QuerySQL(szSql);
   if(iRet < 0)
   {
      RetCode = ERR_SYSTEM_DBERROR;
		  LOG(LOG_ERROR,__FILE__,__LINE__,"friendRoleID is not exists !" );
		  goto EndOf_Process;
   }
   else if(iRet == 1)
   {
   		RetCode = ERR_SYSTEM_DBNORECORD;
		  LOG(LOG_DEBUG,__FILE__,__LINE__,"friendRole[%d] is not exists !",friendRoleName.c_str());
		  goto EndOf_Process;
   }

   friendRoleID = dbo.GetIntField(0);
		reqRoleItem.ProID=dbo.GetIntField(1);
   //查询自身roleName
   sprintf(szSql ,"select RoleName,ProID from Role \
             where RoleID = %d;", roleID);

   iRet = dbo.QuerySQL(szSql);
   if(iRet < 0)
   {
      RetCode = ERR_SYSTEM_DBERROR;
		  LOG(LOG_ERROR,__FILE__,__LINE__,"Excute SQL not found ! szSql[%s]",szSql);
		  goto EndOf_Process;
   }
   else if(iRet == 1)
   {
   		RetCode = ERR_SYSTEM_DBNORECORD;
		  LOG(LOG_DEBUG,__FILE__,__LINE__,"friendRoleID[%d] is not exists !",roleID);
		  goto EndOf_Process;
   }

   roleName = dbo.GetStringField(0);
	 agrRoleItem.ProID=dbo.GetIntField(1);

    //检查好友上限，不能超过300
  sprintf(szSql,"select count(*) from Friend\
  								where RoleID = %d and FriendType =1;",roleID);
  iRet =dbo.QuerySQL(szSql);
  if(iRet < 0)
  {
    RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error !");
		goto EndOf_Process;
  }
  friendCnt = dbo.GetIntField(0);

  if(friendCnt > 300)
  {
    RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"Role[%d] friend more than 300 !",roleID);
		goto EndOf_Process;
  }

  //好友是否在线
   if(0 == _mainSvc->GetCoreData()->ProcessGetRolePtr(friendRoleID)->ID())
   {
      RetCode = ERR_APP_DATA;
		  LOG(LOG_DEBUG,__FILE__,__LINE__,"friendRoleID[%d] is not online !",roleID);
		  goto EndOf_Process;
   }

   //查询好友表中角色是否存在
   sprintf(szSql, "select RoleID,FriendType from Friend \
                  where RoleID = %d and FriendRoleID = %d;",\
                  roleID, friendRoleID);

   iRet = dbo.QuerySQL(szSql);
   if(1 == iRet)  //记录不存在，两边添加记录
   {
   		sprintf(szSql, "insert Friend(RoleID,FriendRoleID,FriendName,FriendType,ProID) \
   		               value(%d,%d,'%s',%d,%d);",\
   		               roleID, friendRoleID, friendRoleName.c_str(),fType,agrRoleItem.ProID);

   		tmpRet = dbo.ExceSQL(szSql);
   		if(0 != tmpRet)
   		{
   		  RetCode = ERR_SYSTEM_DBNORECORD;
		  	LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error !" );
		  	goto EndOf_Process;
   		}

   		sprintf(szSql, "insert Friend(RoleID,FriendRoleID,FriendName,FriendType,ProID) \
   		               value(%d,%d,'%s',%d,%d);",\
   		               friendRoleID,roleID, roleName.c_str(),fType,reqRoleItem.ProID);

   		tmpRet = dbo.ExceSQL(szSql);
   		if(0 != tmpRet)
   		{
   		  RetCode = ERR_SYSTEM_DBNORECORD;
		  	LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error !" );
		  	goto EndOf_Process;
   		}

   	 agrRoleItem.friendName = roleName;
	   agrRoleItem.friendRoleID = roleID;
	   agrRoleItem.friendType = 1;
	   agrRoleItem.IsOnline = 1;

	   reqRoleItem.friendName = friendRoleName;
	   reqRoleItem.friendRoleID = friendRoleID;
	   reqRoleItem.friendType = 1;
	   reqRoleItem.IsOnline = 1;

   }
   else if(0 == iRet)   //如果记录存在，则修改FriendType
   {
   		friendTpye = dbo.GetIntField(1);
   		if(1 == friendTpye)
   		{
   		    RetCode = ERR_APP_OP;
		   		LOG(LOG_ERROR,__FILE__,__LINE__,"FriendType[%d] is itsself ! " , friendTpye);
		   		goto EndOf_Process;
   		}

			sprintf(szSql, "update Friend set FriendType = %d \
               where RoleID = %d and FriendRoleID = %d;", fType,roleID, friendRoleID);

      tmpRet = dbo.ExceSQL(szSql);
      if(tmpRet != 0)
      {
	 			RetCode = ERR_SYSTEM_DBERROR;
   			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
   			goto EndOf_Process;
 		  }

 		  sprintf(szSql, "update Friend set FriendType = %d \
               where RoleID = %d and FriendRoleID = %d;", fType,friendRoleID,roleID);

      tmpRet = dbo.ExceSQL(szSql);
      if(tmpRet != 0)
      {
	 			RetCode = ERR_SYSTEM_DBERROR;
   			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
   			goto EndOf_Process;
 		  }

		 alterFriendType = 1;

	 	 agrRoleItem.friendName = roleName;
	   agrRoleItem.friendRoleID = roleID;
	   agrRoleItem.friendType = 1;
	   agrRoleItem.IsOnline = 1;

	   reqRoleItem.friendName = friendRoleName;
	   reqRoleItem.friendRoleID = friendRoleID;
	   reqRoleItem.friendType = 1;
	   reqRoleItem.IsOnline = 1;

   }
   else
   {
   		RetCode = ERR_SYSTEM_DBERROR;
		  LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error ! ");
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

	LOG(LOG_ERROR,__FILE__,__LINE__,"RetCode[%d]",RetCode);
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

	if(0 == RetCode && 0 == alterFriendType)
	{
	   //Friend表中记录不存在，则发送添加好友成功通知(双方)
	  // LOG(LOG_DEBUG,__FILE__,__LINE__,"====AgreeFriend: NotifyAddFriendSuccessd !");
	   NotifyAddFriendSuccessd(friendRoleID,agrRoleItem);
	   NotifyAddFriendSuccessd(roleID, reqRoleItem);
	}
	if(0 == RetCode && 1 == alterFriendType)
	{//Friend表中存在记录，则发送修改好友类型通知(双方)
	  // LOG(LOG_DEBUG,__FILE__,__LINE__,"====AgreeFriend: Already hava record !");
	   NotifyAlterFriendType(roleID, reqRoleItem);
	   NotifyAlterFriendType(friendRoleID, agrRoleItem);
	}

}

//[MsgType:1206]好友私聊
void FriendSvc::ProcessFriendPrivateChat(Session& session, Packet& packet)
{
 	char szSql[1024];
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	UInt32 roleID= packet.RoleID;
	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	int iRet;
	UInt32 friendRoleID = 0;
	Byte friendType = 0;
	string chatContent;
	string Name;
	string FriendName;

	Connection con;
	DBOperate dbo;
	//序列化类
	Serializer s(packet.GetBuffer());
	s>>friendRoleID>>FriendName>>chatContent;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//查看自己信息，以及自己是否在线
	if(0 == pRole->ID())
	{
	    RetCode = ERR_APP_DATA;
		  LOG(LOG_ERROR,__FILE__,__LINE__,"role[%d] is not online !",friendRoleID);
		  goto EndOf_Process;
	}
	Name=pRole->Name();

	if(friendRoleID==0)
	{
		friendRoleID=_mainSvc->GetCoreData()->FromNameToFindID(roleID,FriendName);
	}
	//查看对方是否在信息
	else
	{
		iRet=_mainSvc->GetCoreData()->GetRoleMapID(friendRoleID);
		if(iRet==0)
		{
				RetCode = ERR_APP_DATA;
		  	LOG(LOG_ERROR,__FILE__,__LINE__,"friend[%d] is not online !",friendRoleID);
		  	goto EndOf_Process;
		}
	}


	if(friendRoleID<=0)
	{
			RetCode = ERR_APP_DATA;
		  LOG(LOG_ERROR,__FILE__,__LINE__,"friend[%d] is not online !",friendRoleID);
		  goto EndOf_Process;
	}

	//好友是否在黑名单中，在黑名单中不能聊天
	/*
  sprintf(szSql,"select FriendType from Friend\
  							where RoleID=%d and FriendRoleID=%d;",roleID,friendRoleID);
  iRet = dbo.QuerySQL(szSql);
  if(iRet < 0)
  {
      RetCode = ERR_SYSTEM_DBERROR;
		  LOG(LOG_ERROR,__FILE__,__LINE__,"role[%d]",friendRoleID);
		  goto EndOf_Process;
  }
  if(iRet == 1)
  {
      RetCode = ERR_SYSTEM_DBERROR;
		  LOG(LOG_ERROR,__FILE__,__LINE__,"role[%d]",friendRoleID);
		  goto EndOf_Process;
  }

  friendType = dbo.GetIntField(0);
	if(3 == friendType)    //黑名单不能聊天
	{
	    RetCode = ERR_APP_OP;
		  LOG(LOG_ERROR,__FILE__,__LINE__,"Friend[%d] is on the blacklist of Role[%d]",friendRoleID,roleID);
		  goto EndOf_Process;
	}
	*/
  //判断聊天消息是否为空
  if(chatContent.empty())
  {
      RetCode = ERR_APP_DATA;
		  LOG(LOG_ERROR,__FILE__,__LINE__,"chatContent is empty !");
		  goto EndOf_Process;
  }

  //聊天内容大小限制
  if(chatContent.size() > MAX_CHAT_CONTENT)
  {
      RetCode = ERR_APP_DATA;
		  LOG(LOG_ERROR,__FILE__,__LINE__,"chatContent size more than %d !",MAX_CHAT_CONTENT);
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

	LOG(LOG_ERROR,__FILE__,__LINE__,"RetCode[%d]",RetCode);
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

  if(0 == RetCode)
  {//好友私聊
    NotifyFriendPrivateChat(friendRoleID,roleID,Name,chatContent);
  }

}

//好友上线
void FriendSvc::OnFriendOnLine(UInt32 roleID)
{
  char szSql[1024];
	UInt32	RetCode = 0;
	int iRet;
	UInt32 friendRoleID = 0;

	Connection con;
	DBOperate dbo;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	sprintf(szSql,"select FriendRoleID from Friend\
									where RoleID = %d;",roleID);
  iRet = dbo.QuerySQL(szSql);
  if(iRet < 0)
  {
    LOG(LOG_ERROR,__FILE__,__LINE__,"Excute SQL not found ! szSql[%s]",szSql);
    return ;
  }
  if(iRet == 1)
  {
    LOG(LOG_DEBUG,__FILE__,__LINE__,"role[%d] has no friend !",roleID);
    return ;
  }

  while(dbo.HasRowData())
  {
    friendRoleID = dbo.GetIntField(0);
  	if(_mainSvc->GetCoreData()->ProcessGetRolePtr(friendRoleID)->ID() != 0)
  	{//好友上线通知
  	 //LOG(LOG_DEBUG,__FILE__,__LINE__,"Online friend:role[%d]friend[%d]",friendRoleID,roleID);
  	  NotifyFriendOnAndOffLine(friendRoleID,roleID,1);  // 1 上线， 0 下线
  	}
    dbo.NextRow();
  }

}

//好友下线
void FriendSvc::OnFriendOffLine(UInt32 roleID)
{
 	char szSql[1024];
	UInt32	RetCode = 0;
	int iRet,tmpRet = 0;
	UInt32 friendRoleID = 0;

	Connection con;
	DBOperate dbo;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	sprintf(szSql,"select FriendRoleID from Friend\
									where RoleID = %d;",roleID);
  iRet = dbo.QuerySQL(szSql);
  if(iRet < 0)
  {
    LOG(LOG_ERROR,__FILE__,__LINE__,"Excute SQL not found ! szSql[%s]",szSql);
    return ;
  }
  if(iRet == 1)
  {
    LOG(LOG_DEBUG,__FILE__,__LINE__,"role[%d] has no friend !",roleID);
    return ;
  }

  while(dbo.HasRowData())
  {
    friendRoleID = dbo.GetIntField(0);
  	tmpRet= _mainSvc->GetCoreData()->FindRoleMapID(friendRoleID);
  	if(tmpRet != 0)
  	{//好友下线通知
  	  //LOG(LOG_DEBUG,__FILE__,__LINE__,"Offline friend: role[%d] friend[%d]",friendRoleID,roleID);
  	  NotifyFriendOnAndOffLine(friendRoleID,roleID,0);  // 1 上线， 0 下线
  	}
    dbo.NextRow();
  }
}


//S-C [MsgType:1201] 请求加为好友
void FriendSvc::NotifyFriendRole(UInt32 friendRoleID, string&roleName)
{
	List<UInt32> lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 1201;
	p.UniqID = 214;
	p.PackHeader();
	lrid.push_back(friendRoleID);
	s<<roleName;
	p.UpdatePacketLength();

	if( _mainSvc->Service()->Broadcast( lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );

}


// [MsgType:1203] 添加好友成功
void FriendSvc::NotifyAddFriendSuccessd(UInt32 RoleID, ArchvFriendAddPro &friendItem)
{
  List<UInt32> lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 1203;
	p.UniqID = 214;
	p.PackHeader();

	s<<friendItem.friendName<<friendItem.friendRoleID;
	s<<friendItem.friendType<<friendItem.IsOnline<<friendItem.ProID;
	p.UpdatePacketLength();

	lrid.push_back(RoleID);

	if( _mainSvc->Service()->Broadcast( lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );

}


// [MsgType:1204]修改好友类型
void FriendSvc::NotifyAlterFriendType(UInt32 RoleID, ArchvFriendAddPro &friendItem)
{
  List<UInt32> lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 1204;
	p.UniqID = 214;
	p.PackHeader();

	s<<friendItem.friendName<<friendItem.friendRoleID;
	s<<friendItem.friendType<<friendItem.IsOnline<<friendItem.ProID;
	p.UpdatePacketLength();

	lrid.push_back(RoleID);

	if( _mainSvc->Service()->Broadcast( lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );

}


// [MsgType:1202] 删除好友成功
void FriendSvc::NotifyDeleteFriend(UInt32 RoleID,UInt32 frinedID,Byte friendType)
{
  List<UInt32> lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 1202;
	p.UniqID = 214;
	p.PackHeader();

	s<<frinedID<<friendType;

	p.UpdatePacketLength();

	lrid.push_back(RoleID);

	if( _mainSvc->Service()->Broadcast( lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );

}

// [MsgType:1205]好友私聊
void FriendSvc::NotifyFriendPrivateChat(UInt32 friendRoleID,UInt32 roleID,string& name,string &chatContent)
{
 	List<UInt32> lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 1205;
	p.UniqID = 214;
	p.PackHeader();

	s<<roleID<<name<<chatContent;

	p.UpdatePacketLength();

	lrid.push_back(friendRoleID);

	if( _mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );

}


// [MsgType:1206]好友上线、下线
//param : friendState 好友状态 1 上线， 0 下线
void FriendSvc::NotifyFriendOnAndOffLine(UInt32 friendRoleID,UInt32 roleID,Byte friendState)
{
  List<UInt32> lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 1206;
	p.UniqID = 214;
	p.PackHeader();

	s<<roleID<<friendState;

	p.UpdatePacketLength();

	lrid.push_back(friendRoleID);

	if( _mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );

}



