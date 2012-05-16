#ifndef COMMON_H
#define COMMON_H

#define MAXFDS				5000
#define ERROR_MSG_MAX		512
#define POLL_WAIT_TIME		100
#define POLL_SEND_MAX		100
#define Q_SLEEP_TIME			100

#pragma pack(1)
enum
{
	UDP_REQUEST = 0,
	TCP_REQUEST,
	UDP_RESPONSE,
	TCP_RESPONSE,

	INT_REQUEST,
	INT_RESPONSE,

	PAD_REQUEST
};
struct shm_block
{
	int length;
	// 32(ip)+16(port)+16(socketfd)
	long long id;
	/*
	* 0: extern call request
	* 1: extern call response
	* 2: intern call request
	* 3: intern call response
	* 4: fill bytes, discard
	*/
	int type;
	char* data;
};
#pragma pack()

#define MB_HEAD_LENGTH	((int)(sizeof (shm_block) - sizeof (char*)))
#define MB_DATA_LENGTH(mb)	(mb->length - MB_HEAD_LENGTH)

//pipe
enum
{
	UDP_LISTEN = 0,
	TCP_LISTEN,
	PIPE_STREAM,
	TCP_STREAM,
	UDP_STREAM
};
struct Connect_Session_t
{
	long long id;
	char	type;
	unsigned int stamp;
	char* recv_mb;
	int recv_len;
	char* send_mb;
	int send_len;

	Connect_Session_t (long long key, char t);
	~Connect_Session_t ();
};

extern long long CONNECTION_ID (int ip, unsigned short port, int fd); 


extern void init_fdlimit ();
extern void daemon_start ();

extern bool stopped;

#define CONNECTION_FD(x)	(int)(x&0xFFFF)
#define CONNECTION_IP(x)	(x >> 32)
#define CONNECTION_PORT(x)	(int)((x >> 16)&0xFFFF)

#endif
