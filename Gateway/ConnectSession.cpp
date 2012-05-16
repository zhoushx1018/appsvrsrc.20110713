#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "ConnectSession.h"
#include "Log.h"
#include "PkgHead.h"
#include "DebugData.h"

ConnectSession::~ConnectSession ()
{
	if( NULL != _recvBuff )
	{
		delete[] _recvBuff;
		_recvBuff = NULL;
	}
}

ConnectSession::ConnectSession(long long llInputConId, char cInputConnType )
{
	_connID = llInputConId;
	_connType = cInputConnType;
	_stamp = time (NULL);
	_loginStatus = 0;
	_srvType = 0;
	_srvSeq = 0;
	_isSameUser = 0;
	_closeFlag = 0;
	_closedNum = 0;
	_roleID = 0;
	_trySendNum = 0;
	
	//缓存
	_recvBuff = NULL;
	_recvBuff = new char[MAXSOCKBUFF+1];
	memset( _recvBuff, 0, MAXSOCKBUFF+1 );
	_recvLen = 0;
	
	//ip port fd
	in_addr adrtTmp;
	adrtTmp.s_addr = CONNECTION_IP(_connID);
	sprintf( _ip, "%s", inet_ntoa( adrtTmp ) );

	_port = ntohs(CONNECTION_PORT(_connID));
	_fd = CONNECTION_FD(_connID);
	
	
}

Byte ConnectSession::GetConnType()
{
	return _connType;
}

long long ConnectSession::GetConnId()
{
	return _connID;
}

u_int ConnectSession::GetStamp()
{
	return _stamp;
}

Byte ConnectSession::GetLoginStatus()
{
	return _loginStatus;
}

u_int ConnectSession::GetRoleID()
{
	return _roleID;
}

Byte ConnectSession::GetIsSameUser()
{
	return _isSameUser;
}

Byte ConnectSession::GetSrvType()
{
	return _srvType;
}

Byte ConnectSession::GetSrvSeq()
{
	return _srvSeq;
}

Byte ConnectSession::GetCloseFlag()
{
	MutexLockGuard lock(mutexData_);
	return _closeFlag;
}


Byte ConnectSession::GetClosedNum()
{
	MutexLockGuard lock(mutexData_);
	return _closedNum;
}

char * ConnectSession::GetIP()
{
	return _ip;
}

int ConnectSession::GetPort()
{
	return _port;
}
int ConnectSession::GetFd()
{
	return CONNECTION_FD(_connID);
}




void ConnectSession::SetConnType( Byte ucInput )
{
	MutexLockGuard lock(mutexData_);
	_connType = ucInput;
}

void ConnectSession::SetConnId( long long llInput )
{
	MutexLockGuard lock(mutexData_);
	_connID = llInput;
}

void ConnectSession::SetStamp()
{
	_stamp = time(NULL);
}

void ConnectSession::SetLoginStatus( Byte ucInput )
{
	MutexLockGuard lock(mutexData_);
	_loginStatus = ucInput;
}

void ConnectSession::SetRoleID( u_int input )
{
	MutexLockGuard lock(mutexData_);
	_roleID = input;
}

void ConnectSession::SetIsSameUser( Byte ucInput )
{
	MutexLockGuard lock(mutexData_);
	_isSameUser = ucInput;
}

void ConnectSession::SetSrvType( Byte ucInput )
{
	MutexLockGuard lock(mutexData_);
	_srvType = ucInput;
}

void ConnectSession::SetSrvSeq( Byte ucInput )
{
	MutexLockGuard lock(mutexData_);
	_srvSeq = ucInput;
}

void ConnectSession::CloseConn()
{
	MutexLockGuard lock(mutexData_);
	
	CloseConnNoLock();
}


void ConnectSession::CloseConnNoLock()
{
	//关闭连接
	if( 0 == _closeFlag )
	{
		if(_fd >0 )
		{
			if( INSIDE_TCP_STREAM == _connType )
				LOG ( LOG_ERROR, __FILE__, __LINE__, "CloseConnNoLock(): shutdown _fd[%d], _connType[inside], _srvType[%d],_srvSeq[%d]", _fd, _srvType, _srvSeq );
			else
				LOG ( LOG_ERROR, __FILE__, __LINE__, "CloseConnNoLock(): shutdown _fd[%d], _connType[outside], _roleID[%d]", _fd, _roleID );
			
			shutdown( _fd, SHUT_RDWR );
			close(_fd);
			_fd = -1;
		}
	}

	//设置关闭标志
	_closeFlag = 1;

	//设置关闭标志的次数累计
	if( 0 == _closedNum || 1 == _closedNum )
		_closedNum++;

//	LOG ( LOG_ERROR, __FILE__, __LINE__, "CloseConnNoLock ! _fd[%d], _connType[%d], _roleID[%d], _closedNum[%d]", _fd, _connType, _roleID, _closedNum );
}



int ConnectSession::RecvMsg()
{
	int sockfd = _fd;
	
	MutexLockGuard lock(mutexData_);

//LOG ( LOG_VERBOSE, __FILE__, __LINE__, "RecvMsg, connIP[%s],connPort[%d], fd=[%d], srvType[%d], srvSeq[%d]", _ip, _port, sockfd, _srvType, _srvSeq );

	//连接已关闭
	if( _closeFlag )
	{
		LOG ( LOG_ERROR, __FILE__, __LINE__, "session conn is closed... fd[%d],  ", GetFd() );
		return -1;
	}
	
	int recv_bytes = recv (sockfd, _recvBuff + _recvLen, MAXSOCKBUFF - _recvLen, 0);

//if( recv_bytes <= 0 )
//	LOG( LOG_VERBOSE, __FILE__, __LINE__, "recv_bytes[%d], fd[%d],errno[%d], strerror[%s]", recv_bytes, sockfd, errno, strerror(errno) );

	if (recv_bytes == -1)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return 0;
		else
		{
			CloseConnNoLock();
			return -1;
		}
	}
	else if (recv_bytes ==	0)
	{
		LOG ( LOG_VERBOSE, __FILE__, __LINE__, "recv 0 Byte!!peer close Conn!! connIP[%s],connPort[%d], fd=[%d], errmsg[%s], _recvLen[%d]", _ip, _port, sockfd, strerror(errno), _recvLen );
		CloseConnNoLock();
		return -1;
	}

	_recvLen += recv_bytes;	
	_stamp = time (NULL);
	
	if( _connType == INSIDE_TCP_STREAM )
		LOG ( LOG_VERBOSE, __FILE__, __LINE__, "RecvMsg<----Server success ! srvType[%d], srvSeq[%d], connIP[%s],connPort[%d], recv fd=[%d],_recvLen=[%d],session_len=[%d]", _srvType, _srvSeq, _ip, _port, sockfd, recv_bytes, _recvLen );
	else
		LOG ( LOG_VERBOSE, __FILE__, __LINE__, "RecvMsg<----Client success ! roleID[%d], connIP[%s],connPort[%d], recv fd=[%d],_recvLen=[%d],session_len=[%d]", _roleID, _ip, _port, sockfd, recv_bytes, _recvLen );

	return recv_bytes;
}

//向本连接session，发送全部数据
//参数
//	buffer			数据包指针
//	buflen			数据包长度
//返回值
//	0					成功
//	非0			 	失败
//						其中  -1 表示该 session被设置关闭标志 1次
//						其中  -2 表示该 session被设置关闭标志 2次及以上
int ConnectSession::SendMsg( const char *buffer, int buflen)
{
	int tmp;
	int total = buflen;
	const char *p = buffer;
	int sockfd = _fd;

	MutexLockGuard lock(mutexData_);

	//连接已关闭
	if( _closeFlag )
	{
		if(_closedNum < 2 )
		{
			LOG ( LOG_ERROR, __FILE__, __LINE__, "fd[%d], session conn closed! _connType[%d],_roleID[%d], _closeFlag[%d], _closedNum[%d], _loginStatus[%d] ",
				GetFd(), _connType, _roleID, _closeFlag, _closedNum, _loginStatus );		
			return -1;
		}
		else
		{
			return -2;
		}
	}

	while(1)
	{
		tmp = send(sockfd, p, total, 0);
		if(tmp < 0)
		{
			// 当socket是非阻塞时,如返回EAGAIN,表示写缓冲队列已满
			// 返回 EINTR 表示中断，可继续写
			// 在这里做延时后再重试
			if( errno == EAGAIN || errno == EINTR )
			{
				_trySendNum++;
				usleep(1000);

				//没有超过最大尝试发送次数,则继续发送
				if( _trySendNum<= SESSION_TRYSENDNUM )
					continue;
			}

			_trySendNum = 0;
			LOG ( LOG_ERROR, __FILE__, __LINE__, "socket send error, errno[%d],errormsg[%s] ", errno, strerror(errno) );
			CloseConnNoLock();
			return -1;
		}

		if(tmp == total)
		{
			_trySendNum = 0;
			
			if( _connType == INSIDE_TCP_STREAM )
				LOG ( LOG_VERBOSE, __FILE__, __LINE__, "SenMsg---->Server success! srvType[%d],srvSeq[%d], connIP[%s],connPort[%d], send fd=[%d],sendLen=[%d]", _srvType, _srvSeq, _ip, _port, sockfd, total );
			else			
				LOG ( LOG_VERBOSE, __FILE__, __LINE__, "SenMsg---->Client success! roleID[%d], connIP[%s],connPort[%d], send fd=[%d],sendLen=[%d]", _roleID, _ip, _port, sockfd, total );
				
			return buflen;
		}

		total -= tmp;
		p += tmp;
	}
	
	return 0;

}


//获取缓存数据包
//参数
//	ptrData			返回的数据包指针
//	iLen				返回的数据包长度
//返回值
//	大于0			成功
//	0					数据包暂无
//	小于0			缓存包数据错误，应关闭连接
//
int ConnectSession::GetDataPack( char * ptrData, int &iLen )
{
	MutexLockGuard lock(mutexData_);
	
	if( INSIDE_TCP_STREAM == _connType ||
			OUTSIDE_TCP_STREAM == _connType	)
	{
		//获取内网数据包
		return( CheckDataPack( ptrData, iLen ) );
	}
	else
	{
		//未知类型包
		return -1;
	}

	return 1;
}

//包头校验
//参数
//	ptrData			返回的数据包指针
//	iLen				返回的数据包长度
//返回值
//	大于0			成功
//	0					数据包暂无
//	小于0			缓存包数据错误，应关闭连接
//
int ConnectSession::CheckDataPack(char * ptrData, int &iLen)
{
	PkgHead pkgHead;

	//缓存数据中是否有包头
	if( _recvLen < (int)sizeof(PkgHead) )
		return 0;
	
	//获取包头数据
	pkgHead.unpacket( _recvBuff );
	
	//包头字段校验
	if( pkgHead.usPkgLen < sizeof(PkgHead) ||  pkgHead.usPkgLen > (MAXSOCKBUFF < (0xffff-1)?MAXSOCKBUFF:(0xffff-1)) )
	{
		LOG( LOG_ERROR, __FILE__, __LINE__, "CheckDataPack , usPkgLen error" );
		return -1;
	}
	
	//缓存数据中包头包体是否完整
	if( pkgHead.usPkgLen > _recvLen )
		return 0;
	
	//数据包拷贝
	memcpy( ptrData, _recvBuff, pkgHead.usPkgLen );
	iLen = pkgHead.usPkgLen;
	
	//缓存去除数据包
	_recvLen = _recvLen - pkgHead.usPkgLen;
	memmove( _recvBuff, _recvBuff+pkgHead.usPkgLen, _recvLen );
	
	return 1;
}


//_recvBuff 缓存初始是否错误
//参数
//返回值
//	0					初始化成功，没有错误
//	非0				初始化失败
//
int ConnectSession::BuffInitCheck()
{
	if( _recvBuff == NULL )
		return -1;
	else
		return 0;
}

