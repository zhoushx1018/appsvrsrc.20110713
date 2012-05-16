#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include "net.h"
#include "log.h"
#include "ini_file.h"
#include "PK_main_thread.hpp"
#include "PK_thread.h"

Main_thread::Main_thread()
{
	pthread_mutex_init(&lock_send_gw, NULL);
	pthread_mutex_init(&lock_kill_thread, NULL);
	max_fd = 0;
	client = (pollfd*) malloc (sizeof (pollfd) * MAXFDS);
	for(int i=0; i<MAXFDS; i++)
		client[i].fd = -1;
	fd_gw = -1;
}

Main_thread::~Main_thread()
{
	map<int,Connect_Session_t*>::iterator pos;
	for (pos=cn_session.begin(); pos!=cn_session.end(); ++pos)	
	{
		Connect_Session_t* s = pos->second;
		remove_session (s->id);
	}

	map<unsigned int,  PK_PE*>::iterator pos_pk;
	for (pos_pk=map_PK.begin(); pos_pk!=map_PK.end(); ++pos_pk)	
	{
		map_PK.erase (pos_pk);
		delete pos_pk->second;
	}
	
	free (client);
}

void Main_thread:: run()
{
	int listenfd = open_tcp_port (option.PKIP, option.PKPort, option.backlog);
	if (listenfd == -1)
		exit (1);
	long long id= CONNECTION_ID (inet_addr (option.PKIP), option.PKPort, listenfd);
	Connect_Session_t* s = new Connect_Session_t (id, TCP_LISTEN);
	insert_session (s);

	pthread_t _thrTimer;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if((pthread_attr_setstacksize(&attr, 1024000)) != 0)
	{
	        return;
	}
	if(pthread_create(&_thrTimer, &attr,timer_event,this) != 0)
	{
	        return;
	}

	int timeout = 1000;
	while (!stopped)
	{
		int ready = poll (client, max_fd + 1, timeout);
		if (ready > 0)
			run_once (ready);
		
		connect_gw();
	}
}

void* Main_thread::timer_event(void* object)
{
	Main_thread *ptrC = (Main_thread*)object;
	map<unsigned int,  PK_PE*>::iterator pos_PK;

	struct timeval timeout;
	while (!stopped)
	{
	        timeout.tv_sec = 0;
	        timeout.tv_usec = 100000;
	        select( 0, NULL, NULL, NULL, &timeout );
	        pthread_mutex_lock(&ptrC->lock_kill_thread);
	        for(pos_PK=ptrC->map_PK.begin(); pos_PK != ptrC->map_PK.end(); ++pos_PK)
	        {
	                (pos_PK->second)->write_pipe(1);
	        }
	        pthread_mutex_unlock(&ptrC->lock_kill_thread);
	}

        return NULL;
}

int Main_thread::send_client(char* SendBuffer, int Length)
{
	int send_length = 0;
	pthread_mutex_lock(&lock_send_gw);
	send_length = send_buff(fd_gw, SendBuffer, Length);
 	pthread_mutex_unlock(&lock_send_gw);

	return send_length;
}

int Main_thread::kill_thread(unsigned int thread_id)
{
	map<unsigned int,  PK_PE*>::iterator pos;
	pthread_mutex_lock(&lock_kill_thread);
	pos = map_PK.find (thread_id);
	if (pos != map_PK.end ())
	{
		delete pos->second;
		map_PK.erase (pos);
	}
	else
	{
		LOG (LOG_ERROR,  __FILE__, __LINE__, "kill_thread: thread can not find[%d]", thread_id);
	}
	pthread_mutex_unlock(&lock_kill_thread);
	LOG (LOG_DEBUG,  __FILE__, __LINE__, "kill_thread: thread[%d] killed!left thread[%d]", thread_id, map_PK.size());

	return 0;
}

void Main_thread::insert_session (Connect_Session_t* s)
{
	int key = CONNECTION_FD (s->id);
	cn_session.insert (make_pair (key, s));

	add_in_event (key);
	LOG (LOG_VERBOSE,  __FILE__, __LINE__, "insert into session,fd=%d,type=%d", key, s->type);
	return;
}

Connect_Session_t* Main_thread::find_session(int fd)
{
	map<int,Connect_Session_t*>::iterator pos;
	pos = cn_session.find (fd);
	if (pos == cn_session.end ())
		return NULL;

	return pos->second;
}

int Main_thread::remove_session(long long id)
{
	map<int,Connect_Session_t*>::iterator pos;

	int fd = CONNECTION_FD (id);
	if ((pos = cn_session.find (fd)) == cn_session.end())
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "remove session,can't find session map \n");
		return -1;
	}
	int poll_index;
	if ((poll_index = find_poll_index (fd)) == -1)
	{
		LOG (LOG_FATAL, __FILE__, __LINE__, "handle_output,can't find poll index: fd=%d", fd);
		return -1;
	}

	if (max_fd > 1)
		client [poll_index] = client[max_fd - 1];
	client[max_fd - 1].fd = -1;
	max_fd -- ;

	if(fd_gw == fd)
		fd_gw = -1;

	Connect_Session_t* s = pos->second;
	LOG (LOG_VERBOSE, __FILE__, __LINE__, "remove session,fd=%d,type=%d,stamp=%d", pos->first, s->type, s->stamp);
	delete s;
	cn_session.erase (pos);

	return 0;
}

int Main_thread::handle_tcp_close (Connect_Session_t* s) 
{
	int fd = CONNECTION_FD (s->id);
	if (fd < 0)
		return -1;

	close (fd);
	remove_session (s->id);
	
	return 0; 
}

int Main_thread::handle_accept(Connect_Session_t* s)
{
	int sockfd, newfd;
	struct sockaddr_in peer;

	if(s->type != TCP_LISTEN)
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "accept session type=%d,invalid",s->type);
		return -1;
	}

	sockfd = CONNECTION_FD(s->id);
	newfd = accept_tcp (sockfd, &peer);
	if (newfd < 0)
		return -1;

	long long id = CONNECTION_ID (peer.sin_addr.s_addr, peer.sin_port, newfd);
	Connect_Session_t* ss = new Connect_Session_t (id, TCP_STREAM);
	if (ss->recv_mb == NULL || ss->send_mb == NULL)
	{
		LOG (LOG_FATAL, __FILE__, __LINE__,  "malloc session buffer failed");
		delete ss;
		return -1;
	}
	insert_session (ss);

	return 0;
}

int Main_thread::check_timeout(Connect_Session_t* s, int timeout)
{
	if (s->type != TCP_STREAM || s->recv_len == 0)
		return 0;

	int difftime = time(NULL) - s->stamp;
	if (difftime > timeout || difftime < 0)
	{
		LOG (LOG_VERBOSE, __FILE__, __LINE__, "check_timeout:%d", difftime);
		return handle_tcp_close (s) ;
	}
	
	return 0;
}

int Main_thread::connect_gw()
{
	if(fd_gw > 0)
		return fd_gw;
	
	int sockfd = 0;
	struct	sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;	
	servaddr.sin_port = htons(option.GWPort);
	inet_pton(AF_INET, option.GWIP, &servaddr.sin_addr);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (connect (sockfd, (const struct sockaddr *)&servaddr, sizeof (servaddr)) != 0)	
	{
		LOG (LOG_ERROR,  __FILE__, __LINE__, "connect exchange[%s][%d] fail! error[%s]", option.GWIP, option.GWPort, strerror(errno));
		return -1;
	}

	struct timeval timeout = {1, 0};
	setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)& timeout, sizeof (struct timeval));

	//set socket buffer
	int bufsize = option.socket_bufsize;
	setsockopt (sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, sizeof (int));
	setsockopt (sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize, sizeof (int));

	PkgHead stPkgHead;
	stPkgHead.ucDirection = DIRECT_SC_S_REQ;
	stPkgHead.ucSrvType = SRVTYPE_PK;
	stPkgHead.ucSrvSeq = option.srv_req;
	stPkgHead.usMsgType = MSGTYPE_S_S_CONNECT_GW;
	stPkgHead.usPkgLen = LEN_PKG_HEAD+1+1;
	
	char sBuffer[LEN_PKG_HEAD+1+1+1]= {0};
	int nLen = LEN_PKG_HEAD;
	stPkgHead.packet(sBuffer);
	*(sBuffer + LEN_PKG_HEAD) = SRVTYPE_PK;
	*(sBuffer + LEN_PKG_HEAD+1) = option.srv_req;
	nLen += 2;
	if ( nLen != send (sockfd, sBuffer, nLen, 0))		
	{
		LOG (LOG_FATAL,  __FILE__, __LINE__, "send exchange fail!");
		close (sockfd);	
		return -1;
	}

	nLen = LEN_PKG_HEAD + 4;
	memset(sBuffer, 0 , sizeof(sBuffer));
	if(nLen != recv (sockfd, sBuffer, nLen, 0))
	{
		LOG (LOG_FATAL, __FILE__, __LINE__,  "recv GW[%s][%d] fail!", option.GWIP, option.GWPort);
		close (sockfd);
		return -1;
	}

	int val = fcntl(sockfd, F_GETFL, 0); 
	val |= O_NONBLOCK; 
	fcntl (sockfd, F_SETFL, val);

	Connect_Session_t* ss = new Connect_Session_t (sockfd, TCP_STREAM);
	if(ss == NULL)
		return -1;
	insert_session(ss);
	fd_gw = sockfd;

	LOG (LOG_DEBUG,  __FILE__, __LINE__, "connect GW[%s][%d] OK!", option.GWIP, option.GWPort);

	return sockfd;
}

int Main_thread::handle_tcp_input(Connect_Session_t* s)
{
	if (recv_tcp_buffer (s) == -1)
	{
		handle_tcp_close (s);
		return -1;
	}

	PkgHead stPkgHead;
	while(1)
	{
		if (s->recv_len < LEN_PKG_HEAD)
		{
			return 0;
		}

		stPkgHead.unpacket(s->recv_mb);
		if (stPkgHead.usPkgLen < LEN_PKG_HEAD || stPkgHead.usPkgLen > option.socket_bufsize)
		{
			handle_tcp_close (s);
			return -1;
		}

		if(stPkgHead.usPkgLen > s->recv_len)
			return 0;

		//¿Í»§¶ËÇëÇó
		if(stPkgHead.ucDirection == DIRECT_C_S_REQ)
		{
			if(stPkgHead.usMsgType == MSGTYPE_C_S_READY)
			{
				process_c_s_ready(stPkgHead.uiRoleID, s->recv_mb + LEN_PKG_HEAD, stPkgHead.usPkgLen - LEN_PKG_HEAD);
			}
			if(stPkgHead.usMsgType == MSGTYPE_C_S_ORDER)
			{
				process_c_s_order(stPkgHead.uiRoleID, s->recv_mb + LEN_PKG_HEAD, stPkgHead.usPkgLen - LEN_PKG_HEAD);
			}
		}
		else if(stPkgHead.ucDirection == DIRECT_SC_S_REQ)
		{
			if(stPkgHead.usMsgType == MSGTYPE_S_S_BEGIN)
			{
				int sockfd = CONNECTION_FD (s->id);
				process_s_s_begin(sockfd, s->recv_mb + LEN_PKG_HEAD, stPkgHead.usPkgLen - LEN_PKG_HEAD);
			}
		}
		
		if (stPkgHead.usPkgLen == s->recv_len)
		{
			s->recv_len = 0;
			return 0;
		}
		else 
		{
			s->recv_len = s->recv_len - stPkgHead.usPkgLen;
			memmove (s->recv_mb, s->recv_mb + stPkgHead.usPkgLen, s->recv_len);
		}
	}

	return 0;
}

int Main_thread::process_c_s_ready(int RoleID, char *input, int length)
{
	int nInLen = 0;
	if(length != 4)
		return -1;

	unsigned int pk_oject_id;
	memcpy(&pk_oject_id, input + nInLen, 4);
	nInLen += 4;

	pthread_mutex_lock(&lock_kill_thread);
	map<unsigned int,  PK_PE*>::iterator pos_pk;
	pos_pk = map_PK.find (pk_oject_id);
	if (pos_pk != map_PK.end ())
	{
		pos_pk->second->process_c_s_ready(RoleID);
	}
	pthread_mutex_unlock(&lock_kill_thread);
	
	return 0;
}

int Main_thread::process_c_s_order(int RoleID, char *input, int length)
{
	//LOG (LOG_DEBUG, __FILE__, __LINE__, "process_c_s_order[%d][%d]!", RoleID, length);
	int nInLen = 0;
	if(length != 15)
		return -1;

	UserOrder stUserOrder;
	stUserOrder.ControlID = RoleID;
	
	unsigned int pk_oject_id;
	memcpy(&pk_oject_id, input + nInLen, 4);
	nInLen += 4;
	memcpy(&stUserOrder.RoleID, input + nInLen, 4);
	nInLen += 4;
	memcpy(&stUserOrder.Order.Type, input + nInLen, 2);
	nInLen += 2;
	if(*(input + nInLen) == 1)
	{
		stUserOrder.Order.TargetType = 1; //1: Pos
		nInLen += 1;
		memcpy(&stUserOrder.Order.TargetPos.X, input + nInLen, 2);
		nInLen += 2;
		memcpy(&stUserOrder.Order.TargetPos.Y, input + nInLen, 2);
		nInLen += 2;
	}
	else if(*(input + nInLen) == 2)
	{
		stUserOrder.Order.TargetType = 2; //2: RoleID
		nInLen += 1;
		memcpy(&stUserOrder.Order.TargetRoleID, input + nInLen, 4);
	}
	else
	{
		return -1;
	}
	
	map<unsigned int,  PK_PE*>::iterator pos;
	pthread_mutex_lock(&lock_kill_thread);
	pos = map_PK.find (pk_oject_id);
	if (pos != map_PK.end ())
	{
		(pos->second)->AddOrder(stUserOrder);
		(pos->second)->write_pipe(0);
	}
	pthread_mutex_unlock(&lock_kill_thread);

	return 0;
}

int Main_thread::process_s_s_begin(int sockfd, char *input, int length)
{
	int nRtn = 0;
	unsigned int key_pk_object = 0;
	memcpy(&key_pk_object, input, 4);
	PK_PE *PK_worker = NULL;
	PK_worker = new PK_PE();
	if(PK_worker == NULL)
	{
		LOG (LOG_FATAL, __FILE__, __LINE__, "malloc error!");
		return -1;
	}
	else
	{
		nRtn = PK_worker->Init(this, input, length);
		if(nRtn == 0)
		{
			pthread_mutex_lock(&lock_kill_thread);
			map_PK.insert (make_pair(key_pk_object, PK_worker));
			pthread_mutex_unlock(&lock_kill_thread);
			LOG (LOG_DEBUG, __FILE__, __LINE__, "[%u] create thread OK!", key_pk_object);
		}
		else
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "PK_PE.Init error!");
			delete PK_worker;
			PK_worker = NULL;
		}
	}	

	char	create_pk_resp[LEN_PKG_HEAD+4+4] = {0};
	PkgHead stPkgHead;
	stPkgHead.usPkgLen = LEN_PKG_HEAD+4+4;
	stPkgHead.ucDirection = DIRECT_SC_S_RESP;
	stPkgHead.ucSrvType = SRVTYPE_PK;
	stPkgHead.ucSrvSeq = option.srv_req;
	stPkgHead.usMsgType = MSGTYPE_S_S_BEGIN;
	stPkgHead.packet(create_pk_resp);
	memcpy(create_pk_resp+LEN_PKG_HEAD, &nRtn, 4);
	memcpy(create_pk_resp+LEN_PKG_HEAD+4, &key_pk_object, 4);
	
	int send_length = send_buff(sockfd, create_pk_resp, LEN_PKG_HEAD+4+4);
	if(send_length != LEN_PKG_HEAD+4+4 )
		LOG (LOG_ERROR, __FILE__, __LINE__, "send buffer error!");

	return 0;
}

void Main_thread::run_once (int ready)
{
	Connect_Session_t* s = NULL; 
	for (int i = 0; i < max_fd || client[i].fd != -1; i++)
	{
		if ((s = find_session (client[i].fd)) == NULL)
			continue;
		if (s->type == TCP_STREAM && (client[i].revents & (POLLERR | POLLHUP)))
		{
			handle_tcp_close (s);
			continue;
		}
		if (client[i].revents & POLLIN)
		{
			switch (s->type)
			{
				case TCP_LISTEN:
					handle_accept (s);
					break;
				case TCP_STREAM:
					handle_tcp_input (s);
					break;
				default:
					LOG (LOG_ERROR, __FILE__, __LINE__, "invalid Connect_Session_t type=%d", s->type);
			}
		}
		else
		{
			send_tcp_session (s);
			check_timeout (s, option.socket_timeout);
		}
	}
	return;
}
