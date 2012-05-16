#include "MainSvc.h"
#include "DBOperate.h"
#include "DebugData.h"
#include "Packet.h"
#include "ArchvRole.h"
#include "ArchvPK.h"
#include "SSClientManager.h"
#include "TaskSvc.h"
#include "NpcSvc.h"
#include "BagSvc.h"
#include "AvatarSvc.h"
#include "PKSvc.h"
#include "LoginSvc.h"
#include "MapSvc.h"
#include "RoleInfoSvc.h"
#include "CoreData.h"
#include "ChatSvc.h"
#include "PetSvc.h"
#include "FriendSvc.h"
#include "TradeSvc.h"
#include "MailSvc.h"
#include "OffLineUpdateSvc.h"
#include "TeamSvc.h"
#include "ShopSvc.h"
#include "GuideSvc.h"
#include "DelegateSvc.h"
#include "StoreBagSvc.h"
#include "MailSvc.h"

MainSvc::MainSvc()
:_cp(NULL)
,_coreData(NULL)
,_mainSvcSS(NULL)
,_ssClientManager(NULL)
,_taskSvc(NULL)
,_npcSvc(NULL)
,_bagSvc(NULL)
,_avatarSvc(NULL)
,_pkSvc(NULL)
,_loginSvc(NULL)
,_mapSvc(NULL)
,_roleInfoSvc(NULL)
,_chatSvc(NULL)
,_petSvc(NULL)
,_storeBagSvc(NULL)
,_friendSvc(NULL)
,_tradeSvc(NULL)
,_mailSvc(NULL)
,_offlineupdateSvc(NULL)
,_teamsvc(NULL)
,_shopSvc(NULL)
{
}

MainSvc::~MainSvc()
{
	if(_coreData)
		delete _coreData;

	if(_cp)
		delete _cp;

	if(_mainSvcSS)
		delete _mainSvcSS;

	if(_ssClientManager)
		delete _ssClientManager;

	if(_taskSvc)
		delete _taskSvc;

	if(_npcSvc)
		delete _npcSvc;

	if(_bagSvc)
		delete _bagSvc;

	if(_avatarSvc)
		delete _avatarSvc;

	if(_pkSvc)
		delete _pkSvc;

	if(_loginSvc)
		delete _loginSvc;

	if(_mapSvc)
		delete _mapSvc;

	if(_roleInfoSvc)
		delete _roleInfoSvc;

	if(_chatSvc)
		delete _chatSvc;
	if(_petSvc)
		delete _petSvc;
	if(_storeBagSvc)
		delete _storeBagSvc;

	if(_friendSvc)
		delete _friendSvc;

	if(_mailSvc)
		delete _mailSvc;

	if(_tradeSvc)
		delete _tradeSvc;

	if(_offlineupdateSvc)
		delete _offlineupdateSvc;
	
	if(_teamsvc)
		delete _teamsvc;

	if(_shopSvc)
		delete _shopSvc;
		
}

int MainSvc::OnInit(void* service)
{
	_service = static_cast<GWProxy<MainSvc>*>(service);
	
	_service->SetSvcName("MainSvc");

	IniFile _file;
	if(!_file.open(_service->GetConfFilePath()))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_file.open() error!" );

		return -1;
	}

	string ip,user,password,db,poolsize;

	if( _file.read("mysql","ip", ip) ) return -1;
	if( _file.read("mysql","user", user) ) return -1;
	if( _file.read("mysql","passwd", password) ) return -1;
	if( _file.read("mysql","db", db ) ) return -1;
	if( _file.read("mysql","poolsize", poolsize) ) return -1;

	_cp = new ConnectionPool(atoi(poolsize.c_str()));
	if(!_cp->Connect(ip.c_str(),user.c_str(),password.c_str(),db.c_str()))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__, "Connect DB error, ip[%s], user[%s], passwd[%s], db[%s]",
			ip.c_str(), user.c_str(), password.c_str(), db.c_str() );
		return -1;
	}

	_timerPool.init();
	//_coreData 初始化
	_coreData = new CoreData(_cp);
	if(_coreData->Init(this))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_coreData->Init error" );
		return -1;
	}
LOG(LOG_ERROR,__FILE__,__LINE__,"_coreData run ok!!!!!!!!!!!!!!!!!!" );


	// _mainSvcSS
	_mainSvcSS = new SSServer<MainSvcSS>(_service->GetConfFilePath());
	_mainSvcSS->SetService(this);
	if(_mainSvcSS->Run())
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_mainSvcSS run error" );
		return -1;
	}

LOG(LOG_ERROR,__FILE__,__LINE__,"_mainSvcSS run ok!!!!!!!!!!!!!!!!!!" );

	// _ssClientManager
	_ssClientManager = new SSClientManager(_service->GetConfFilePath());
	if(_ssClientManager->Init())
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_ssClientManager Init error" );
		return -1;
	}

LOG(LOG_ERROR,__FILE__,__LINE__,"_ssClientManager run ok!!!!!!!!!!!!!!!!!!" );

	// _taskSvc
	_taskSvc = new TaskSvc(this,_cp);
	if( NULL == _taskSvc )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_taskSvc Init error" );
		return -1;
	}

LOG(LOG_ERROR,__FILE__,__LINE__,"_taskSvc run ok!!!!!!!!!!!!!!!!!!" );
	// _npcSvc
	_npcSvc = new NpcSvc(this,_cp);
	if( NULL == _npcSvc )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_npcSvc Init error" );
		return -1;
	}

	// _bagSvc
	_bagSvc = new BagSvc(this,_cp);
	if( NULL == _bagSvc )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_bagSvc Init error" );
		return -1;
	}

	// _avatarSvc
	_avatarSvc = new AvatarSvc(this,_cp);
	if( NULL == _avatarSvc )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_avatarSvc Init error" );
		return -1;
	}

	// _pkSvc
	_pkSvc = new PKSvc( this, _cp );
	if( NULL == _pkSvc )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_pkSvc Init error" );
		return -1;
	}

	// _loginSvc
	_loginSvc = new LoginSvc( this, _cp );
	if( NULL == _loginSvc )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_loginSvc Init error" );
		return -1;
	}

	// _mapSvc
	_mapSvc = new MapSvc( this, _cp );
	if( NULL == _mapSvc )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_mapSvc Init error" );
		return -1;
	}

	// _roleInfoSvc
	_roleInfoSvc = new RoleInfoSvc( this, _cp );
	if( NULL == _roleInfoSvc )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_roleInfoSvc Init error" );
		return -1;
	}
	_chatSvc = new ChatSvc( this, _cp );
	if( NULL == _chatSvc )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_chatSvc Init error" );
		return -1;
	}
	_petSvc=new PetSvc(this,_cp);
	if( NULL == _chatSvc )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_petSvc Init error" );
		return -1;
	}

	_storeBagSvc=new StoreBagSvc(this,_cp);
	if(NULL ==_storeBagSvc)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_STOREbAGSvc Init error" );
		return -1;
	}

	_friendSvc = new FriendSvc(this,_cp);
	if(NULL == _friendSvc)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_friendSvc Init error" );
		return -1;
	}

	_tradeSvc = new TradeSvc(this,_cp);
	if(NULL == _tradeSvc)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_tradeSvc Init error" );
		return -1;
	}

	_mailSvc = new MailSvc(this,_cp);
	if(NULL == _mailSvc)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_mailSvc Init error" );
		return -1;
	}

	_offlineupdateSvc = new OffLineUpdateSvc(this,_cp);
	if(NULL == _offlineupdateSvc)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_mailSvc Init error" );
		return -1;
	}
	
	_teamsvc = new TeamSvc(this,_cp);
	if(NULL == _teamsvc)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_teamsvc Init error" );
		return -1;
	}

	_shopSvc = new ShopSvc(this,_cp);
	if(NULL == _shopSvc)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_shopSvc Init error" );
		return -1;
	}

	_guideSvc = new GuideSvc(this,_cp);
	if(NULL == _guideSvc)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_guideSvc Init error" );
		return -1;
	}

	_delegateSvc = new DelegateSvc(this,_cp);
	if(NULL == _delegateSvc)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"_delegateSvc Init error" );
		return -1;
	}

	
	
	return 0;
}

void MainSvc::OnStop()
{
	_mainSvcSS->Stop();
}

void MainSvc::OnConnected(Session& session)
{
	LOG(LOG_ERROR,__FILE__,__LINE__,"Connect gw success, fd[%d]",session.Handle);
	DEBUG_PRINTF1( "Connect gw success, fd[%d] \n", session.Handle );
}


void MainSvc::OnProcessPacket(Session& session,Packet& packet)
{
	
DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );
	
	int moduleType = packet.MsgType/100;
//	LOG(LOG_DEBUG,__FILE__,__LINE__,"Msgtype[%d], thread id[%d]", packet.MsgType, pthread_self());

	if( 1 == moduleType )
	{// login
		_loginSvc->OnProcessPacket( session, packet);
	}
	else if( 2 == moduleType )
	{// map
		_mapSvc->OnProcessPacket( session, packet);
	}
	else if( 3 == moduleType )
	{// roleInfo
		_roleInfoSvc->OnProcessPacket( session, packet);
	}
	else if( 4 == moduleType )
	{//pk
		_pkSvc->OnProcessPacket( session, packet);
	}
	else if( 5 == moduleType )
	{//avatar
		_avatarSvc->OnProcessPacket( session, packet);
	}
	else if( 7 == moduleType )
	{//任务
		_taskSvc->OnProcessPacket( session, packet);
	}
	else if( 6 == moduleType )
	{// Npc
		_npcSvc->OnProcessPacket( session, packet);
	}
	else if( 8 == moduleType )
	{// 背包
		_bagSvc->OnProcessPacket( session, packet);
	}
	else if( 9 == moduleType )
	{
		_chatSvc->OnProcessPacket( session, packet);
	}
	else if( 10==moduleType )
	{
		_petSvc->OnProcessPacket(session,packet);
	}
	else if(11==moduleType)
	{
		_storeBagSvc->OnProcessPacket(session,packet);
	}
	else if(12 == moduleType)
	{
		_friendSvc->OnProcessPacket(session, packet);
	}
	else if(13 == moduleType)
	{
	  _tradeSvc->OnProcessPacket(session,packet);
	}
	else if(14 == moduleType)
	{
		_teamsvc->OnProcessPacket(session,packet);
	}
	else if(15 == moduleType)
	{
		_mailSvc->OnProcessPacket(session, packet);
	}
	else if(16 == moduleType)
	{
		_offlineupdateSvc->OnProcessPacket(session, packet);
	}
	else if(17 == moduleType)
	{
		_shopSvc->OnProcessPacket(session,packet);
	}
	else if(18 == moduleType)
	{
		_guideSvc->OnProcessPacket(session,packet);
	}
	else if(19 == moduleType)
	{
		_delegateSvc->OnProcessPacket(session, packet);
	}
	else
	{
		switch(packet.MsgType)
		{

			case 999:	//sS服务端测试
				ProcessSSServerTest(session,packet);
				break;


			default:
				ClientErrorAck(session,packet,ERR_SYSTEM_PARAM);
				LOG(LOG_ERROR,__FILE__,__LINE__,"MsgType[%d] not found",packet.MsgType);
				break;
		}
	}

//	LOG(LOG_DEBUG,__FILE__,__LINE__,"Msgtype[%d], thread id[%d]", packet.MsgType, pthread_self());
	
}

GWProxy<MainSvc>* MainSvc::Service()
{
	return _service;
}


CoreData * MainSvc::GetCoreData()
{
	return _coreData;
}

SSClientManager * MainSvc::GetSSClientManager()
{
	return _ssClientManager;
}


TaskSvc * MainSvc::GetTaskSvc()
{
	return _taskSvc;
}

NpcSvc * MainSvc::GetNpcSvc()
{
	return _npcSvc;
}

BagSvc * MainSvc::GetBagSvc()
{
	return _bagSvc;
}

AvatarSvc * MainSvc::GetAvatarSvc()
{
	return _avatarSvc;
}

PKSvc * MainSvc::GetPkSvc()
{
	return _pkSvc;
}

LoginSvc * MainSvc::GetLoginSvc()
{
	return _loginSvc;
}

MapSvc * MainSvc::GetMapSvc()
{
 return _mapSvc;
}

RoleInfoSvc * MainSvc::GetRoleInfoSvc()
{
 return _roleInfoSvc;
}

ChatSvc * MainSvc::GetChatSvc()
{
	return _chatSvc;
}

PetSvc * MainSvc::GetPetSvc()
{
	return _petSvc;
}
StoreBagSvc * MainSvc::GetStoreSvc()
{
	return _storeBagSvc;
}

FriendSvc * MainSvc::GetFriendSvc()
{
	return _friendSvc;
}

TradeSvc *MainSvc::GetTradeSvc()
{
  return _tradeSvc;
}

OffLineUpdateSvc* MainSvc::GetOffLineUpdateSvc()
{
	return _offlineupdateSvc;
}
TeamSvc * MainSvc::GetTeamSvc()
{
	return _teamsvc;
}

ShopSvc * MainSvc::GetShopSvc()
{
	return _shopSvc;
}

GuideSvc * MainSvc::GetGuideSvc()
{
	return _guideSvc;
}

DelegateSvc * MainSvc::GetDelegateSvc()
{
	return _delegateSvc;
}

MailSvc* MainSvc::GetMailSvc()
{
	return _mailSvc;
}


//客户端错误应答
//@param  session 连接对象
//@param	packet 请求包
//@param	RetCode 返回errorCode 值
void MainSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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


void MainSvc::ProcessSSServerTest(Session& session,Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	UInt32 RoleID;
	ArchvRoleInfo ari;

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>RoleID;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
/*	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	*/
	//ss 消息处理
	iRet = _ssClientManager->ProcessGetRoleInfo( RoleID, ari);
	if(iRet)
	{
		RetCode = iRet;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRoleInfo error" );
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
		s<<ari;
	}	

	p.UpdatePacketLength();
	
	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}
	
DEBUG_PRINTF1( "C_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
	
}



void MainSvc::ProcessScreenDisplay(Session& session,Packet& packet)
{
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

	return;
	
}

//屏显通知 S_C 通知
void MainSvc::NotifyScreenDisplay( UInt32 roleID, string &str )
{
	List<UInt32> lrid;

	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );	
	Packet p(&serbuffer);
	serbuffer.Reset();

	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 9901;
	p.UniqID = 123;

	p.PackHeader();

	s<<str;

	p.UpdatePacketLength();

	lrid.push_back(roleID);
	
	if(_service->Broadcast( lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );

}


