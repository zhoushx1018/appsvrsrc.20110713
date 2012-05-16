#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "Log.h"
#include "SSClient.h"
#include "Packet.h"
#include "DataBuffer.h"
#include "DebugData.h"



SSClient::SSClient( string inIp, UInt16 inPort )
:_sockFd(-1), _ip(inIp), _port(inPort)
{
}


SSClient::~SSClient()
{
	Close();
}


void SSClient::Close()
{
	if(_sockFd > 0 )
	{
		close(_sockFd);
	}
	_sockFd = -1;
}

int SSClient::Connect()
{

	_sockFd = socket( AF_INET, SOCK_STREAM, 0 );
	if( _sockFd < 0 )
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "socket error,errno[%d],errmsg[%s]", errno, strerror(errno));
		return -1;
	}

	struct sockaddr_in address;
	memset( &address, 0, sizeof(address) );
	address.sin_family = AF_INET;
	address.sin_port = htons(_port);
	inet_pton( AF_INET, _ip.c_str(), &address.sin_addr);

	int iRet = connect( _sockFd, (sockaddr*)&address, sizeof(address) );
	if(iRet)
	{
		Close();
		LOG (LOG_ERROR, __FILE__, __LINE__, "connect error,errno[%d],errmsg[%s]", errno, strerror(errno));
		return -1;
	}

	return 0;
}

// socket发送
//param	sendBuff	发送缓存
//param iSendLen  发送字节数
//return	<0 出错   >=0  成功发送的字节数

int SSClient::SockSend( void * sendBuff, int iSendLen )
{
	if( iSendLen <= 0 )
		return 0;

	if( _sockFd < 0 )
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "invalid _sockFd[%d]", _sockFd);
		return -1;
	}
	
	int iLeft = iSendLen;
	char * curr = (char*)sendBuff;
	while(iLeft > 0)
	{
		int iSended = send( _sockFd, curr, iLeft, 0 );
		if(iSended< 0)
		{
			if( EINTR == errno || EWOULDBLOCK == errno )
			{
				usleep(1000);
				continue;
			}

			LOG (LOG_ERROR, __FILE__, __LINE__, "send error, errno[%d],errmsg[%s]", errno, strerror (errno));
			return -1;
		}

		iLeft -= iSended;
		curr += iSended;
	}

	return (iSendLen - iLeft);
}


//sock 接收
//param	recvBuff	接收缓存
//param iRecvLen  接收字节数
//return	<0 出错   >=0  成功接收的字节数

int SSClient::SockRecv( void * recvBuff, int iRecvLen )
{
	int  iLeft, iRead;
	char * curr;

	curr = (char*)recvBuff;
	iLeft = iRecvLen;

	if(_sockFd < 0)
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "invalid _sockFd[%d]", _sockFd);
		return -1;
	}
	
	while( iLeft > 0 )
	{
		if( (iRead = recv( _sockFd, curr , iLeft, 0) ) < 0 )
		{
			if( errno == EINTR || errno == EWOULDBLOCK)
				iRead=0;
			else
			{
				LOG (LOG_ERROR, __FILE__, __LINE__, "recv error, errno[%d],errmsg[%s]", errno, strerror (errno));
				return -1;
			}
		}
		else if ( iRead == 0 )
			break;
		iLeft -= iRead;
		curr += iRead;
	}
	return ( iRecvLen-iLeft );

}



//消息发送
//return 0 成功  非0 失败

int SSClient::SendMsg( Packet & packet )
{

	DataBuffer * buff = packet.GetBuffer();
	int iRet = SockSend( buff->GetReadPtr(), buff->GetDataSize());
	if(iRet < 0)
		return -1;
		
	return 0;
}

//消息接收
//return 0 成功  非0 失败

int SSClient::RecvMsg( Packet & packet)
{
	DataBuffer * buff = packet.GetBuffer();

	//消息头接收
	int iRecv = SockRecv( buff->GetWritePtr(), PACKET_HEADER_LENGTH);
	if( iRecv < 0 || iRecv != PACKET_HEADER_LENGTH )
		return -1;

	//移动 写指针
	buff->MoveWritePtr(iRecv);

	//解包头
	packet.UnpackHeader();

	//包体长度
	int iLeft = packet.Length- PACKET_HEADER_LENGTH;
	if( 0 == iLeft )
		return 0;

	//消息体接收
	iRecv = SockRecv( buff->GetWritePtr(), iLeft );
	if( iRecv != iLeft )
		return -1;

	//移动 写指针 
	buff->MoveWritePtr(iRecv);

	return 0;
}

//S_S 请求及应答
int SSClient::SSRequest( Packet &packet)
{
	int iRet = 0;

	//连接
	iRet = Connect();
	if(iRet<0)
		goto EndOfProcess;


DEBUG_PRINTF( "SSRequest: req pkg------- \n" );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr(), packet.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );

	//发送消息
	iRet = SendMsg(packet);
	if(iRet<0)
		goto EndOfProcess;

	//缓存清空
	packet.GetBuffer()->Reset();

	//接收应答
	iRet = RecvMsg(packet);
	if(iRet<0)
		goto EndOfProcess;

DEBUG_PRINTF( "SSRequest: ack pkg------- \n" );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH , 0, __FILE__, __LINE__ );
		
EndOfProcess:

	Close();
	return iRet;
}

