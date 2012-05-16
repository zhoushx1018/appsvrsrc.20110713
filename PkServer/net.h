#ifndef NET_H
#define NET_H

#include "common.h"


int send_buffer (Connect_Session_t* s, const shm_block* mb);
int send_tcp_session (Connect_Session_t* s);

int open_tcp_port (const char* ip, short port, int backlog);
int accept_tcp (int listenfd, struct sockaddr_in *peer);

int recv_tcp_buffer (Connect_Session_t* s);

int send_buff(int sockfd, const char *buffer, int buflen);

#endif



