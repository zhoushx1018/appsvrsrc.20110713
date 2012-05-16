#include "SSClientManager.h"
#include "SSClient.h"
#include <stdlib.h>
#include "Packet.h"
#include "IniFile.h"
#include "Log.h"
#include "DebugData.h"
#include "ArchvPK.h"
#include "ArchvRole.h"

SSClientManager::SSClientManager(char* cfg)
:mutex_(),_cfg(cfg)
{
	for( int i = 0; i < MAX_SVRTYPE; i++ )
		for( int j = 0; j < MAX_SVRSEQ; j++ )
			_arrClient[i][j] = NULL;

	for( int i = 0; i < MAX_SVRTYPE; i++ )
		_maxSvrSeq[i] = -1;

	for( int i = 0; i < MAX_SVRTYPE; i++ )
		_currSvrSeq[i] = 1;
}
SSClientManager::~SSClientManager()
{
	for( int i = 0; i < MAX_SVRTYPE; i++ )
		for( int j = 0; j < MAX_SVRSEQ; j++ )
		{
			if( _arrClient[i][j] != NULL)
			{
				delete _arrClient[i][j];
				_arrClient[i][j] = NULL;
			}
		}
}

//读取参数文件
int SSClientManager::GetConf(const char* cfg)
{
	IniFile iniFile;
	string value;

	if(!iniFile.open(_cfg)) return -1;

	//服务器列表
	int iCount = 0;
	char szTmp[256] = {0};
	if(  iniFile.read( "Server", "count", value ) )return -1;
	iCount = atoi(value.c_str());

	ServerInfo serverInfo;
	for( int i = 0; i < iCount; i++ )
	{
		sprintf( szTmp, "Server_%d", i );
		if(  iniFile.read( szTmp, "svrType", value ) )return -1;
		serverInfo.svrType = atoi(value.c_str());

		if(  iniFile.read( szTmp, "svrSeq", value ) )return -1;
		serverInfo.svrSeq = atoi(value.c_str());

		if(  iniFile.read( szTmp, "ip", value ) )return -1;
		serverInfo.ip = value;

		if(  iniFile.read( szTmp, "port", value ) )return -1;
		serverInfo.port = atoi(value.c_str());

		//最大服务器序号
		if( serverInfo.svrSeq > _maxSvrSeq[serverInfo.svrType-1] )
			_maxSvrSeq[serverInfo.svrType-1] = serverInfo.svrSeq;

		_listServer.push_back(serverInfo);
	}

	return 0;
}

int SSClientManager::Init()
{
	//读取参数
	if(GetConf(_cfg))
 	{
 		LOG( LOG_ERROR, __FILE__, __LINE__, "GetConf error!" );
 		return -1;
 	}

 	
	//SS 客户端初始化
	list<ServerInfo>::iterator itor;
	SSClient * curr = NULL;
	for( itor = _listServer.begin(); itor != _listServer.end(); itor++ )
	{
		//配置参数 svrType, svrSeq 是否有重复
		if( NULL != _arrClient[itor->svrType-1][itor->svrSeq-1] )
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "error:repeat svrType or svrSeq!!");
			return -1;
		}
		
		curr = new SSClient(itor->ip, itor->port);
		_arrClient[itor->svrType-1][itor->svrSeq-1] = curr;
	}

	return 0;
}


//获取 SS客户端
//@param	svrType	服务器类型
//@param	svrSeq 服务器序号， 如果为0 则自动选择可用的 svrSeq
//@return NULL 失败   非NULL 成功
SSClient * SSClientManager::GetClientHandle( Byte svrType, Byte svrSeq )
{
	MutexLockGuard lock(mutex_);
	//校验
	if( 0 == svrType )
	{
		LOG( LOG_ERROR, __FILE__, __LINE__, "svrType[%d] error!", svrType );
		return NULL;
	}

	//校验
	if( _maxSvrSeq[svrType-1] <= 0 )
	{
		LOG( LOG_ERROR, __FILE__, __LINE__, "svrType[%d], _maxSvrSeq[%d] error!", svrType, _maxSvrSeq[svrType-1] );
		return NULL;
	}
	
	//自动选择可用的 svrSeq
	if( 0 == svrSeq )
	{
		svrSeq = _currSvrSeq[svrType-1];

		//当前序号重置
		if( ++_currSvrSeq[svrType-1] > _maxSvrSeq[svrType-1] )
			_currSvrSeq[svrType-1] = 1;
	}

DEBUG_PRINTF2( "GetClientHandle:svrType[%d], svrSeq[%d]", svrType, svrSeq );

	//获取 handle
	SSClient * curr = _arrClient[svrType-1][svrSeq-1];
	if( NULL == curr )
	{
		LOG( LOG_ERROR, __FILE__, __LINE__, "ClientHandle error! svrType[%d],svrSeq[%d]", svrType, svrSeq );
		return NULL;
	}

	return curr;
}


//msgtype 101 PK请求建立
//@param mapID   地图ID
//@param x			坐标x
//@param y			坐标y
//@return 0 成功  非0 失败的 error code
int SSClientManager::ProcessPkReq(	UInt32 mapID,	UInt16 X,	UInt16 Y, List<ArchvRolePKInfo> &lpk, UInt32 &pkID )
{
	DataBuffer	serbuffer(65535);
	Packet packet(&serbuffer);
	Serializer s(packet.GetBuffer());
	UInt32	RetCode = 0;
	int iRet = 0;
	SSClient * ssc = NULL;
	UInt16 mID = mapID;

	//包头
	packet.Direction = DIRECT_S_S_REQ;
	packet.SvrType = SVRTYPE_PK;
	packet.SvrSeq = 0;
	packet.MsgType = 101;
	packet.UniqID = 123;
	
	packet.PackHeader();

	//包体
	s<<pkID<<mID<<X<<Y<<lpk;
	packet.UpdatePacketLength();

	//发送请求
	ssc = GetClientHandle( SVRTYPE_PK, 0);
	if( NULL == ssc )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"GetClientHandle error" );
		return ERR_SYSTEM_SVRACCESS;
	}
	
	iRet = ssc->SSRequest(packet);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"SSRequest error" );
		return ERR_APP_OP;
	}
	
	s>>RetCode;
	//

		return RetCode;
}


//msgtype 101 PK请求建立
//@return 0 成功  非0 失败的 error code
int SSClientManager::ProcessGetRoleInfo(	UInt32 roleID, ArchvRoleInfo & ari) 
{
	DataBuffer	serbuffer(1024);
	Packet packet(&serbuffer);
	Serializer s(packet.GetBuffer());
	UInt32	RetCode = 0;
	int iRet = 0;
	SSClient * ssc = NULL;

	//包头
	packet.Direction = DIRECT_S_S_REQ;
	packet.SvrType = SVRTYPE_MAIN;
	packet.SvrSeq = 0;
	packet.MsgType = 101;
	packet.UniqID = 123;
	
	packet.PackHeader();

	//包体
	s<<roleID; 
	packet.UpdatePacketLength();

	//发送请求
	ssc = GetClientHandle( SVRTYPE_MAIN, 1);
	if( NULL == ssc )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"GetClientHandle error" );
		return ERR_SYSTEM_SVRACCESS;
	}
	
	iRet = ssc->SSRequest(packet);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"SSRequest error" );
		return ERR_APP_OP;
	}
	
	s>>RetCode;
	if( 0 == RetCode )
		s>>ari;

	return RetCode;
}


