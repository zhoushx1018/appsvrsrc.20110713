#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "AppGwSvc.h"
#include "Log.h"
#include "DebugData.h"
#include "Option.h"

#define MULTITHREAD												//测试用 

using namespace std;

void * AppGwSvc::thread_ProcMsg(void *arg)
{
	ThreadArg* thdArg = static_cast<ThreadArg*>(arg);
	
	thdArg->_service->HandleThdPopFd( thdArg->_iThreadSeq );
	
	return NULL;
}

void * AppGwSvc::thread_CloseConn(void *input)
{
	AppGwSvc * service = static_cast<AppGwSvc*>(input);
	
	while(1)
	{
		sleep(5);
		service->RemoveConnSession();
	}

	return NULL;
}


void * AppGwSvc::thread_ResetListenFd(void *input)
{
	AppGwSvc * service = static_cast<AppGwSvc*>(input);
	
	while(1)
	{
		sleep(8);
		service->ResetListenFd();
	}

	return NULL;
}




void * AppGwSvc::thread_accept_isd_blk(void *input)
{
	AppGwSvc * service = static_cast<AppGwSvc*>(input);
	service->HandleAcceptBlk( service->GetListenIsdFd() );

	return NULL;
}


void * AppGwSvc::thread_accept_osd_blk(void *input)
{
	AppGwSvc * service = static_cast<AppGwSvc*>(input);
	
	service->HandleAcceptBlk(service->GetListenOsdFd());

	return NULL;
}


AppGwSvc::~AppGwSvc()
{
	//关闭所有连接会话
	FD_CONNSS_MAP::iterator pos;
	for(pos=_fd_ConnSs_Map.begin(); pos!=_fd_ConnSs_Map.end(); ++pos)	
	{
		ConnectSession* s = pos->second;
		CloseMarkConnSession(s);
	}
	
}

AppGwSvc::AppGwSvc()
:mutexData_()
,mutexQ_()
,condQ_(mutexQ_)
,_epfd(0)
,_listenInsideFd(0)
, _listenOutsideFd(0)
{
	
	for( int i = 0; i < MAX_SVRTYPE; i++ )
		for( int j = 0; j < MAX_SVRSEQ; j++ )
			_iSrvFd[i][j] = -1;
}

void AppGwSvc::run_once()
{
	int iRet = init();
	//int iRet = init1();
	if( iRet )
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "init error!! ");
		exit(-1);
	}
	
	run();
	
	return;
}

int AppGwSvc::init1()
{
	//epoll初始化
	_epfd = epoll_create(MAXEPOLLFDNUM);
	
	//内网监听初始化
	if( ListenInit1( _listenInsideFd, bind_port_in.ip, bind_port_in.port ) )
	{
		LOG( LOG_ERROR, __FILE__, __LINE__, " ListenInit error, bind_port_in.ip[%s], bind_port_in.port[%d]", bind_port_in.ip, bind_port_in.port );
		return -1;
	}
	
		
	//外网监听初始化
	if( ListenInit1( _listenOutsideFd, bind_port_out.ip, bind_port_out.port ) )
	{
		LOG( LOG_ERROR, __FILE__, __LINE__, " ListenInit error, bind_port_out.ip[%s], bind_port_out.port[%d]", bind_port_out.ip, bind_port_out.port );
		return -1;
	}
	
	//★★★多线程代码
	//初始化业务处理子线程
	pthread_t tid;
	ThreadArg thdArg;

#ifdef MULTITHREAD
	LOG( LOG_ERROR,  __FILE__, __LINE__, "multi thread ..................." );
	for( int i = 0; i < ini.thread_num; i++ )
	{
		thdArg._service = this;
		thdArg._iThreadSeq = i+1;
		int iRet = pthread_create( &tid, NULL, thread_ProcMsg, &thdArg );
		if(iRet == 0)
			usleep(1000);
		else
			LOG(LOG_ERROR,	__FILE__, __LINE__, "pthread_create error ! iRet[%d],strerr[%s]", iRet, strerror(errno));
	}
#endif

	int iRet = 0;
	iRet = pthread_create( &tid, NULL, thread_CloseConn, this );
	if(iRet)
	{
		LOG(LOG_ERROR,	__FILE__, __LINE__, "pthread_create  thread_CloseConn error ! iRet[%d],strerr[%s]", iRet, strerror(errno));
		return -1;
	}

	iRet = pthread_create( &tid, NULL, thread_accept_isd_blk, this );
	if(iRet)
	{
		LOG(LOG_ERROR,	__FILE__, __LINE__, "pthread_create  thread_accept_isd_blk error ! iRet[%d],strerr[%s]", iRet, strerror(errno));
		return -1;
	}

	iRet = pthread_create( &tid, NULL, thread_accept_osd_blk, this );
	if(iRet)
	{
		LOG(LOG_ERROR,	__FILE__, __LINE__, "pthread_create  thread_accept_osd_blk error ! iRet[%d],strerr[%s]", iRet, strerror(errno));
		return -1;
	}
	
	
	
	
	return 0;
}


int AppGwSvc::GetListenIsdFd()
{
	return _listenInsideFd;
}

int AppGwSvc::GetListenOsdFd()
{
	return _listenOutsideFd;
}


int AppGwSvc::init()
{
	//epoll初始化
	_epfd = epoll_create(MAXEPOLLFDNUM);
	
	//内网监听初始化
	if( ListenInit( _listenInsideFd, bind_port_in.ip, bind_port_in.port ) )
	{
		LOG( LOG_ERROR, __FILE__, __LINE__, " ListenInit error, bind_port_in.ip[%s], bind_port_in.port[%d]", bind_port_in.ip, bind_port_in.port );
		return -1;
	}
	
		
	//外网监听初始化
	if( ListenInit( _listenOutsideFd, bind_port_out.ip, bind_port_out.port ) )
	{
		LOG( LOG_ERROR, __FILE__, __LINE__, " ListenInit error, bind_port_out.ip[%s], bind_port_out.port[%d]", bind_port_out.ip, bind_port_out.port );
		return -1;
	}
	
	//★★★多线程代码
	//初始化业务处理子线程
	pthread_t tid;
	ThreadArg thdArg;

#ifdef MULTITHREAD
	LOG( LOG_ERROR,  __FILE__, __LINE__, "multi thread ..................." );
	for( int i = 0; i < ini.thread_num; i++ )
	{
		thdArg._service = this;
		thdArg._iThreadSeq = i+1;
		int iRet = pthread_create( &tid, NULL, thread_ProcMsg, &thdArg );
		if(iRet == 0)
			usleep(1000);
		else
			LOG(LOG_ERROR,  __FILE__, __LINE__, "pthread_create error ! iRet[%d],strerr[%s]", iRet, strerror(errno));
	}
#endif

	int iRet = pthread_create( &tid, NULL, thread_CloseConn, this );
	if(iRet)
	{
		LOG(LOG_ERROR,  __FILE__, __LINE__, "pthread_create  thread_CloseConn error ! iRet[%d],strerr[%s]", iRet, strerror(errno));
		return -1;
	}

	iRet = pthread_create( &tid, NULL, thread_ResetListenFd, this );
	if(iRet)
	{
		LOG(LOG_ERROR,  __FILE__, __LINE__, "pthread_create  thread_ResetListenFd error ! iRet[%d],strerr[%s]", iRet, strerror(errno));
		return -1;
	}

	
	
	
	return 0;
}


int AppGwSvc::ListenInit1( int &lsfd, char * ip, u_short port )
{
	//外网监听，阻塞
	lsfd = open_tcp_port_blk( ip, port, ini.backlog);
	if( lsfd < 0 )
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, " open_tcp_port_nblk error, ip[%s], port[%d], backlog[%d]", ip, port, ini.backlog );
		return -1;
	}
	
	return 0; 
}


int AppGwSvc::ListenInit( int &lsfd, char * ip, u_short port )
{
	//外网监听，非阻塞
	lsfd = open_tcp_port_nblk( ip, port, ini.backlog);
	if( lsfd < 0 )
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, " open_tcp_port_nblk error, ip[%s], port[%d], backlog[%d]", ip, port, ini.backlog );
		return -1;
	}
	
	//epoll 事件注册
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = lsfd;
	epoll_ctl( _epfd, EPOLL_CTL_ADD, lsfd, &ev );

	
		
	return 0;	
}


void AppGwSvc::run()
{
	struct epoll_event events[MAXEVENTS];
	
	while( !stopped)
	{
		int nfds = epoll_wait(_epfd, events, MAXEVENTS, -1);

		if( nfds <= 0)
		{
			LOG( LOG_ERROR, __FILE__, __LINE__, "epoll_wait error!!! nfds[%d],errno[%d] \n", nfds, errno );
		}
//LOG( LOG_VERBOSE, __FILE__, __LINE__, "epoll_wait##### nfds[%d] \n", nfds );
		for( int n = 0; n < nfds; n++ )
		{

			if( events[n].data.fd == _listenInsideFd ||
				events[n].data.fd == _listenOutsideFd )
			{
//LOG( LOG_VERBOSE, __FILE__, __LINE__, "nfds accept n[%d], fd[%d] \n", n, events[n].data.fd );
				HandleAccept( events[n].data.fd );
			}
			else
			{
			
#ifdef MULTITHREAD
//DEBUG_PRINTF2( "multi thread ...................event[%d] fd[%d]\n", n, events[n].data.fd );
				//★★★多线程代码
				HandlePushFd(events[n].data.fd);
#else
				//☆☆☆单进程代码
				char * szBuff = new char[MAXSOCKBUFF+1];
DEBUG_PRINTF( "single process________________________\n" );
				HandleProcessPkg( events[n].data.fd, szBuff );
#endif
				//ResetFd(events[n].data.fd);

			}
		}
	}
}




//新连接处理
//参数
//
//返回值
//	空
void AppGwSvc::HandleAcceptBlk( const int lsfd )
{
	struct sockaddr_in peer;
	struct epoll_event ev;
	
	//内网连接
	while(1)
	{
		int connFd = accept_tcp_nblk( lsfd, &peer);

		if(connFd < 0)
		{
DEBUG_PRINTF2(" accept_tcp_nblk error, errno[%d], strerror[%s]", errno, strerror(errno) );
			LOG( LOG_ERROR, __FILE__, __LINE__, " accept_tcp_nblk error, errno[%d], strerror[%s]", errno, strerror(errno) );

			if( errno == EMFILE )
				sleep(1);
			
			continue;
		}
		
		//外网，需检查 IP 连接数
		if( lsfd == _listenOutsideFd )
		{
			if( CheckIpCount( peer ) )
			{
				close( connFd );
				LOG( LOG_ERROR, __FILE__, __LINE__, " CheckIpCount error, connFd[%d]!!", connFd );
				continue;
			}
		}
		
		//新的连接会话
		ConnectSession* ptrSs = NULL;
		long long llConId = CONNECTION_ID ( peer.sin_addr.s_addr, peer.sin_port, connFd );
		if( lsfd == _listenInsideFd )
			ptrSs = new ConnectSession( llConId, INSIDE_TCP_STREAM );
		else
			ptrSs = new ConnectSession( llConId, OUTSIDE_TCP_STREAM );
			
		if( ptrSs->BuffInitCheck() )
		{
			// 此时无需 close( connFd )， ConnectSession 销毁的时候自动关闭连接
			delete ptrSs;
			LOG(LOG_ERROR, __FILE__, __LINE__, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!malloc session buffer failed");
			continue;
		}
		
		
		//注册 epoll 事件
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = connFd;
		epoll_ctl(_epfd, EPOLL_CTL_ADD, connFd, &ev);
		
		//_fd_ConnSs_Map 加入新连接会话
		InsertConnSession(ptrSs);
		
		LOG( LOG_VERBOSE, __FILE__, __LINE__, "+++++++++++++ accept new  connect:fd[%d],ip[%s],port[%d]", connFd, ptrSs->GetIP(), ptrSs->GetPort());
	}
		
	return;
}


//定期将监听字注册到 epoll
//	listen内核队列满，则accept 将遇到连接数过多( errno=EMFILE )的错误，如果 epoll 是 ET模式，
//	则不会再触发该监听fd的边缘事件； 即使 客户端断开了很多连接，使得listen内核队列不再满，但是
//	epoll 也不会再触发该监听fd 的可读事件
//
//	解决办法是，这个时候只要 epoll_ctl(MOD) 一下该监听fd即可
//	本函数定期 epoll_ctl(MOD) 监听 fd
//参数
//
//返回值
//	空
void AppGwSvc::ResetListenFd()
{
	ResetFd( _listenInsideFd );
	ResetFd( _listenOutsideFd );
}


void AppGwSvc::ResetFd( int fd)
{
	struct epoll_event ev;
	
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = fd;
	epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &ev);
}



//新连接处理
//参数
//
//返回值
//	空
void AppGwSvc::HandleAccept( const int lsfd )
{
	struct sockaddr_in peer;
	struct epoll_event ev;
	
	//内网连接
	while(1)
	{
		int connFd = accept_tcp_nblk( lsfd, &peer);

		if(connFd < 0)
		{
			//没有后续的tcp连接, 或者达到系统最大文件数
			if( EAGAIN == errno || 
					EMFILE == errno)
				break;
		
			LOG( LOG_ERROR, __FILE__, __LINE__, " accept_tcp_nblk error" );
			continue;
		}
		
		//外网，需检查 IP 连接数
		if( lsfd == _listenOutsideFd )
		{
			if( CheckIpCount( peer ) )
			{
				close( connFd );
				LOG( LOG_ERROR, __FILE__, __LINE__, " CheckIpCount error, connFd[%d]!!", connFd );
				continue;
			}
		}
		
		//新的连接会话
		ConnectSession* ptrSs = NULL;
		long long llConId = CONNECTION_ID ( peer.sin_addr.s_addr, peer.sin_port, connFd );
		if( lsfd == _listenInsideFd )
			ptrSs = new ConnectSession( llConId, INSIDE_TCP_STREAM );
		else
			ptrSs = new ConnectSession( llConId, OUTSIDE_TCP_STREAM );
			
		if( ptrSs->BuffInitCheck() )
		{
			// 此时无需 close( connFd )， ConnectSession 销毁的时候自动关闭连接
			delete ptrSs;
			LOG(LOG_ERROR, __FILE__, __LINE__, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!malloc session buffer failed");
			continue;
		}
		
		
		//注册 epoll 事件
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = connFd;
		epoll_ctl(_epfd, EPOLL_CTL_ADD, connFd, &ev);
		
		//_fd_ConnSs_Map 加入新连接会话
		InsertConnSession(ptrSs);
		
		LOG( LOG_VERBOSE, __FILE__, __LINE__, "+++++++++++++ accept new  connect:fd[%d],ip[%s],port[%d]",	connFd, ptrSs->GetIP(), ptrSs->GetPort());
	}
		
	return;
}


void AppGwSvc::HandlePushFd( const int fd )
{
	MutexLockGuard lock(mutexQ_);
	_queFd.push(fd);
	condQ_.notify();
}


void AppGwSvc::HandleThdPopFd( int thdSeq )
{
	char * szMsgBuffer = new char[MAXSOCKBUFF+1];
	if( szMsgBuffer == NULL )
	{
		LOG( LOG_ERROR, __FILE__, __LINE__,  "buff malloc error, thread exit!!!!!!!!!!!!!!!!!!!!!!!" );
		return;
	}
	
	memset( szMsgBuffer, 0, sizeof(szMsgBuffer) );
	
//	LOG( LOG_VERBOSE, __FILE__, __LINE__,  "HandleThdPopFd : pid[%d],thdSeq[%d]", getpid(), thdSeq );
	
	while(1)
	{
		int fd = 0;
		{
		    MutexLockGuard lock(mutexQ_);
		    while( _queFd.size() == 0 ){
			     condQ_.wait();
		    }

		    fd = _queFd.front();
		    _queFd.pop();
		}
		
		HandleProcessPkg( fd, szMsgBuffer );

		ResetFd(fd);
	}
}

void AppGwSvc::HandleProcessPkg( int fd, char * ptrBuff )
{
	int iPkgLen = 0 ;
	int iRet = 0;


DEBUG_PRINTF1( "HandleProcessPkg, ---fd[%d]", fd );


	//查找连接会话
	ConnectSession* ss = NULL;
	if( (ss = FindFdConnSession(fd)) == NULL )
	{
		CloseEpollFd(fd);
		LOG( LOG_ERROR, __FILE__, __LINE__, " HandleProcessPkg():FindFdConnSession error, fd[%d], not found!!", fd );
		return;
	}

	//判断连接是否已关闭
	if( ss->GetCloseFlag() )
		return;
		
	//接收数据
	iRet = ss->RecvMsg();
	if( iRet == 0 )
		return;
	else if( iRet < 0 )
	{
		//连接关闭
//		LOG( LOG_ERROR, __FILE__, __LINE__, " HandleProcessPkg():conn need to close, fd[%d], errno[%d], err msg[%s]!", fd, errno, strerror(errno) );
		CloseMarkConnSession(ss);
		return;
	}
	
	//获取请求包
	while( (iRet = ss->GetDataPack( ptrBuff, iPkgLen )) )
	{
		if( iRet == 0 )
			return;
		else if( iRet < 0 )
		{
			LOG( LOG_ERROR, __FILE__, __LINE__, " HandleProcessPkg():GetDataPack error, iRet[%d], remove connSession!!", iRet );
			CloseMarkConnSession(ss);
			return;
		}
			
		//处理请求包
		if( ss->GetConnType() == INSIDE_TCP_STREAM )
		{
			PkgHead * pkg = (PkgHead*)ptrBuff;
			
			switch( pkg->ucDirection )
			{
				case DIRECT_C_S_RESP:
				case DIRECT_S_C_REQ:
					
					ProcessS2C( ss, ptrBuff );	
					break;
					
				case DIRECT_S_S_REQ:
					ProcessS2G( ss, ptrBuff );	
					break;
					
				default:
					LOG( LOG_ERROR, __FILE__, __LINE__, " bad pkg->ucDirection[%d], srvType[%d], connSs->ConnType[%d]", pkg->ucDirection, ss->GetSrvType(), ss->GetConnType() );
					break;
			}
			
		}
		else if ( ss->GetConnType() == OUTSIDE_TCP_STREAM )
		{
			PkgHead * pkg = (PkgHead*)ptrBuff;
			
			switch( pkg->ucDirection )
			{
				
				case DIRECT_C_S_REQ:
				case DIRECT_S_C_RESP:
					ProcessC2S( ss, ptrBuff);
					break;
					
				default:
					LOG( LOG_ERROR, __FILE__, __LINE__, " bad pkg->ucDirection[%d], srvType[%d], connSs->ConnType[%d]", pkg->ucDirection, ss->GetSrvType(), ss->GetConnType() );
					break;			
			}
		}
		else
		{
			LOG( LOG_ERROR, __FILE__, __LINE__, " bad connType[%d], remove connSession!!", ss->GetConnType() );
			CloseMarkConnSession(ss);
		}
	}
}

void AppGwSvc::ProcessS2G( ConnectSession* reqConnSs, const char * ptrReqPkg )
{
	PkgHead * ptrReqPkgHead = (PkgHead *)ptrReqPkg;
	
	PkgBuff	pkgbuff;
	PkgHead	ackPkgHead;
	
	memset( &ackPkgHead, 0, sizeof(ackPkgHead) );
	
	if( MSGTYPE_GW_SVRCONNECT == ptrReqPkgHead->usMsgType && 		
		DIRECT_S_S_REQ == ptrReqPkgHead->ucDirection )
	{
		//服务器连接网关请求交易,需关注包内容
		unsigned int uiRetCode = 0;
		PKGBODY_SG_SVRCONNECT_REQ pkgBody;
		memset( &pkgBody, 0, sizeof(pkgBody));
		
		//组应答包包头	
		memcpy( &ackPkgHead, ptrReqPkgHead, sizeof(PkgHead) );
			
		//请求包包体
		memcpy( &pkgBody, ptrReqPkg + sizeof(PkgHead), sizeof(PKGBODY_SG_SVRCONNECT_REQ) );
			
		//判断请求包包体 srvtype srvseq 范围
		if( check_srv_scope( pkgBody.ucSrvType, pkgBody.ucSrvSeq) )
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "S_G bad pkgBody  ucSrvType[%d]ucSrvSeq[%d]",	pkgBody.ucSrvType, pkgBody.ucSrvSeq);
			return;
		}
			
		//获取 srvfd
		int srvfd = _iSrvFd[pkgBody.ucSrvType -1 ][pkgBody.ucSrvSeq-1];
			
		//应答包组合
		ackPkgHead.ucDirection = DIRECT_S_S_RESP;		
		
		if( srvfd > 0 )
			uiRetCode = ERR_SYSTEM_SVRLOGINED;		//服务器已登录
		else
			uiRetCode = 0;
		
		//组建发送包
		pkgbuff.Clear();
		pkgbuff.Append( &ackPkgHead, (int)sizeof(ackPkgHead) );
		pkgbuff.Append( &uiRetCode, (int)sizeof(uiRetCode) );
		pkgbuff.EncodeLength();
	
DEBUG_PRINTF( " S_G, socket_send to srv\n" );
	DEBUG_SHOWHEX( pkgbuff.GetBuff(), pkgbuff.GetSize(), 0, __FILE__, __LINE__ );
	
		int iSend = reqConnSs->SendMsg( pkgbuff.GetBuff(), pkgbuff.GetSize() );
	
		if( iSend < 0 )
		{
				CloseMarkConnSession( reqConnSs );
				LOG( LOG_ERROR, __FILE__, __LINE__, "S_G ,connSs send msg error, srvType[%d], CloseMarkConnSession!", reqConnSs->GetSrvType());
				return;
		}
		else if( pkgbuff.GetSize() == iSend )
		{
			//同一 srvtype、srvseq是否已经登录
			if( srvfd > 0 )
			{
				LOG(LOG_ERROR, __FILE__, __LINE__, " process_s_g() ,srv is logined!! srvType[%d]ucSrvSeq[%d]",
					pkgBody.ucSrvType, pkgBody.ucSrvSeq );
				return;
			}
				
			//记录服务器连接
			_iSrvFd[pkgBody.ucSrvType -1 ][pkgBody.ucSrvSeq-1] = CONNECTION_FD(reqConnSs->GetConnId());
			reqConnSs->SetSrvType(pkgBody.ucSrvType);
			reqConnSs->SetSrvSeq(pkgBody.ucSrvSeq);
			reqConnSs->SetLoginStatus(1);
	
			LOG( LOG_VERBOSE, __FILE__, __LINE__, "S_G ,srvType[%d]SrvSeq[%d] [%lld] login success!!!!!", pkgBody.ucSrvType, pkgBody.ucSrvSeq, reqConnSs->GetConnId() );
				
			return;
		}
		else
		{
			LOG( LOG_ERROR, __FILE__, __LINE__, "S_G ,connId[%lld]ip[%s]port[%d] login fail!!!", reqConnSs->GetConnId(), reqConnSs->GetIP(), reqConnSs->GetPort());
			return;
		}
	}
	else if( MSGTYPE_GW_CLOSECLIENT == ptrReqPkgHead->usMsgType &&		
		DIRECT_S_S_REQ == ptrReqPkgHead->ucDirection )
	{
		//关闭指定客户端连接				
		ConnectSession * client_session = NULL;		
		client_session = FindRoleConnSession( ptrReqPkgHead->uiRoleID );
		
		if( NULL != client_session )
		{
			LOG( LOG_ERROR, __FILE__, __LINE__, "S_G, close the _client! CloseMarkConnSession, roleid[%d] ", ptrReqPkgHead->uiRoleID );
			CloseMarkConnSession( client_session );
			return;
		}
		
		LOG( LOG_ERROR, __FILE__, __LINE__, "S_G, close the _client error, roleid[%d] not found!", ptrReqPkgHead->uiRoleID );
		return;		
	}
	else
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "S_G, srvType[%d]SrvSeq[%d] unknow commandid[%d]!", ptrReqPkgHead->ucSrvType, ptrReqPkgHead->ucSrvSeq, ptrReqPkgHead->usMsgType);
		return;
	}
	
	
	
}


//组建客户端端应答包
//参数
//	buff						缓存
//	uiRetCode				错误代码
//返回值
//	无
void AppGwSvc::MakeClientAckPkg( PkgBuff &buff, PkgHead & pkgHead, unsigned int uiRetCode )
{
	//组建发送包
	buff.Clear();
	buff.Append( &pkgHead, sizeof(pkgHead) );
	buff.Append( &uiRetCode, sizeof(uiRetCode) );
	buff.EncodeLength();
	
}

void AppGwSvc::ProcessC2S( ConnectSession* reqConnSs, const char * ptrReqPkg )
{
	
	int srvfd;
	PkgBuff	pkgbuff;
	
	PkgHead * ptrReqPkgHead = (PkgHead *)ptrReqPkg;		//外网请求包头
	PKGBODY_CS_ROLELOGIN_REQ_R * ptrReqPkgBody = NULL;				//外网登录请求包体
	PkgHead tranPkgHead;											//内网包头
	PKGBODY_CS_ROLELOGIN_REQ_L tranPkgBody;								//内网登录请求包体
	
	//初始化
	memset( &tranPkgHead, 0, sizeof(tranPkgHead) );
	memset( &tranPkgBody, 0, sizeof(tranPkgBody) );

	// srv范围检查
	if( check_srv_scope( ptrReqPkgHead->ucSrvType, ptrReqPkgHead->ucSrvSeq) )
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "C_S ucSrvType[%d]ucSrvSeq[%d] error",	ptrReqPkgHead->ucSrvType, ptrReqPkgHead->ucSrvSeq);
		return;
	}
	
	//获取llSrvid
	srvfd = _iSrvFd[ptrReqPkgHead->ucSrvType-1][ptrReqPkgHead->ucSrvSeq-1];
	
	//srvfd校验
	if(srvfd <= 0)
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "C_S, srvfd[%d] is invalid, srvType[%d],SrvSeq[%d]", srvfd, ptrReqPkgHead->ucSrvType, ptrReqPkgHead->ucSrvSeq );
		
		//组应答包返回客户端
		PkgHead ackPkgHead;
		memcpy( &ackPkgHead, ptrReqPkgHead, sizeof(PkgHead) );
		ackPkgHead.ucDirection = DIRECT_C_S_RESP;
		
		MakeClientAckPkg( pkgbuff, ackPkgHead, ERR_SYSTEM_SVRNOTSTART );
		
		int iSend = reqConnSs->SendMsg( pkgbuff.GetBuff(), pkgbuff.GetSize() );
		if( iSend < 0 )
		{
			CloseMarkConnSession( reqConnSs );
			LOG( LOG_ERROR, __FILE__, __LINE__, "C_S ,reqConnSs send msg error, roleid[%d], CloseMarkConnSession!", reqConnSs->GetRoleID());
		}
		
DEBUG_PRINTF( "C_S --> srvfd is invalid, ack to client Pkg:\n" );
	DEBUG_SHOWHEX( pkgbuff.GetBuff(), pkgbuff.GetSize(), 0, __FILE__, __LINE__ );

		
		return;
	}
	
	ConnectSession* srvConnSs = FindFdConnSession( srvfd );
	if( srvConnSs == NULL )
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "C_S, srvConnSs not found, srvfd[%d]", srvfd );
		return;
	}
	
	
	//包头Direction 校验
	if( DIRECT_C_S_REQ != ptrReqPkgHead->ucDirection )
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "C_S, direction isn't DIRECT_C_S_REQ " );
		return;
	}

DEBUG_PRINTF( "C_S --> Pkg head:\n" );
	DEBUG_SHOWHEX( (char*)ptrReqPkg, sizeof(PkgHead), 0, __FILE__, __LINE__ );

	
	//内外网包的转换
	if( SVRTYPE_MAIN == ptrReqPkgHead->ucSrvType &&
		MSGTYPE_CS_ROLELOGIN == ptrReqPkgHead->usMsgType )
	{// C_S online srv 登录交易请求，需关注包体内容
		
		ptrReqPkgBody = (PKGBODY_CS_ROLELOGIN_REQ_R *)( ptrReqPkg + sizeof(PkgHead) );
		
		//内网包头，处理 userid 字段
		memcpy( &tranPkgHead, ptrReqPkgHead, sizeof(PkgHead) );
		tranPkgHead.uiRoleID = ptrReqPkgBody->uiRoleID;

		//内网包体，处理 ClientId 字段
		memcpy( &tranPkgBody, ptrReqPkgBody, sizeof(PKGBODY_CS_ROLELOGIN_REQ_R) );
		tranPkgBody.llClientId = reqConnSs->GetConnId();

		//组建发送包
		pkgbuff.Clear();
		pkgbuff.Append( &tranPkgHead, sizeof(tranPkgHead) );
		pkgbuff.Append( &tranPkgBody, sizeof(tranPkgBody) );
		pkgbuff.EncodeLength();
	}
	else
	{// 其他交易，仅需调整包头
		
		//客户端是否已登录
		if( 0 == reqConnSs->GetLoginStatus() )
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "client isn't logined!" );
			
			//组应答包返回客户端
			PkgHead ackPkgHead;
			memcpy( &ackPkgHead, ptrReqPkgHead, sizeof(PkgHead) );
			ackPkgHead.ucDirection = DIRECT_C_S_RESP;
			ackPkgHead.uiRoleID = 0;		//对客户端屏蔽 roleID
			
			MakeClientAckPkg( pkgbuff, ackPkgHead, ERR_SYSTEM_ROLENOTLOGIN );
			
			int iSend = reqConnSs->SendMsg( pkgbuff.GetBuff(), pkgbuff.GetSize() );
			if( iSend < 0 )
			{
				CloseMarkConnSession( reqConnSs );
				LOG( LOG_ERROR, __FILE__, __LINE__, "C_S ,reqConnSs send msg error, roleid[%d], CloseMarkConnSession!", reqConnSs->GetRoleID());
			}
			
			return;
		}
		
		//内网包头，处理 userid 字段
		memcpy( &tranPkgHead, ptrReqPkgHead, sizeof(PkgHead) );
		tranPkgHead.uiRoleID = reqConnSs->GetRoleID();

		//组建发送包
		pkgbuff.Clear();
		pkgbuff.Append( &tranPkgHead, sizeof(tranPkgHead) );
		pkgbuff.Append( ptrReqPkg+sizeof(PkgHead), ptrReqPkgHead->usPkgLen - sizeof(PkgHead));
		pkgbuff.EncodeLength();
		
	}

DEBUG_PRINTF( "C_S --> pkg, send to srv\n" );
	DEBUG_SHOWHEX( pkgbuff.GetBuff(), pkgbuff.GetSize(), 0, __FILE__, __LINE__ );

	int iSend = srvConnSs->SendMsg( pkgbuff.GetBuff(), pkgbuff.GetSize() );
	if( iSend < 0 )
	{
		//清除srv 的连接会话
		CloseMarkConnSession( srvConnSs );
		LOG( LOG_ERROR, __FILE__, __LINE__, "C_S ,srvConnSs send msg error,srvType[%d], CloseMarkConnSession!", srvConnSs->GetSrvType());
		
		//组应答包返回客户端
		PkgHead ackPkgHead;
		memcpy( &ackPkgHead, ptrReqPkgHead, sizeof(PkgHead) );
		ackPkgHead.ucDirection = DIRECT_C_S_RESP;
		
		MakeClientAckPkg( pkgbuff, ackPkgHead, ERR_SYSTEM_SVRACCESS );
		
		iSend = reqConnSs->SendMsg( pkgbuff.GetBuff(), pkgbuff.GetSize() );
		if( iSend < 0 )
		{
			CloseMarkConnSession( reqConnSs );
			LOG( LOG_ERROR, __FILE__, __LINE__, "C_S ,reqConnSs send msg error,roleid[%d], CloseMarkConnSession!", reqConnSs->GetRoleID());
		}
		
		return;
	}
	
	return;
}


void AppGwSvc::ProcessS2C( ConnectSession* reqConnSs, const char * ptrReqPkg )
{
	
	PkgBuff	pkgbuff;
	PkgHead * ptrReqPkgHead = (PkgHead *)ptrReqPkg;		//内网请求包头
	PKGBODY_CS_ROLELOGIN_RESP_L * ptrReqPkgBody_RspnLogin = NULL;				//内网登录应答包体
	PkgHead tranPkgHead;															//外网包头
	unsigned int uiRetCode;												//外网登录应答包体
	
	ConnectSession* s_client = NULL;
	ConnectSession* s_logined = NULL;
	
	//初始化
	
	memset( &tranPkgHead, 0, sizeof(tranPkgHead) );
	memset( &uiRetCode, 0, sizeof(uiRetCode) );


	//判断 srvtype srvseq 范围
	if( check_srv_scope( ptrReqPkgHead->ucSrvType, ptrReqPkgHead->ucSrvSeq) )
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, " S_C, ucSrvType[%d],ucSrvSeq[%d] or error",	ptrReqPkgHead->ucSrvType, ptrReqPkgHead->ucSrvSeq );
		return;
	}
	
	//内外网包的转换
	
	if( DIRECT_C_S_RESP == ptrReqPkgHead->ucDirection &&
		SVRTYPE_MAIN == ptrReqPkgHead->ucSrvType &&
		MSGTYPE_CS_ROLELOGIN == ptrReqPkgHead->usMsgType )
	{// C_S online srv 登录交易应答，需关注包体内容
		
		ptrReqPkgBody_RspnLogin = (PKGBODY_CS_ROLELOGIN_RESP_L*) ( ptrReqPkg + sizeof(PkgHead) );
		
		//外网包头
		memcpy( &tranPkgHead, ptrReqPkgHead, sizeof(PkgHead) );
		tranPkgHead.uiRoleID = 0;			//对客户端屏蔽 roleID

		//目标客户端连接会话查找
		s_client = FindFdConnSession( CONNECTION_FD(ptrReqPkgBody_RspnLogin->llClientId) );
		if ( s_client == NULL)			
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "S_C fd [%d]not exists ", CONNECTION_FD(ptrReqPkgBody_RspnLogin->llClientId) );
			return;
		}
		
		//组建外网应答包体
		uiRetCode = ptrReqPkgBody_RspnLogin->uiRetCode;
		
		if( 0 == ptrReqPkgBody_RspnLogin->uiRetCode )
		{
			//如之前已经有用户已经登录，且并非本连接session，则断开之前已登录的连接session（用户）
			s_logined = FindRoleConnSession( ptrReqPkgBody_RspnLogin->uiRoleID );

			if( NULL != s_logined  &&
				s_logined->GetLoginStatus() &&
				s_logined != s_client )
			{
				UpdateUserFdMap( ptrReqPkgBody_RspnLogin->uiRoleID, CONNECTION_FD(ptrReqPkgBody_RspnLogin->llClientId) );
				s_logined->SetIsSameUser(1);		//设置标志，避免CloseMarkConnSession 向 online srv 重复发同一UserID 的logout交易
				CloseMarkConnSession( s_logined );
				LOG( LOG_ERROR, __FILE__, __LINE__, " repeat login, CloseMarkConnSession previous client, roleid[%d]!!", s_logined->GetRoleID() );
			}
			else
				InsertUserFdMap( ptrReqPkgBody_RspnLogin->uiRoleID, CONNECTION_FD(ptrReqPkgBody_RspnLogin->llClientId) );
			
			//登录成功, 对应客户端 session 信息更新
			s_client->SetSrvType( 0 );
			s_client->SetLoginStatus(1);
			s_client->SetRoleID( ptrReqPkgBody_RspnLogin->uiRoleID );
			
		}
		else
		{
			LOG( LOG_ERROR, __FILE__, __LINE__, "roleid[%d], login fail, retcode[%d] ", ptrReqPkgBody_RspnLogin->uiRoleID, ptrReqPkgBody_RspnLogin->uiRetCode );
		}
		
		//组建发送包
		pkgbuff.Clear();
		pkgbuff.Append( &tranPkgHead, sizeof(tranPkgHead) );
		pkgbuff.Append( &uiRetCode, sizeof(uiRetCode) );
		pkgbuff.Append( &(ptrReqPkgBody_RspnLogin->uiRoleID), sizeof(ptrReqPkgBody_RspnLogin->uiRoleID) );		
		pkgbuff.EncodeLength();
	}
	else
	{// 其他交易，仅需调整包头
		
		//外网包头
		memcpy( &tranPkgHead, ptrReqPkgHead, sizeof(PkgHead) );
		tranPkgHead.uiRoleID = 0;		//对客户端屏蔽 roleID
		
		//客户端 session 查找
		if ( (s_client = FindRoleConnSession( ptrReqPkgHead->uiRoleID ) ) == NULL )
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "roleid[%d], connSession not found ", ptrReqPkgHead->uiRoleID );
			return;
		}
		
		//组建发送包
		pkgbuff.Clear();
		pkgbuff.Append( &tranPkgHead, sizeof(tranPkgHead) );
		pkgbuff.Append( ptrReqPkg+sizeof(PkgHead), ptrReqPkgHead->usPkgLen - sizeof(PkgHead));
		pkgbuff.EncodeLength();
	}
	

DEBUG_PRINTF( "S_C --> pkg, send to srv\n" );
	DEBUG_SHOWHEX( pkgbuff.GetBuff(), pkgbuff.GetSize(), 0, __FILE__, __LINE__ );	
	
	int iSend = s_client->SendMsg( pkgbuff.GetBuff(), pkgbuff.GetSize() );
	if( -1 == iSend )
	{// SendMsg 返回 -2的情况不用考虑, 因为 返回 -2 说明之前已经 CloseMarkConnSession 过了
		//清除client的连接会话
		CloseMarkConnSession( s_client );
		LOG( LOG_ERROR, __FILE__, __LINE__, "S_C ,clientConnSs send msg error,CloseMarkConnSession!! roleid[%d], errno[%d]", s_client->GetRoleID(), errno);
		return;
	}

	return;
}






//单个 IP 连接数校验
//参数
//	peer		socket 地址描述
//返回值
//	0		成功
//	非0	失败
int AppGwSvc::CheckIpCount( const struct sockaddr_in &peer )
{
	IP_COUNT_MAP::iterator pos;
	MutexLockGuard lock(mutexData_);
	char szTmp[64];
	
	pos = _ip_Count_Map.find(peer.sin_addr.s_addr);
	int connect_count = 0;
	if(pos != _ip_Count_Map.end())
	{
		connect_count =(int)pos->second;
		if(connect_count > ini.ip_connect_limit)
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "CheckIpCount error, over count limit , ip[%s],port[%d]", inet_ntop( AF_INET, &peer.sin_addr, szTmp, sizeof(szTmp)), ntohs(peer.sin_port));
			return -1;
		}
	}
	connect_count++;
	_ip_Count_Map[peer.sin_addr.s_addr] = connect_count;
	
	return 0;
}

int AppGwSvc::InsertConnSession(ConnectSession* s)
{
	MutexLockGuard lock(mutexData_);
	
	int key = CONNECTION_FD(s->GetConnId());
	_fd_ConnSs_Map.insert(make_pair(key, s));
	
//	LOG( LOG_VERBOSE, __FILE__, __LINE__, "insert into _fd_ConnSs_Map,fd[%d],cConnType=[%d], _fd_ConnSs_Map.size[%d] ", key, s->GetConnType(), _fd_ConnSs_Map.size() );
	
	
	return 0;
}

void AppGwSvc::RemoveConnSession()
{
	//连接会话关闭队列，是否有元素
	if( _queCloseConn.size() == 0 )
		return;

	MutexLockGuard lock(mutexData_);
	FD_CONNSS_MAP::iterator pos;
	in_addr adrtTmp;
	int iQueSize = _queCloseConn.size();

//	LOG( LOG_VERBOSE, __FILE__, __LINE__, "Remove ConnSession begin### _fd_ConnSs_Map[%d],_roleID_Fd_Map[%d] ", _fd_ConnSs_Map.size(),_roleID_Fd_Map.size());

	while( iQueSize-- > 0 )
	{
 
		long long llConId = _queCloseConn.front();
		_queCloseConn.pop();
	
		//查找连接会话
		int fd = CONNECTION_FD(llConId);
		pos = _fd_ConnSs_Map.find(fd);
		if( pos == _fd_ConnSs_Map.end())
		{
//			LOG(LOG_ERROR, __FILE__, __LINE__, "RemoveConnSession(), can't find session map, fd[%d], _fd_ConnSs_Map.size [%d] \n", fd, _fd_ConnSs_Map.size() );
			continue;
		}
		ConnectSession* s = pos->second;

		//时间戳判断
		if( time(NULL) - s->GetStamp() < GW_CONNCLOSe_INTERNAL )
		{
			_queCloseConn.push(llConId);
			continue;
		}

		//分内外网清理其他数据
		if( s->GetConnType() == INSIDE_TCP_STREAM )
		{//内网连接处理
			
			//状态为登录，才将 _iSrvFd 置为 -1
			//因为已登录状态的内网session，ucSrvType 才填入了正确的值
			if( s->GetLoginStatus() )
			{
				_iSrvFd[s->GetSrvType()-1][s->GetSrvSeq()-1] = -1;
				LOG( LOG_ERROR, __FILE__, __LINE__, "Removing svrConnSession; set _iSrvFd to -1, svrType[%d],svrSeq[%d]", s->GetSrvType(), s->GetSrvSeq());
			}
			LOG( LOG_ERROR, __FILE__, __LINE__, "Removing svrConnSession;");
		}
		else if( s->GetConnType() == OUTSIDE_TCP_STREAM )
		{//外网连接处理
			
			//是否被踢掉的旧连接
			if( 0 == s->GetIsSameUser() )
			{
				//通知到 OL srv
				NotifyClientClosed(s);
			
				//删除 _roleID_Fd_Map 记录， 用户名重复的旧连接除外
				RemoveRoleFdMap( s->GetRoleID());
			}
			
			//删除 _ip_Count_Map
			IP_COUNT_MAP::iterator pos_ip;	
			pos_ip = _ip_Count_Map.find(CONNECTION_IP(s->GetConnId()));
			int connect_count = 0;
			if(pos_ip != _ip_Count_Map.end())
			{
				connect_count =(int)pos_ip->second;
				connect_count--;
				if(connect_count <= 0)
				{
					_ip_Count_Map.erase(pos_ip);
				}
				else
				{
					_ip_Count_Map[CONNECTION_IP(s->GetConnId())] = connect_count;
				}
				
			}
			else
			{
				LOG(LOG_ERROR, __FILE__, __LINE__, "_client connID[%lld], ip[%s] not find in map",	s->GetConnId(), s->GetIP());
			}
			
		}
		else
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "RemoveConnSession unknow conntype[%d]!!!!!!!!!!", s->GetConnType() );
		}
		
		//delete 会话连接对象
//		LOG( LOG_VERBOSE, __FILE__, __LINE__, "----Remove ConnSession success!! fd[%d],ip[%s],port[%d],Conntype[%d], roleid[%d], srvType[%d] ",	s->GetFd(), s->GetIP(), s->GetPort(), s->GetConnType(), s->GetRoleID(), s->GetSrvType());
		delete s;
		
		//_fd_ConnSs Map 删除连接会话
		_fd_ConnSs_Map.erase(pos);
		adrtTmp.s_addr = CONNECTION_IP(llConId);
	}

//	LOG( LOG_VERBOSE, __FILE__, __LINE__, "---Remove ConnSession end__ _fd_ConnSs_Map[%d],_roleID_Fd_Map[%d] ", _fd_ConnSs_Map.size(),_roleID_Fd_Map.size());

	return;
}


void AppGwSvc::CloseEpollFd( int fd )
{
	MutexLockGuard lock(mutexData_);

	close(fd);

	//注销 epoll 事件
	struct epoll_event ev;
	ev.data.fd = fd;
	epoll_ctl( _epfd, EPOLL_CTL_DEL, fd, &ev );
	
}

void AppGwSvc::CloseMarkConnSession( ConnectSession* s )
{
	MutexLockGuard lock(mutexData_);
	in_addr adrtTmp;

	//标记时间戳，及设置关闭标志
	s->SetStamp();
	s->CloseConn();

	//入连接关闭队列
	_queCloseConn.push(s->GetConnId());
	
	//注销 epoll 事件
	struct epoll_event ev;
	int fd = CONNECTION_FD(s->GetConnId());
	ev.data.fd = fd;
	epoll_ctl( _epfd, EPOLL_CTL_DEL, fd, &ev );

	long long llConId = s->GetConnId();
	adrtTmp.s_addr = CONNECTION_IP(llConId);
//	LOG( LOG_VERBOSE, __FILE__, __LINE__, "CloseMarkConnSession  success!! fd[%d],ip[%s],port[%d], SessionConnType[%d],roleid[%d],srvType[%d] ",
//		s->GetFd(), s->GetIP(), s->GetPort(), s->GetConnType(), s->GetRoleID(), s->GetSrvType());

	return;
}

ConnectSession* AppGwSvc::FindFdConnSession(int fd)
{	
	FD_CONNSS_MAP::iterator pos;
	MutexLockGuard lock(mutexData_);
	
	pos = _fd_ConnSs_Map.find(fd);
	if(pos == _fd_ConnSs_Map.end())
		return NULL;

	return pos->second;
}

void AppGwSvc::check_timeout(ConnectSession* s, int timeout)
{
	if(s->GetConnType() != OUTSIDE_TCP_STREAM )
		return;

	int difftime = time(NULL) - s->GetStamp();
	if(difftime > timeout || difftime < 0)
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "check_timeout,CloseMarkConnSession: srvType[%d] _llConnId[%lld] closed", s->GetSrvType(),s->GetConnId());

		CloseMarkConnSession( s );
		return;
	}
	
	return;
}

void AppGwSvc::NotifyClientClosed( ConnectSession* s )
{
	PkgHead reqPkgHead;								//内网包头
	PkgBuff	pkgbuff;
	int srvfd;
	
	//初始化
	memset( &reqPkgHead, 0, sizeof(reqPkgHead) );
	
	//获取 Srvfd
	srvfd = _iSrvFd[SVRTYPE_MAIN-1][SVRSEQ_MAIN-1];

	if(srvfd == -1)
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "NotifyClientClosed, online srv srvfd[%d] is invalid", srvfd );
		return;
	}
	
	//查找 online 的连接会话
	FD_CONNSS_MAP::iterator it;
	it = _fd_ConnSs_Map.find(srvfd);
	if(it == _fd_ConnSs_Map.end())
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "NotifyClientClosed, online connSession not found " );
		return;
	}

	ConnectSession * connSsOl = it->second;
	
	//如果是被踢掉的旧连接，则不发 logout 消息到 online srv 
	if( s->GetIsSameUser() )
		return;
		
	//空用户名，直接返回
	if( 0 == s->GetRoleID() )
		return;
	
	//内网包头，处理 userid 字段
	reqPkgHead.ucSrvType = SVRTYPE_MAIN;
	reqPkgHead.ucSrvSeq = 1;
	reqPkgHead.usMsgType = MSGTYPE_CS_ROLELOGOUT;
	reqPkgHead.uiUniqID = 111;
	reqPkgHead.ucDirection = DIRECT_C_S_REQ;
	reqPkgHead.uiRoleID = s->GetRoleID();
	

	//组建发送包
	pkgbuff.Clear();
	pkgbuff.Append( &reqPkgHead, (int)sizeof(reqPkgHead) );
	pkgbuff.EncodeLength();
	
DEBUG_PRINTF( " NotifyClientClosed, socket_send to online srv\n" );
	DEBUG_SHOWHEX( pkgbuff.GetBuff(), pkgbuff.GetSize(), 0, __FILE__, __LINE__ );

	//请求转发 srv
	int iSend = connSsOl->SendMsg( pkgbuff.GetBuff(), pkgbuff.GetSize() );
	if( iSend < 0 )
	{
		LOG( LOG_ERROR, __FILE__, __LINE__, "NotifyClientClosed  SendMsg error--- ");
		return;
	}
	
	//成功发送
	LOG( LOG_VERBOSE, __FILE__, __LINE__, "NotifyClientClosed() Success!!!!!roleid[%d]connSs closed, ", s->GetRoleID() );

	return;
}


int AppGwSvc::UpdateUserFdMap( u_int roleid, int fd )
{
	MutexLockGuard lock(mutexData_);
	ROLEID_FD_MAP::iterator pos;
	
	pos = _roleID_Fd_Map.find(roleid);
	if(pos != _roleID_Fd_Map.end())
	{
		pos->second = fd;
	}
//	LOG( LOG_VERBOSE, __FILE__, __LINE__, "UpdateUserFdMap ,roldid[%d], fd[%d]", roleid, fd );
	
	return 0;
}


int AppGwSvc::InsertUserFdMap( u_int roleid, int fd )
{
	MutexLockGuard lock(mutexData_);
	
	_roleID_Fd_Map.insert(make_pair( roleid, fd ) );
//	LOG( LOG_VERBOSE, __FILE__, __LINE__, "InsertUserFdMap,roleid[%d], fd[%d]", roleid, fd );
	
	return 0;
}
	
int AppGwSvc::RemoveRoleFdMap( u_int roleid )
{
	ROLEID_FD_MAP::iterator pos;
		
	//空用户名，直接返回
	if( 0 == roleid)
		return 0;
	
	if((pos = _roleID_Fd_Map.find(roleid)) == _roleID_Fd_Map.end())
	{
		LOG( LOG_ERROR, __FILE__, __LINE__, "remove _roleID_Fd_Map,can't find roleid [%d]\n", roleid);
		return -1;
	}

	_roleID_Fd_Map.erase(pos);

	return 0;
}
	
int AppGwSvc::FindUserFd( u_int roleid )
{
	MutexLockGuard lock(mutexData_);
	ROLEID_FD_MAP::iterator pos;

	pos = _roleID_Fd_Map.find(roleid);
	if(pos == _roleID_Fd_Map.end())
		return 0;

	return pos->second;
}


ConnectSession* AppGwSvc::FindRoleConnSession( u_int roleid )
{
	
	//客户端 fd 查找
	int fd = 0;
	fd = FindUserFd( roleid );
	if( 0 == fd )
		return NULL;

	//客户端 session 查找
	return(FindFdConnSession( fd ));
}

int AppGwSvc::check_srv_scope( unsigned char ucSrvType, unsigned char ucSrvSeq )
{
	//服务器类型、序号不能为0
	if( 0 == ucSrvType || 0 == ucSrvSeq )
		return -1;

	return 0;
}
	

