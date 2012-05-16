#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <error.h>
#include <errno.h>

#include "net.h"
#include "log.h"
#include "common.h"
#include "ini_file.h"

int recv_tcp_buffer (Connect_Session_t* s)
{
	int buf_len = option.socket_bufsize;
	int sockfd = CONNECTION_FD (s->id);

	int recv_bytes = recv (sockfd, s->recv_mb + s->recv_len, buf_len - s->recv_len, 0);
	if (recv_bytes == -1)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return 0;
		else
			return -1;
	}
	else if (recv_bytes ==	0)
	{
		LOG (LOG_VERBOSE,  __FILE__, __LINE__, "recv 0 byte,buf_len=%d,fd=%d,%s", buf_len - s->recv_len, sockfd, strerror (errno));
		return -1;
	} 

	s->recv_len += recv_bytes;	
	s->stamp = time (NULL);
	LOG (LOG_VERBOSE,  __FILE__, __LINE__, "recv fd=%d,recv_len=%d,session_len=%d", sockfd, recv_bytes, s->recv_len);

	return 0;		
}

int recv_udp_buffer(int sockfd, Connect_Session_t* s)
{
	struct sockaddr_in addr;

	socklen_t addrlen = sizeof addr;
	int recv_bytes = recvfrom (sockfd, s->recv_mb, option.socket_bufsize, 0,
			(struct sockaddr*)&addr, &addrlen);

	if (recv_bytes == -1 || recv_bytes == 0)
	{
		s->recv_len = 0;
		LOG (LOG_ERROR,  __FILE__, __LINE__, "recvfrom fd=%d,%s", sockfd, strerror (errno));
		return -1;
	}

	s->recv_len = recv_bytes;
	s->id = CONNECTION_ID (addr.sin_addr.s_addr, addr.sin_port, sockfd);

	return 0;
}

int send_tcp_session(Connect_Session_t* s)
{
	if (s->type != TCP_STREAM)
		return 0;

	int sockfd = CONNECTION_FD(s->id);
	//先发送session中的数据
	if (s->send_len > 0)
	{
		int bytes_tr = send(sockfd, s->send_mb, s->send_len, 0);
		if (bytes_tr == -1)
		{
			if (errno != EINTR && errno != EWOULDBLOCK)
			{
				LOG (LOG_ERROR,  __FILE__, __LINE__, "send error,fd=%d,length=%d,%s", sockfd, s->send_len, strerror (errno));
				return -1;
			}
			bytes_tr = 0;
		}

		LOG (LOG_VERBOSE,  __FILE__, __LINE__, "send session buffer,total len=%d,send len=%d,",s->send_len, bytes_tr);
		s->stamp = time (NULL);
		if (bytes_tr < s->send_len && bytes_tr >= 0)
		{
			s->send_len = s->send_len - bytes_tr;
			memmove (s->send_mb, s->send_mb + bytes_tr, s->send_len);
		} 
		else if (bytes_tr == s->send_len)
		{
			s->send_len = 0;
		}
	}
	
	return 0;
}

int send_tcp_buffer(Connect_Session_t* s, const shm_block* mb)
{
	int bytes_tr, surplus;
	int sockfd = CONNECTION_FD (s->id);

	if (mb == NULL || MB_DATA_LENGTH(mb) == 0)
		return 0;

	bytes_tr = send (sockfd, mb->data, MB_DATA_LENGTH(mb), 0);
	if (bytes_tr == -1)
	{
		if (errno != EINTR && errno != EWOULDBLOCK)
		{
			LOG (LOG_ERROR,  __FILE__, __LINE__, "send error,length=%d,%s",s->send_len, strerror (errno));
			return -1;
		}
		bytes_tr = 0;	
	}
	LOG (LOG_VERBOSE,  __FILE__, __LINE__, "send tcp buffer,total=%d,send len=%d",MB_DATA_LENGTH(mb), bytes_tr);

	s->stamp = time (NULL);
	if ((surplus = MB_DATA_LENGTH(mb) - bytes_tr) > 0)
	{
		memcpy (s->send_mb, mb->data + bytes_tr, surplus);
		s->send_len = surplus;
	}

	return 0;
}


int send_buffer (Connect_Session_t* s,const shm_block* mb)
{
	if (s->type == TCP_STREAM)
	{
		if (send_tcp_session (s) != 0)
			return -1;

		if (s->send_len == 0)
			return send_tcp_buffer (s, mb);

		//has session,append mb
		if (s->send_len + MB_DATA_LENGTH(mb) > option.socket_bufsize)
		{
			LOG (LOG_ERROR,  __FILE__, __LINE__, "session buffer is overflow,s_len=%d,mb_len=%d",
				s->send_len, MB_DATA_LENGTH(mb));
			return -1;
		}

		memcpy (s->send_mb + s->send_len, mb->data, MB_DATA_LENGTH(mb));
		s->send_len += MB_DATA_LENGTH(mb);

		return 0;
	}
	
	return -1;
}

int open_tcp_port (const char* ip, short port, int backlog)
{
	int	listenfd;
	struct sockaddr_in servaddr;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons (port);
	inet_pton(AF_INET, ip, &servaddr.sin_addr);

	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) == -1) 
	{ 
		fprintf (stderr, "socket error:%s\n",strerror(errno)); 
		return -1;
	} 
	int reuse_addr = 1;
	setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof reuse_addr);

	if (bind (listenfd, (struct sockaddr *)(&servaddr), sizeof(struct sockaddr)) == -1) 
	{ 
		LOG (LOG_ERROR,  __FILE__, __LINE__, "bind %s:%d error:%s", ip, port, strerror(errno));
		return -1;
	} 
	
	if (listen (listenfd, backlog) == -1) 
	{ 
		LOG (LOG_ERROR,  __FILE__, __LINE__,  "listen error:%s",strerror(errno)); 
		return -1;
	} 
	
	int val = fcntl(listenfd, F_GETFL, 0); 
	val |= O_NONBLOCK; 
	fcntl (listenfd, F_SETFL, val);

	LOG (LOG_DEBUG,  __FILE__, __LINE__, "open tcp %s:%d ok", ip, port);
	return listenfd;
}


int accept_tcp (int sockfd, struct sockaddr_in *peer)
{
	socklen_t peer_size;
	int newfd;

	for ( ; ; ) 
	{
		peer_size = sizeof(struct sockaddr_in); 
		if ((newfd = accept(sockfd, (struct sockaddr *)peer, &peer_size)) < 0)
		{
			if (errno == EINTR)
				continue;         /* back to for () */

			LOG (LOG_ERROR, __FILE__, __LINE__, "accept failed,listenfd=%d", sockfd);
			return -1;
		}

		break;
	}

	//set nonblock
	int val = fcntl(newfd, F_GETFL,0); 
	val |= O_NONBLOCK; 
	fcntl(newfd, F_SETFL, val);
	
	//set socket buffer
	int bufsize = 65535;
	setsockopt (newfd, SOL_SOCKET, SO_RCVBUF, (char *) bufsize, sizeof (int));
	setsockopt (newfd, SOL_SOCKET, SO_SNDBUF, (char *) bufsize, sizeof (int));

	LOG (LOG_VERBOSE,  __FILE__, __LINE__, "accept connection,fd=%d,ip=%s,port=%d", 
		sockfd, inet_ntoa(peer->sin_addr), ntohs(peer->sin_port));
	return newfd;
}

int send_buff(int sockfd, const char *buffer, int buflen)
{
	int tmp;
	int total = buflen;
	const char *p = buffer;

	while(1)
	{
		tmp = send(sockfd, p, total, 0);
		if(tmp < 0)
		{
			// 当send收到信号时,可以继续写,但这里返回-1.
			if(errno == EINTR)
				return -1;
			// 当socket是非阻塞时,如返回此错误,表示写缓冲队列已满,
			// 在这里做延时后再重试.
			if(errno == EAGAIN)
			{
				usleep(1000);
				continue;
			}
			return -1;
		}

		if(tmp == total)
		{
			LOG (LOG_VERBOSE,  __FILE__, __LINE__, "send_buff,fd=%d,length=%d", 
				sockfd, total);
			return buflen;
		}
		
		total -= tmp;
		p += tmp;
	}
}

