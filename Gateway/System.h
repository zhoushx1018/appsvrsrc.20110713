#ifndef SYSTEM_H
#define SYSTEM_H

#include "OurDef.h"



#define CONNECTION_FD(x)	(int)(x&0xFFFF)
#define CONNECTION_IP(x)	(x >> 32)
#define CONNECTION_PORT(x)	(int)((x >> 16)&0xFFFF)


extern long long CONNECTION_ID (int ip, unsigned short port, int fd); 

extern void init_fdlimit ();

extern void daemon_start ();

extern bool stopped;

int open_tcp_port_nblk (const char* ip, short port, int backlog);

int open_tcp_port_blk (const char* ip, short port, int backlog);

int accept_tcp_nblk (int listenfd, struct sockaddr_in *peer);

void setnonblocking(int sock);





#endif

