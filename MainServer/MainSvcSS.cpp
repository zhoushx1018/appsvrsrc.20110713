#include "MainSvcSS.h"
#include "DBOperate.h"
#include "DebugData.h"
#include "Packet.h"
#include "ArchvRole.h"
#include "MainSvc.h"
#include "ArchvPK.h"
#include "ArchvMap.h"
#include "CoreData.h"
#include "PKSvcSS.h"
#include "AccountSvcSS.h"

MainSvcSS::MainSvcSS()
:_cp(NULL)
,_pkSvcSS(NULL)
,_accountSvcSS(NULL)
{
}

MainSvcSS::~MainSvcSS()
{
	if(_cp)
		delete _cp;

	if(_pkSvcSS)
		delete _pkSvcSS;

	if(_accountSvcSS)
		delete _accountSvcSS;
}

void MainSvcSS::SetService( MainSvc * svc )
{
	 _mainSvc = svc;
}


int MainSvcSS::OnInit(void* service)
{
	_service = static_cast<SSServer<MainSvcSS>*>(service);

	_service->SetSvcName("MainSvcSS");

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



	// _pkSvcSS 初始
	_pkSvcSS = new PKSvcSS( _mainSvc, _cp );
	if( NULL == _pkSvcSS )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__, "_pkSvcSS Init error!!!" );
		return -1;
	}

	// _accountSvcSS 初始
	_accountSvcSS = new AccountSvcSS( _mainSvc, _cp );
	if( NULL == _accountSvcSS )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__, "_accountSvcSS Init error!!!" );
		return -1;
	}


	return 0;
}

void MainSvcSS::OnStop()
{
}

void MainSvcSS::OnConnected(Session& session)
{

	LOG(LOG_DEBUG,__FILE__,__LINE__,"new session:fd[%d]",session.Handle);
}

void MainSvcSS::OnProcessPacket(Session& session,Packet& packet)
{
	
DEBUG_PRINTF1( "S_S req pkg->->->->->->MsgType[%d]\n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	int moduleType = packet.MsgType/100;

LOG(LOG_DEBUG,__FILE__,__LINE__,"MsgType[%d] ---------------------",packet.MsgType);

	if( 2 == moduleType )
	{// PK
		_pkSvcSS->OnProcessPacket( session, packet);
	}
	else if( 3 == moduleType )
	{//account
		_accountSvcSS->OnProcessPacket( session, packet);
	}
	else
	{
		ClientErrorAck(session,packet,ERR_SYSTEM_PARAM);
		LOG(LOG_ERROR,__FILE__,__LINE__,"MsgType[%d] not found",packet.MsgType);
	}

LOG(LOG_DEBUG,__FILE__,__LINE__,"MsgType[%d] ============",packet.MsgType);

}

//客户端错误应答
//@param  session 连接对象
//@param	packet 请求包
//@param	RetCode 返回errorCode 值
void MainSvcSS::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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
	
DEBUG_PRINTF1( "S_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

}


