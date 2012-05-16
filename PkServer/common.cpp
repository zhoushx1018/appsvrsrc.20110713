#include "common.h"
#include "ini_file.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/socket.h>

bool stopped = false;

long long CONNECTION_ID (int ip, unsigned short port, int fd) 
{	
	long long retval, tmp; 
	int pf = port;
	pf = (pf << 16) + fd;
	tmp = ip;
	retval = (tmp << 32) + pf; 
	return retval; 
}

static void sigterm_handler(int signo) 
{
	stopped = true;
}

Connect_Session_t::Connect_Session_t (long long key, char t)
{
	id = key;
	type = t;
	stamp = time (NULL);
	recv_len = 0;
	send_len = 0;
	recv_mb = (char *) malloc (option.socket_bufsize);
	send_mb = (char *) malloc (option.socket_bufsize);
}

Connect_Session_t::~Connect_Session_t ()
{
	if (recv_mb != NULL)
	{
		free (recv_mb);
		recv_mb = NULL;
	}
	if (send_mb != NULL)
	{
		free (send_mb);
		send_mb = NULL;
	}

	send_len = 0;
	recv_len = 0;
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

	/* raise open files */
	rlim.rlim_cur = MAXFDS;
	rlim.rlim_max = MAXFDS;
	setrlimit(RLIMIT_NOFILE, &rlim);

	/* allow core dump */
	rlim.rlim_cur = RLIM_INFINITY;
	rlim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &rlim);
}

