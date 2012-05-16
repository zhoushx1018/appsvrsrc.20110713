#include "Option.h"
#include "System.h"
#include "Log.h"
#include "AppGwSvc.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

Log *g_pLog;
Option option;

int main(int argc,char* argv[])
{
	if (argc < 2)
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "usage: AppGwSvc config_file" );
		return -1;
	}

	char szIiniFile[256] = {0};
	strncpy (szIiniFile ,argv[1], sizeof (szIiniFile));

	if(access (szIiniFile, F_OK) != 0)
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "get config file:[%s] error", szIiniFile );
		return -1;
	}
	
	if(option.read (szIiniFile) != 0)
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "initalize config file:[%s] error\n", szIiniFile);
		return -1;
	}

	g_pLog = new Log (ini.log_dir, ini.log_priority, ini.log_size, ini.log_num, ini.log_prename);
	
	init_fdlimit ();

	daemon_start ();

	
	pid_t pid;
	if ( (pid = fork ()) < 0)
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "fork error\n");
		return -1;
	}
	else if (pid == 0) 
	{
		AppGwSvc worker;
		
		worker.run_once();
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
				LOG (LOG_ERROR, __FILE__, __LINE__, "waitpid(%d, %d)=%d", pid, status, result);
				pid = fork ();
				if(pid == 0)
				{
					AppGwSvc worker;
		
					worker.run_once();						
				}
				else if (pid > 0)
				{
					continue;
				}
				else
				{
					LOG (LOG_ERROR, __FILE__, __LINE__, "fork error\n");
					return -1;
				}
			}
		}
	}
		
	return 0;

}


