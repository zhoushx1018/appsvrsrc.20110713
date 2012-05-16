#ifndef SERVICE_DAEMON_H
#define SERVICE_DAEMON_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/resource.h>
#include "IniFile.h"
#include "Log.h"

Log* g_pLog = NULL;

template<typename SERVICE>
class Daemon
{
public:
	static int Run(const char* cfg)
	{
		if(InitLog(cfg))
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "log config occur error.");
			return -1;
		}

		init_fdlimit ();
		daemon_start ();

		bool run = true;

		pid_t pid;
		if ( (pid = fork ()) < 0)
		{
			LOG (LOG_FATAL, __FILE__, __LINE__, "fork error.");
			return -1;
		}
		else if (pid == 0) 
		{
			run = StartWork(cfg);
		}
		else
		{  
			while(run)
			{
				sleep(3);
				
				int status = 0;
				int result = waitpid (pid, &status, WNOHANG);
				if (result != 0 || status != 0)
				{
					pid = fork ();
					if(pid == 0)
					{
						StartWork(cfg);
					}
					else if (pid > 0)
					{
						continue;
					}
					else
					{
						LOG (LOG_FATAL, __FILE__, __LINE__, "fork error.");
						return -1;
					}
				}
			}
		}

		if(g_pLog) delete g_pLog;
			
		return 0;
	}

private:
	static void daemon_start ()
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

	static void sigterm_handler(int signo) 
	{
	}

	static void init_fdlimit ()
	{
		struct rlimit rlim;

		/* raise open files 
		rlim.rlim_cur = MAXFDS;
		rlim.rlim_max = MAXFDS;
		setrlimit(RLIMIT_NOFILE, &rlim);*/

		/* allow core dump */
		rlim.rlim_cur = RLIM_INFINITY;
		rlim.rlim_max = RLIM_INFINITY;
		setrlimit(RLIMIT_CORE, &rlim);
	}

	static bool StartWork(const char* cfg)
	{
		SERVICE service(cfg);
		if(service.Run()) 
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "service run error" );
			return false;
		}
		else
		{
			service.Wait();
		}

		return true;
	}

	static int InitLog(const char* cfg)
	{
		IniFile _file;
		if(!_file.open(cfg)) return -1;

		string dir,size,num,priority,prevname;

		if ( _file.read("log","dir", dir) ) return -1;
		if ( _file.read("log","size", size) ) return -1;
		if ( _file.read("log","num", num) ) return -1;
		if ( _file.read("log","priority", priority) ) return -1;

		//ºöÂÔ prevname Îª¿Õ
		_file.read("log","prevname", prevname);

		g_pLog = new Log(dir.c_str(),atoi(priority.c_str())
			,atoi(size.c_str()),atoi(size.c_str()),prevname.c_str());
		
		return 0;
	}
};

#endif
