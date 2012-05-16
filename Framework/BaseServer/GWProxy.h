//网关代理类

#ifndef GWPROXY_H
#define GWPROXY_H

#include <map>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include "List.h"
#include "Packet.h"
#include "IniFile.h"
#include "Log.h"
#include "Session.h"
#include <errno.h>
#include <string.h>
#include "OurDef.h"

class Gateway:public Session
{
	public:
		unsigned short ID;
		bool Registered;
};

typedef std::list<Gateway> GWList;

template<class CService>
class GWProxy:public CService
{
public:
	GWProxy(const char* cfg)
		:_stoped(true)
		,_pfds(NULL)
		,_thrWork(NULL)
		,_threads(0)
		,_svrType(0)
		,_svrSeq(0)
		,_bufferSize(0)
		,_maxMsgSize(0)
		,_totalGW(0)
		,_svcName(NULL)
		,_pollsize(0)
		,_confFilePath(NULL)
		
	{
		_confFilePath = new char[strlen(cfg)+1];
		strcpy(_confFilePath,cfg);

		memset(_localIP,0,sizeof(_localIP));
		memset(&_localaddr,0,sizeof(_localaddr));

		pthread_mutex_init(&_pkgMutex,NULL);
		pthread_cond_init(&_pkgCond,NULL);
	}

	~GWProxy()
	{
		if(_confFilePath) delete _confFilePath;

		if(_pfds) delete _pfds;

		if(_thrWork) delete _thrWork;

		pthread_mutex_destroy(&_pkgMutex);
		pthread_cond_destroy(&_pkgCond);
		
		DataBuffer* buffer = NULL;
		GWList::iterator iter = _gateways.begin();
		for(;iter!=_gateways.end();++iter)
		{
			buffer = iter->GetInBuffer();
			if(buffer) delete buffer;
			
			buffer = iter->GetOutBuffer();
			if(buffer) delete buffer;
		}
	}

	void SetSvcName(char* svcName)
	{
		_svcName = svcName;
	}


	int Run()
	{
		if(!_stoped) return 0;

		if(this->OnInit(this))
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "OnInit() error" );
			exit(-1);
		}

		if(GetConf(_svcName,_confFilePath))
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "Config() error" );
			exit(-1);
		}

		if(_threads<1) 
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "count of the service threads must be greater than zero");
			exit(-1);
		}

		_localaddr.sin_family = AF_INET;
		_localaddr.sin_port = 0;

		if(inet_pton(AF_INET,_localIP,&_localaddr.sin_addr)==0)
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "inet_pton() error");
			exit(-1);
		}

		if(_gateways.size()==0) 
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, " gateways.size error" );
			exit(-1);
		}

		_stoped = false;

		if(pthread_create(&_thrMain,NULL,OnMain,this))
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "create threads occurr error.");
			_stoped = true;
			exit(-1);
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

		this->OnStop();
	}

	void Wait()
	{
		pthread_join(_thrMain,NULL);

		Stop();
	}

	int AddGateway(const char* ip,unsigned short port)
	{
		if(!_stoped) return -1;

		Gateway gw ;
		
		gw.ID = 1;
		gw.RemoteAddress.sin_family = AF_INET;
		gw.RemoteAddress.sin_port = htons(port);
		
		if(inet_pton(AF_INET,ip,&gw.RemoteAddress.sin_addr)==0)
		{
			LOG(LOG_ERROR, __FILE__, __LINE__, "inet_pton  error " );
			return -1;
		}

		gw.SetInBuffer(new DataBuffer(_bufferSize));
		gw.SetOutBuffer(new DataBuffer(_bufferSize));

		gw.Service(this);

		_gateways.push_back(gw);

		return 0;
	}

	int Broadcast(const std::list<UInt32>& lrid,DataBuffer* buffer)
	{
		unsigned int dataSize = buffer->GetDataSize();

DEBUG_PRINTF1( "Broadcast, list size[%d] ,pkg all: \n", lrid.size() );
DEBUG_SHOWHEX( buffer->GetReadPtr(), buffer->GetDataSize(), 0, __FILE__, __LINE__ );
	
		Gateway* session = NULL;
		std::list<UInt32>::const_iterator iter = lrid.begin();
		for(;iter!=lrid.end();++iter)
		{
			session = FindSessionByID(1);
			if(session)
			{
DEBUG_PRINTF1( "(*iter)[%d] \n",(*iter) );
				memcpy(buffer->GetReadPtr()+11,&(*iter), 4 );
				if(session->Send(buffer))
				{
					LOG(LOG_ERROR, __FILE__, __LINE__, "session->Send() error,errno[%d],strerror[%s]", errno, strerror(errno) );
					return -1;
				}
				buffer->MoveReadPtr(-dataSize);
			}
			else
			{
				LOG(LOG_ERROR, __FILE__, __LINE__, "not found session");
			}
		}

		return 0;
	}

	int Broadcast(const List<UInt32>& lrid,DataBuffer* buffer)
	{
		unsigned int dataSize = buffer->GetDataSize();
	
		Gateway* session = NULL;
		List<UInt32>::const_iterator iter = lrid.begin();
		for(;iter!=lrid.end();++iter)
		{
			session = FindSessionByID(1);
			if(session)
			{
				memcpy(buffer->GetReadPtr()+11, &(*iter), 4 );
				if(session->Send(buffer))
				{
					LOG(LOG_ERROR, __FILE__, __LINE__, "session->Send() error,errno[%d],strerror[%s]", errno, strerror(errno) );
					return -1;
				}
				buffer->MoveReadPtr(-dataSize);
			}
			else
			{
				LOG(LOG_ERROR, __FILE__, __LINE__, "not found session");
			}
		}

		return 0;
	}

	//close the client.
	int CloseClient(UInt32 roleID)
	{
		Gateway* session = FindSessionByID(1);
		if(!session->Connected)
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "session not Connected  " );
			return -1;
		}

		if(!session->Registered)
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "session not registed" );
			return -1;
		}

		Packet packet(PACKET_HEADER_LENGTH);

		packet.Length = PACKET_HEADER_LENGTH;
		packet.Direction = DIRECT_S_S_REQ;
		packet.SvrType = 0;
		packet.SvrSeq = 0;
		packet.MsgType = MSGTYPE_GW_CLOSECLIENT;
		packet.UniqID = 0;
		packet.RoleID= roleID;
		
		packet.UpdatePacketLength();

		if(session->Send(packet.GetBuffer()))
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"session send error");
			return -1;
		}

		return 0;
	}

	Session* GetSession(const string& userId)
	{
		return FindSessionByID(1);
	}


	char* GetConfFilePath()
	{
		return _confFilePath;
	}

	unsigned char SvcType() const
	{
		return _svrType;
	}

	unsigned char SvcSeq() const
	{
		return _svrSeq;
	}

private:
	//获取配置参数
	int GetConf(const char* svcName,const char* cfg)
	{
		IniFile _file;
		if(!_file.open(cfg)) return -1;

		string value;

		if(  _file.read( svcName, "buffersize", value ) )return -1;
		_bufferSize = atoi(value.c_str());

		if(  _file.read( svcName, "maxmsgsize", value ) )return -1;
		_maxMsgSize = atoi(value.c_str());

		if(  _file.read( "gw", "count", value ) )return -1;
		_totalGW = atoi(value.c_str());

		char index[21]={0};
		string ip,port;
		for(int i=0;i<_totalGW;i++)
		{
			sprintf(index,"gw_%d",i);
			
			 if( _file.read( index, "ip", ip ) )return -1;
		
			 if( _file.read( index, "port", port ) )return -1;

			if(AddGateway(ip.c_str(),atoi(port.c_str()))) return -1;
		}
		
		if(  _file.read( svcName, "svrType", value ) )return -1;
		_svrType = atoi(value.c_str());
		
		if(  _file.read( svcName, "svrSeq", value ) )return -1;
		_svrSeq = atoi(value.c_str());

		if(  _file.read( svcName, "ip", value ) )return -1;
		strcpy(_localIP,value.c_str());

		if(  _file.read( svcName, "threads", value ) )return -1;
		_threads = atoi(value.c_str());

		return 0;
	}

	static void* OnMain(void* object)
	{
		GWProxy<CService>* service = static_cast<GWProxy<CService>*>(object);

		if(service==NULL) 
		{
			service->_stoped = true;
			return NULL;
		}

		//----init
		if(service->Init()) 
		{
			return NULL;
		}

		//----------try read data and connect.
		
		while(!service->_stoped) 
		{
			if(service->_needconns>0)
			{
				service->Connect();
			}

			//wait for reading data and connecting to remote host.
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
				LOG (LOG_ERROR, __FILE__, __LINE__, "pthread_create error" );
				_stoped = true;
				return -1;
			}
		}

		_lastconn = time(NULL) -4;
		_needconns = _gateways.size();
		_pfds = new pollfd[_needconns];

		GWList::iterator iter = _gateways.begin();
		for(int index = 0;iter!=_gateways.end();++iter,index++)
		{
			memcpy(&iter->LocalAddress,&_localaddr,sizeof(sockaddr));

			if(iter->Open()<0)
			{
				LOG (LOG_ERROR, __FILE__, __LINE__, "gw open error" );
				return -1;
			}
		}

		return 0;
	}

	int Connect()
	{
 		time_t now = time(NULL);

		if( now - _lastconn >= 1 )
		{
			_lastconn = now;

			GWList::iterator iter = _gateways.begin();
			for(;iter!=_gateways.end();++iter)
			{
				if(!iter->Connected)
				{
					int error = connect(iter->Handle,(sockaddr*)&iter->RemoteAddress,sizeof(sockaddr));
					if(error==0) //connected ,can read data.
					{
						iter->EventIndex = _pollsize;
						_pfds[_pollsize].fd = iter->Handle;
						_pfds[_pollsize].events = POLLIN;
						_pfds[_pollsize].revents = 0;
						_pollsize++;

						_needconns--;

						iter->Connected = true;
						iter->Registered = false;
						
						this->OnConnected(*iter);

						Register(&(*iter));
					}
					else
					{
 						LOG (LOG_ERROR, __FILE__, __LINE__, "errno[%d] strerror[%s]", errno, strerror(errno) );
 						return -1;
 					}
				}
			}
		}

		return 0;
	}
	
	int Poll()
	{
		if(_pollsize==0) 
		{
			sleep(5);
			return 0;
		}

		int events = poll(_pfds,_pollsize,-1);
			
		if(events<0) 
		{
			if(errno!=EINTR)
			{
				LOG (LOG_ERROR, __FILE__, __LINE__, "errno[%d] strerror[%s]", errno, strerror(errno));
				_stoped = true;
				return -1;
			}
		}

		//can read data or connected.
		Gateway* session = NULL;
		for(int index=0;index<_pollsize&&events>0;index++)
		{
			if(_pfds[index].revents==0) continue;

			session = FindSession(_pfds[index].fd);
			if(session==NULL)
			{
				_stoped = true;
				LOG (LOG_ERROR, __FILE__, __LINE__, "session is null");
				return -1;
			}

			if(_pfds[index].revents&(POLLERR|POLLHUP|POLLNVAL)) //occured error.
			{
				_pfds[index].revents= 0;

				CloseSession(session);

				LOG (LOG_ERROR, __FILE__, __LINE__, "session is disconnected.");
				
				index--;
			}
			else if(_pfds[index].revents&POLLIN) //can read data.
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
						LOG (LOG_ERROR, __FILE__, __LINE__, "recv error, errno[%d], strerror[%s]", errno, strerror(errno));
						return -1;
					}
				}
			}

			if(datasize==length)
			{
				//读取数据
				Packet* packet = new Packet(length);
				memcpy(packet->GetBuffer()->GetWritePtr(),buffer->GetReadPtr(),length);
				packet->GetBuffer()->MoveWritePtr(length);

				//包头解包
				packet->UnpackHeader();

				//加入消息队列
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
				LOG (LOG_ERROR, __FILE__, __LINE__, "the peer has performed an orderly shutdown");
			}
			else
			{
				LOG (LOG_ERROR, __FILE__, __LINE__, "recv error, errno[%d], strerror[%s]", errno, strerror(errno));
			}

			return -1;
		}

		return 0;
		
	}

	int CloseSession(Session* session)
	{
		session->Shutdown();
		_pollsize--;

		if(_pollsize>session->EventIndex)
		{
			memcpy(&_pfds[session->EventIndex],&_pfds[_pollsize],sizeof(pollfd));
			Session* s = FindSession(_pfds[_pollsize].fd);
			
			if(s==NULL)
			{
				_stoped = true;
				LOG (LOG_ERROR, __FILE__, __LINE__, "the session is null");
				return -1;
			}
			
			s->EventIndex = session->EventIndex;
		}

		if(!_stoped)
		{
			if(session->Open()) return -1;
			LOG (LOG_ERROR, __FILE__, __LINE__, "service stop !!");
			_needconns++;
		}

		return 0;
	}

	static void* OnProcess(void* object)
	{
		GWProxy<CService>* service = static_cast<GWProxy<CService>*>(object);
		while(!service->_stoped)
		{
			PacketQueue::value_type value = service->PopPacket();
			Gateway* session = (Gateway*)(value.first);
			Packet* packet = value.second;
			
			//do work.
			int errorCode = 0;
			if(session->Registered)
			{
				service->OnProcessPacket(*session,*packet);
			}
			else
			{
				switch(packet->MsgType)
				{
				case MSGTYPE_GW_SVRCONNECT:
					packet->GetBuffer()->Read(&errorCode,sizeof(errorCode));
					if(errorCode!=0)
					{
						LOG (LOG_ERROR, __FILE__, __LINE__, "register to gw error!! read error, errorCode[%d]", errorCode );
					}
					session->Registered = (errorCode==0);
					break;
				case MSGTYPE_GW_CLOSECLIENT:
					break;
				default:
					LOG (LOG_ERROR, __FILE__, __LINE__, "methodId is not find");
				}
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

	Gateway* FindSession(int fd)
	{
		GWList::iterator iter = _gateways.begin();

		for(;iter!=_gateways.end();++iter)
		{
			if(iter->Handle==fd) return &(*iter);
		}

		return NULL;
	}

	Gateway* FindSessionByID(int id)
	{
		GWList::iterator iter = _gateways.begin();

		for(;iter!=_gateways.end();++iter)
		{
			if(iter->ID==id) return &(*iter);
		}

		return NULL;
	}
	
	//register to gateway.
	int Register(Gateway* session)
	{
		if(!session->Connected) return -1;

		if(session->Registered) return 0;

		Packet packet(1024);


		//包头字段填写
		packet.Length = PACKET_HEADER_LENGTH;
		packet.Direction = DIRECT_S_S_REQ;
		packet.SvrType = 0;
		packet.SvrSeq = 0;
		packet.MsgType = MSGTYPE_GW_SVRCONNECT;
		packet.UniqID = 0;

		//包头写到缓存
		packet.PackHeader();

DEBUG_PRINTF1( "req pkg------1111  packet.GetBuffer()->GetDataSize[%d] \n", packet.GetBuffer()->GetBufferSize());
	DEBUG_SHOWHEX( packet.GetBuffer()->GetDataPtr(), PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );



		//包体写到缓存
		packet.GetBuffer()->Write(&_svrType,1);
		packet.GetBuffer()->Write(&_svrSeq,1);

		//包头的包长字段更新
		packet.UpdatePacketLength();

DEBUG_PRINTF1( "req pkg------1111  packet.GetBuffer()->GetDataSize[%d] \n", packet.GetBuffer()->GetBufferSize());
	DEBUG_SHOWHEX( packet.GetBuffer()->GetDataPtr(), 30, 0, __FILE__, __LINE__ );

		return session->Send(packet.GetBuffer());
	}

	
private:
		//消息队列
	PacketQueue _packets;

	//消息队列 mutex
	pthread_mutex_t _pkgMutex;

	//消息队列 条件变量
	pthread_cond_t _pkgCond;

	//本服务停止标志   false 未停止   true 停止
	bool _stoped;

	//网关列表
	GWList _gateways;

	//本服务名
	char* _svcName;

	//本服务sock 地址
	sockaddr_in _localaddr;

	//poll fd
	pollfd* _pfds;

	//需要重连的网关个数
	int _needconns;

	//已连接的网关个数
	int _pollsize;

	//上次连接网关的时间
	time_t _lastconn;

	//主线程 线程 id
	pthread_t _thrMain;

	//子线程池 线程id
	pthread_t* _thrWork;

	//本服务的IP
	//	从 ini 文件读取
	char _localIP[16];
	
	//svrType
	//	从 ini 文件读取
	unsigned char _svrType;

	//svrSeq
	//	从 ini 文件读取
	unsigned char _svrSeq;

	//子线程池线程个数
	//	从 ini 文件读取
	int _threads;

	// Session中 DataBuff 的长度
	//	从 ini 文件读取
	unsigned int _bufferSize;

	//单个消息的最大长度
	//	从 ini 文件读取
	unsigned int _maxMsgSize;

	//网关总个数
	//	从 ini 文件读取
	unsigned int _totalGW;

	//配置文件路径
	char* _confFilePath;
	
};



#endif
