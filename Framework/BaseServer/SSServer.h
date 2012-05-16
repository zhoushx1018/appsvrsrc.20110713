#ifndef SSSERVER_H
#define SSSERVER_H

#include <map>
#include <list>
#include <pthread.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include "OurDef.h"
#include "Session.h"
#include "Packet.h"
#include "IniFile.h"
#include "Log.h"
#include "DebugData.h"

typedef std::map<UInt32,Session*> SessionMap;
typedef std::list<Session*> SessionList;
//typedef std::multimap<int,Session*> SessionMultiMap;

template<typename SERVICE>
class SSServer
	:public SERVICE
{
public:
	SSServer(const char* cfg)
		:_stoped(true)
		,_thrMain(0)
		,_pfds(NULL)
		,_threads(0)
		,_thrWork(NULL)
		,_bufferSize(0)
		,_maxMsgSize(0)
		,_newSession(NULL)
		,_svcName(NULL)
		,_localPort(0)
	{
		_cfg = const_cast<char*>(cfg);

		memset(_localIP,0,sizeof(_localIP));

		pthread_mutex_init(&_pkgMutex,NULL);
		pthread_mutex_init(&_dataMutex,NULL);
		pthread_cond_init(&_pkgCond,NULL);

		_pfds = new pollfd[MAX_SESSION];

		_newSession = new Session();

		
	}

	~SSServer()
	{
		if(_pfds) delete _pfds;

		if(_thrWork) delete _thrWork;

		pthread_mutex_destroy(&_pkgMutex);
		pthread_mutex_destroy(&_dataMutex);
		pthread_cond_destroy(&_pkgCond);
		
		DataBuffer* buffer = NULL;
		SessionMap::iterator mapIter = _busySessions.begin();
		for(;mapIter!=_busySessions.end();++mapIter)
		{
			buffer = mapIter->second->GetInBuffer();
			if(buffer) delete buffer;
			
			buffer = mapIter->second->GetOutBuffer();
			if(buffer) delete buffer;

			delete mapIter->second;
		}

		if(_newSession) delete _newSession;

		SessionList::iterator listIter = _idleSessions.begin();
		for(;listIter!=_idleSessions.end();++listIter)
		{
			buffer = (*listIter)->GetInBuffer();
			if(buffer) delete buffer;
			
			buffer = (*listIter)->GetOutBuffer();
			if(buffer) delete buffer;

			delete *listIter;
		}

	}

	void SetSvcName(char* svcName)
	{
		_svcName = svcName;
	}

	char* GetConfFilePath()
	{
		return _cfg;
	}

	int Run()
	{
		if(!_stoped) return 0;

		if(this->OnInit(this))
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "OnInit error!!");
			return -1;
		}

DEBUG_PRINTF2("_svcName[%s],_cfg[%s]", _svcName, _cfg);
		if(GetConf(_svcName,_cfg))
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "GetConf error!!");
			return -1;
		}

		if(_threads<1) 
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "the count of the service threads must be greater than zero");
			return -1;
		}

		_stoped = false;

		if(Init())
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "Init error.");
			return -1;
		}

		if(Listen()) 
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "Listen error.");
			return -1;
		}


		if(pthread_create(&_thrMain,NULL,OnMain,this))
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "pthread_create error.");
			_stoped = true;
			return -1;
		}

		return 0;
	}

	bool Stoped()
	{
		return _stoped;
	}

	void Stop()
	{
		_stoped = true;
		shutdown(_socket,SHUT_RDWR);
		close(_socket);
		pthread_join(_thrMain,NULL);
	}

	void Wait()
	{
		if(!_stoped)
		{
			pthread_join(_thrMain,NULL);
		}
	}

private:
	int GetConf(const char* svcName,const char* cfg)
	{
		IniFile iniFile;
		if(!iniFile.open(cfg)) return -1;

		string value;

		if(  iniFile.read( svcName, "buffersize", value ) )return -1;
		_bufferSize = atoi(value.c_str());
	
		if(  iniFile.read( svcName, "maxmsgsize", value ) )return -1;
		_maxMsgSize = atoi(value.c_str());

		if(  iniFile.read( svcName, "ip", value ) )return -1;
		strcpy(_localIP,value.c_str());

		if(  iniFile.read( svcName, "port", value ) )return -1;
		_localPort = atoi(value.c_str());

		if(  iniFile.read( svcName, "threads", value ) )return -1;
		_threads = atoi(value.c_str());

		return 0;
	}

	static void* OnMain(void* object)
	{
		SSServer<SERVICE>* service = static_cast<SSServer<SERVICE>*>(object);

		if( NULL == service )
		{
			service->_stoped = true;
			LOG(LOG_ERROR, __FILE__, __LINE__, "service error.");
			exit(-1);
		}

		while(!service->_stoped) 
		{
			service->Poll();
		}

		return NULL;
	}

	int Init()
	{
		//----------create working thread pool.
		_thrWork = new pthread_t[_threads];
		for(int i=0;i<_threads;i++)
		{
			if(pthread_create(&_thrWork[i],NULL,OnProcess,this)) 
			{
				LOG(LOG_ERROR, __FILE__, __LINE__, "pthread_create error.");
				_stoped = true;
				return -1;
			}
		}

		return 0;
	}

	int Poll()
	{
		//---------wait for occuring event-----
		int events = poll(_pfds,_busySessions.size()+1,-1);
		
		if(events<0) 
		{
			if(errno!=EINTR)
			{
				LOG(LOG_ERROR, __FILE__, __LINE__, "poll error.");
				_stoped = true;
				return -1;
			}
		}

		//accepted
		if(_pfds[0].revents>0)
		{
			if(_pfds[0].revents&(POLLERR|POLLHUP|POLLNVAL)) //occured error.
			{
				LOG(LOG_ERROR, __FILE__, __LINE__, "poll revents error.");
				_stoped = true;
				return -1;
			}
			else if(_pfds[0].revents&POLLIN) 
			{
				Accept();
			}

			events--;
		}

		//can read data 
		Session* session = NULL;
		for(int index=1;index<=_busySessions.size()&&events>0;index++)
		{
			if(_pfds[index].revents==0) continue;

			session = FindSession(_pfds[index].fd);
			if(session==NULL)
			{
				LOG (LOG_ERROR, __FILE__, __LINE__, "the session is null");
				_stoped = true;
				return -1;
			}

			if(_pfds[index].revents&(POLLERR|POLLHUP|POLLNVAL)) //occured error.
			{
				//remove the session and move the last event to current position.
				CloseSession(session);
				index--;
			}
			else if(_pfds[index].revents&POLLIN)  //can read data.
			{
				_pfds[index].revents= 0;

				if(OnRecv(session))
				{
					index--;
				}
			}

			events--;
		}

		return 0;
	}


	int Listen()
	{
		//----------open listen socket;
		_socket = socket(AF_INET,SOCK_STREAM,0);
		if(_socket<0)  
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "socket error.");
			return -1;
		}

		int reuseFlag = 1;
		if(setsockopt(_socket,SOL_SOCKET,SO_REUSEADDR,&reuseFlag,sizeof(reuseFlag)))
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "setsockopt error.");
			return -1;
		}

		sockaddr_in _localaddr;
		_localaddr.sin_family = AF_INET;
		_localaddr.sin_port = htons(_localPort);

		inet_pton(AF_INET,_localIP,&_localaddr.sin_addr);


		if(bind(_socket,(sockaddr*)(&_localaddr),sizeof(sockaddr))<0) 
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "bind error.");
			return -1;
		}
		
		long flag = fcntl(_socket,F_GETFD,0);
		flag |= O_NONBLOCK;
		flag = fcntl(_socket,F_SETFD,flag);

		_pfds[0].fd = _socket;
		_pfds[0].events = POLLIN;
		_pfds[0].revents = 0;

		//----------listen------------
		if(listen(_socket,5))
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "listen error.");
			return -1;
		}

		return 0;
	}

	int Accept()
	{
		if(_busySessions.size()>MAX_SESSION) 
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "arrived maxsession:[%d]",MAX_SESSION);
			return -1;
		}

		//----------try accept the new session.
		unsigned int addrlen = sizeof(sockaddr);
		for(;;)
		{
			_newSession->Handle = accept(_socket,(sockaddr*)&_newSession->LocalAddress.sin_addr,&addrlen);
			
			if(_newSession->Handle<0)
			{
				if(errno==EINTR) continue;

				LOG(LOG_ERROR, __FILE__, __LINE__, "accept error.");
				return -1;
			}

			//set socket buffer
			int bufsize = 655350;
			setsockopt (_newSession->Handle, SOL_SOCKET, SO_RCVBUF, (char *) bufsize, sizeof (int));
			setsockopt (_newSession->Handle, SOL_SOCKET, SO_SNDBUF, (char *) bufsize, sizeof (int));

			// ÉèÖÃ SO_REUSEADDR
			int reuse_addr = 1;
			setsockopt (_newSession->Handle, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof reuse_addr);

			
			break;
		}

	
		InsertSession(_newSession);
		this->OnConnected(*_newSession);
		
		return 0;
	}
	
	int OnRecv(Session* session)
	{
		DataBuffer* buffer = session->GetInBuffer();
		unsigned short length = session->PacketLength;
		unsigned int datasize = buffer->GetDataSize();
		
		int left = length==0?(PACKET_HEADER_LENGTH-datasize):(length-datasize);

		int bytes = recv(session->Handle,buffer->GetWritePtr(),left,0);
		
		if(bytes>0)
		{
			buffer->MoveWritePtr(bytes);
			datasize += bytes;

			if(length==0&&datasize>=PACKET_HEADER_LENGTH)
			{
				memcpy(&length,buffer->GetReadPtr(),sizeof(length));
				if(length<PACKET_HEADER_LENGTH||length>_maxMsgSize)
				{
					CloseSession(session);
					LOG (LOG_ERROR, __FILE__, __LINE__
						, "the packet's length error:length[%d]._maxMsgSize[%d]",length,_maxMsgSize);
					return -1;
				}

				session->PacketLength = length;
				left = length - datasize;

				if(left>0)
				{
					bytes = recv(session->Handle,buffer->GetWritePtr(),left,0);

					if(bytes>0)
					{
						buffer->MoveWritePtr(bytes);
						datasize += bytes;
					}
					else
					{
						if(bytes<0&&errno==EAGAIN)
						{
							return 0;
						}

						CloseSession(session);
						LOG(LOG_ERROR, __FILE__, __LINE__, "recv error.");
						return -1;
					}
				}
			}

			if(datasize==length)
			{
				//push into queue.
				Packet* packet = new Packet(length);
				memcpy(packet->GetBuffer()->GetWritePtr(),buffer->GetReadPtr(),length);
				packet->GetBuffer()->MoveWritePtr(length);

				packet->UnpackHeader();

				PushPacket(session,packet);
				
				session->PacketLength = 0;
				buffer->Reset();
			}
		}
		else
		{
			if(bytes<0&&errno==EAGAIN)
			{
				return 0;
			}

			CloseSession(session);
			
			if(bytes==0)
			{
				LOG (LOG_DEBUG, __FILE__, __LINE__, "the peer has performed an orderly shutdown");
			}
			else
			{
				LOG(LOG_ERROR, __FILE__, __LINE__, "recv error.");
			}

			return -1;
		}

		return 0;
	}

	static void* OnProcess(void* object)
	{
		SSServer<SERVICE>* service = static_cast<SSServer<SERVICE>*>(object);
		while(!service->_stoped)
		{
			PacketQueue::value_type value = service->PopPacket();
			Session* session = value.first;
			Packet* packet = value.second;
			
			//do work.
			switch(packet->MsgType)
			{
				default:
					service->OnProcessPacket(*session,*packet);
			}
		

			//finished work
			delete packet;
		}
	}

	void PushPacket(Session* session,Packet* packet)
	{
		pthread_mutex_lock(&_pkgMutex);
		_packets.push(PacketQueue::value_type(session,packet));
		pthread_mutex_unlock(&_pkgMutex);
		pthread_cond_signal(&_pkgCond);
	}

	PacketQueue::value_type PopPacket()
	{
		PacketQueue::value_type value;
		pthread_mutex_lock(&_pkgMutex);
		
		while(_packets.size()==0)
		{
			pthread_cond_wait(&_pkgCond,&_pkgMutex);
		}

		value = _packets.front();
		_packets.pop();

		pthread_mutex_unlock(&_pkgMutex);
		
		return value;
	}

	void CloseSession(Session* session)
	{
		session->Shutdown();
		RemoveSession(session);
	}
	
	void RemoveSession(Session* session)
	{
		pthread_mutex_lock(&_dataMutex);
	
		if(_busySessions.size()>session->EventIndex)
		{
			memcpy(&_pfds[session->EventIndex],&_pfds[_busySessions.size()],sizeof(pollfd));
			Session* s = FindSession(_pfds[_busySessions.size()].fd);
			if(s==NULL)
			{
				LOG (LOG_ERROR, __FILE__, __LINE__, "the session is null");
				_stoped = true;
			}
			s->EventIndex = session->EventIndex;
		}

		_busySessions.erase(session->Handle);
		_idleSessions.push_back(session);

		pthread_mutex_unlock(&_dataMutex);

	}

	void InsertSession(Session* session)
	{
		pthread_mutex_lock(&_dataMutex);
	
		int index = _busySessions.size() + 1;

		long flag = fcntl(session->Handle,F_GETFD);
		flag |= O_NONBLOCK;
		fcntl(session->Handle,F_SETFD,flag);

		_pfds[index].fd = session->Handle;
		_pfds[index].events = POLLIN;
		_pfds[index].revents = 0;

		session->Connected = true;
		session->EventIndex = index;

		session->SetInBuffer(new DataBuffer(_maxMsgSize));

		session->SetOutBuffer(new DataBuffer(_maxMsgSize));

		_busySessions[session->Handle] = session;

		if(_idleSessions.size()>0)
		{
			_newSession = _idleSessions.front();
			_idleSessions.pop_front();
		}
		else
		{
			_newSession = new Session();
		}

		pthread_mutex_unlock(&_dataMutex);
	}

	Session* FindSession(int fd)
	{
		SessionMap::iterator iter = _busySessions.find(fd);

		if(iter==_busySessions.end()) return NULL;
		
		return iter->second;
	}

private:
	bool _stoped;

	//Á¬½Ó session
	Session* _newSession;
	SessionMap _busySessions;
	SessionList _idleSessions;

	int _socket;
	
	pollfd* _pfds;
	
	int _needconns;
	
	time_t _lastconn;
	
	pthread_t _thrMain;
	pthread_t* _thrWork;
	
	int _threads;
	
	pthread_mutex_t _pkgMutex;

	pthread_mutex_t _dataMutex;
	
	pthread_cond_t _pkgCond;
	
	PacketQueue _packets;


	unsigned int _bufferSize;

	unsigned int _maxMsgSize;


	char* _svcName;

	char* _cfg;

	char _localIP[16];
	
	unsigned short _localPort;
};
#endif


