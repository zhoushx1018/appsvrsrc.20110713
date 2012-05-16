#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <fcntl.h>



#include "System.h"
#include "Log.h"

bool stopped = false;

long long CONNECTION_ID (int ip, unsigned short port, int fd) 
{	
	long long retval, tmp; 
	long long pf = 0x0000ffff & port;
	pf = (pf << 16) | fd;
	tmp = ip;
	retval = (tmp << 32) | pf; 
	return retval; 
}



static void sigterm_handler(int signo) 
{
	stopped = true;
}

void daemon_start ()
{
	struct sigaction sa;
	sigset_t sset;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigterm_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);

	signal(SIGPIPE,SIG_IGN);	
	signal(SIGCHLD,SIG_IGN);	

	sigemptyset(&sset);
	sigaddset(&sset, SIGSEGV);
	sigaddset(&sset, SIGBUS);
	sigaddset(&sset, SIGABRT);
	sigaddset(&sset, SIGILL);
	sigaddset(&sset, SIGCHLD);
	sigaddset(&sset, SIGFPE);
	sigprocmask(SIG_UNBLOCK, &sset, &sset);

	daemon (1, 1);
}

void init_fdlimit ()
{
	struct rlimit rlim;

	/* allow core dump */
	rlim.rlim_cur = RLIM_INFINITY;
	rlim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &rlim);
}


int open_tcp_port_blk (const char* ip, short port, int backlog)
{
	int listenfd;
	struct sockaddr_in servaddr;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons (port);
	inet_pton(AF_INET, ip, &servaddr.sin_addr);

	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) == -1) 
	{ 
		fprintf (stderr, "\n%s Line.%d-->socket error:%s\n", __FILE__, __LINE__, strerror(errno));
		return -1;
	} 
	int reuse_addr = 1;
	setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof reuse_addr);

	if (bind (listenfd, (struct sockaddr *)(&servaddr), sizeof(struct sockaddr)) == -1) 
	{ 
		fprintf (stderr, "\n%s Line.%d-->bind %s:%d error:%s\n", __FILE__, __LINE__, ip, port, strerror(errno) );
		return -1;
	}
	
	if (listen (listenfd, backlog) == -1) 
	{ 
		fprintf (stderr, "\n%s Line.%d-->listen error:%s\n", __FILE__, __LINE__, strerror(errno));
		return -1;
	}

	printf( "\n%s Line.%d--> -----open tcp %s:%d\t[ok]\n", __FILE__, __LINE__, ip, port );
	return listenfd;
}


int open_tcp_port_nblk (const char* ip, short port, int backlog)
{
	int	listenfd;
	struct sockaddr_in servaddr;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons (port);
	inet_pton(AF_INET, ip, &servaddr.sin_addr);

	if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) == -1) 
	{ 
		fprintf (stderr, "\n%s Line.%d-->socket error:%s\n", __FILE__, __LINE__, strerror(errno));
		return -1;
	} 
	int reuse_addr = 1;
	setsockopt (listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof reuse_addr);

	if (bind (listenfd, (struct sockaddr *)(&servaddr), sizeof(struct sockaddr)) == -1) 
	{ 
		fprintf (stderr, "\n%s Line.%d-->bind %s:%d error:%s\n", __FILE__, __LINE__, ip, port, strerror(errno) );
		return -1;
	}
	
	if (listen (listenfd, backlog) == -1) 
	{ 
		fprintf (stderr, "\n%s Line.%d-->listen error:%s\n", __FILE__, __LINE__, strerror(errno));
		return -1;
	}

	//setnonblocking
	setnonblocking(listenfd);
	
	printf( "\n%s Line.%d--> -----open tcp %s:%d\t[ok]\n", __FILE__, __LINE__, ip, port );
	return listenfd;
}

int accept_tcp_nblk (int sockfd, struct sockaddr_in *peer)
{
	socklen_t peer_size;
	int newfd;

	for ( ; ; ) 
	{
		peer_size = sizeof(struct sockaddr_in); 
		if ((newfd = accept(sockfd, (struct sockaddr *)peer, &peer_size)) < 0)
		{
			//连接数达到上限
			if( errno == EMFILE )
			{
				//此时不能无法日志，因为已经到达最大文件数；因为写日志需要新的fd，而此时已经没有更多的fd
				fprintf( stderr, "EMFILE !!!!!! too many fd!!! accept error, lsfd[%d], errno[%d], strerror[%s]\n", sockfd, errno, strerror(errno) );
				return -1;
			}

			if (errno == EINTR)
				continue;         /* back to for () */

			if(errno == EAGAIN )
				return -1;

			LOG (LOG_ERROR, __FILE__, __LINE__, "accept failed,listenfd=[%d], errno[%d] errmsg[%s]", sockfd, errno, strerror(errno));
			return -1;
		}

		break;
	}

	//set socket buffer
	int bufsize = 655350;
	setsockopt (newfd, SOL_SOCKET, SO_RCVBUF, (char *) bufsize, sizeof (int));
	setsockopt (newfd, SOL_SOCKET, SO_SNDBUF, (char *) bufsize, sizeof (int));

	// 设置 SO_REUSEADDR
	int reuse_addr = 1;
	setsockopt (newfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof reuse_addr);

	// 设置	SO_LINGER
	linger m_sLinger;
	m_sLinger.l_onoff = 1; // (在closesocket()调用,但是还有数据没发送完毕的时候容许逗留)
	m_sLinger.l_linger = 0; // (容许逗留的时间为0秒)
	setsockopt(newfd, SOL_SOCKET, SO_LINGER,(const char*)&m_sLinger, sizeof(linger));
	
	//setnonblocking
	setnonblocking(newfd);

/*	LOG( LOG_VERBOSE, __FILE__, __LINE__, "accept connection,fd=[%d],ip=[%s],port=[%d]", 
		sockfd, inet_ntoa(peer->sin_addr), ntohs(peer->sin_port));
*/
	return newfd;
}


void setnonblocking(int sock)
{
	int opts;
	opts=fcntl(sock,F_GETFL);
	if(opts<0)
	{
		perror("fcntl(sock,GETFL)");
		LOG (LOG_ERROR, __FILE__, __LINE__, "fcntl error" );
		exit(1);
	}
	
	opts = opts|O_NONBLOCK;
	if(fcntl(sock,F_SETFL,opts)<0)
	{
		perror("fcntl(sock,SETFL,opts)");
		LOG (LOG_ERROR, __FILE__, __LINE__, "fcntl error" );
		exit(1);
	}
}


