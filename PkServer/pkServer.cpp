#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "ini_file.h"
#include "common.h"
#include "PK_main_thread.hpp"
#include <math.h>

Log* g_pLog = NULL;

int main(int argc,char* argv[])
{
	if (argc < 2)
	{
		printf ("usage:[option]\n");
		return -1;
	}
	char ini_file[256] = {0};
	strncpy (ini_file ,argv[1], sizeof (ini_file));

	if(access (ini_file, F_OK) != 0)
	{
		printf("get config file:%s error\n", ini_file);
		return -1;
	}

	if(initLog (ini_file) != 0)
	{
		printf ("config file pk log parameter:%s error\n", ini_file);
		return -1;
	}

	if(read_opt (ini_file, option) != 0)
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "pkServer start failed, initalize config file:%s error\n", ini_file);
		return -1;
	}


	LOG (LOG_DEBUG, __FILE__, __LINE__, "read ini sucess!");

	printf ("start success!\n");

	init_fdlimit ();
 
	daemon_start ();

	LOG (LOG_DEBUG, __FILE__, __LINE__, "pkServer started success!");

	pid_t pid;
	if ( (pid = fork ()) < 0)
	{
		printf ("fork error\n");
		return -1;
	}
	else if (pid == 0) 
	{
		Main_thread worker;
		worker.run ();	
	}
	else
	{  
		while(1)
		{
			sleep(3);
			
			int status = 0;
			int result = waitpid (pid, &status, WNOHANG);
			if (result != 0 || status != 0)
			{
				LOG (LOG_FATAL, __FILE__, __LINE__, "waitpid(%d, %d)=%d", pid, status, result);
				pid = fork ();
				if(pid == 0)
				{
					Main_thread worker;
					worker.run ();	
				}
				else if (pid > 0)
				{
					continue;
				}
				else
				{
					printf ("fork error\n");
					return -1;
				}
			}
		}
	}
	
	return 0;
}


