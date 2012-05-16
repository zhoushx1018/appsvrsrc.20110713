#ifndef __PK_MAIN_THREAD_HPP__
#define __PK_MAIN_THREAD_HPP__

#include <map>
#include <sys/poll.h>
#include <pthread.h>
#include "net.h"

using namespace std;
class PK_PE;

class Main_thread
{
public:
	Main_thread();
	~Main_thread();
	void run();
	void run_once (int ready);
	static void* 	timer_event(void* object);

private:
	void insert_session (Connect_Session_t* s);
	Connect_Session_t* find_session(int fd);
	int remove_session(long long id);
	
	int check_timeout (Connect_Session_t* s, int timeout);
	int handle_tcp_close (Connect_Session_t* s);
	int handle_tcp_input(Connect_Session_t* s);
	int handle_accept(Connect_Session_t* s);

private:
	int process_s_s_begin(int sockfd, char *input, int length);
	int process_c_s_ready(int RoleID, char *input, int length);
	int process_c_s_order(int RoleID, char *input, int length);

public:
	int send_client(char* SendBuffer, int Length);//这个函数是给子线程发数据的
	int kill_thread(unsigned int thread_id);

private:
	int connect_gw();
	int read_map();

private:
	map<int, Connect_Session_t*> 	cn_session;		//所有连接session
	map<unsigned int,  PK_PE*>		map_PK;		//工作对象

	struct pollfd *client;
	int	max_fd;

	 inline int find_poll_index (int fd)
	{
		for (int i = 1; i < max_fd + 1; i++)
			if (client[i].fd == fd)
				return i;
			else if (client[i].fd == -1)
				return -1;
		return -1;
	}

	inline void add_in_event (int fd)
  	{
                client[max_fd].events = POLLIN;
		client[max_fd].fd = fd;
		max_fd ++;
        }

	pthread_mutex_t lock_kill_thread;
	pthread_mutex_t lock_send_gw;
	int 	fd_gw;
};

#endif
