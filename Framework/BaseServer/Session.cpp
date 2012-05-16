#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "Session.h"

Session::Session()
	:Handle(-1)
	,PacketLength(0)
	,Connected(false)
	,_inBuffer(NULL)
	,_outBuffer(NULL)
{
	pthread_mutex_init(&_inMutex,NULL);
	pthread_mutex_init(&_outMutex,NULL);
	pthread_mutex_init(&_sendMutex,NULL);
}

Session::Session(const Session& session)
{
	memcpy(this,&session,sizeof(Session));

	pthread_mutex_init(&_inMutex,NULL);
	pthread_mutex_init(&_outMutex,NULL);
	pthread_mutex_init(&_sendMutex,NULL);
}

Session::~Session()
{
	pthread_mutex_destroy(&_inMutex);
	pthread_mutex_destroy(&_outMutex);
	pthread_mutex_destroy(&_sendMutex);
}

DataBuffer* Session::GetInBuffer(bool lock)
{
	if(lock)
		pthread_mutex_lock(&_inMutex);

	return _inBuffer;
}

void Session::ReleaseInBuffer()
{
	pthread_mutex_unlock(&_inMutex);
}

void Session::SetInBuffer(DataBuffer* buffer)
{
	_inBuffer = buffer;
}

DataBuffer* Session::GetOutBuffer(bool lock)
{
	if(lock)
		pthread_mutex_lock(&_outMutex);

	return _outBuffer;
}

void Session::ReleaseOutBuffer()
{
	pthread_mutex_unlock(&_outMutex);
}

void Session::SetOutBuffer(DataBuffer* buffer)
{
	_outBuffer = buffer;
}

int Session::Send(DataBuffer* buffer,bool lock)
{
	int error = 0;

	if(lock)
		pthread_mutex_lock(&_sendMutex);
	
	error = SendProc(buffer);

	if(lock)
		pthread_mutex_unlock(&_sendMutex);

	return error;
}

int Session::SendProc(DataBuffer* buffer)
{
	int size = buffer->GetDataSize();
	int bytes = 0;

	for(;;)
	{
		bytes = send(Handle,buffer->GetReadPtr(),size,0);
		if(bytes==size) return 0;

		if(bytes>=0)
		{
			size -= bytes;
			buffer->MoveReadPtr(bytes);
		}
		else
		{
			if(errno==EWOULDBLOCK
				||errno==EAGAIN
				||errno==EINTR)
			{
				if(WaitSend()<0) return -1;

				continue;
			}

			return -1;
		}
	}

	return 0;
}

int Session::WaitSend()
{
	_pfd.fd = Handle;
	_pfd.events = POLLOUT;
	_pfd.revents = 0;

	while(poll(&_pfd, 1, -1))
	{
		if(errno!=EINTR) return -1;
	}

	return 0;
}

int Session::Open()
{
	Handle = socket(AF_INET,SOCK_STREAM,0);
	if(Handle<0) 
	{
		return -1;
	}

	if(bind(Handle,(sockaddr*)(&LocalAddress),sizeof(sockaddr))<0) return -1;
	
	long flag = fcntl(Handle,F_GETFD);
	if(flag<0) return -1;
	flag |= O_NONBLOCK;
	flag = fcntl(Handle,F_SETFD,flag);
	if(flag<0) return -1;

	return 0;
}

void Session::Shutdown()
{
	shutdown(Handle,SHUT_RDWR);
	close(Handle);
	Connected = false;
	GetInBuffer(true)->Reset();
	ReleaseInBuffer();
	GetOutBuffer(true)->Reset();
	ReleaseOutBuffer();
}

void* Session::Service()
{
	return _service;
}

void Session::Service(void* service)
{
	_service = service;
}



