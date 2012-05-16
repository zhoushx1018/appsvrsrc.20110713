#include <stdio.h>
#include <string>
#include <cstring>
#include "Option.h"

using namespace std;

Bind_Option_t bind_port_in;
Bind_Option_t bind_port_out;
Ini_Option_t ini;

Option::Option()
{
	memset (&ini, 0x0, sizeof (ini));
	memset (&bind_port_in, 0x0, sizeof (bind_port_in));
	memset (&bind_port_out, 0x0, sizeof (bind_port_out));
}

int Option::read(const char* config_file)
{
	if (!_iniFile.open (config_file))
	{
		printf ("open config file:%s failed\n", config_file);
		return -1;
	}

	string tmp;
	tmp = _iniFile.read ("logger", "LOG_DIR_PATH");
	strncpy (ini.log_dir, tmp.c_str(), sizeof ini.log_dir);
	tmp = _iniFile.read ("logger", "LOG_MAX_SIZE");
	if (tmp.empty ())
		return -1;
	ini.log_size = atoi (tmp.c_str());

	tmp = _iniFile.read ("logger", "LOG_PRENAME");
	if (tmp.empty ())
		memset (ini.log_prename, 0x0, sizeof ini.log_prename);
	else
		strncpy (ini.log_prename, tmp.c_str(), sizeof ini.log_prename);

	tmp = _iniFile.read ("logger", "LOG_MAX_NUM");
	if (tmp.empty ())
		return -1;
	ini.log_num = atoi (tmp.c_str());

	tmp = _iniFile.read ("logger", "LOG_PRIORITY");
	if (tmp.empty ())
		return -1;
	ini.log_priority = atoi (tmp.c_str());

	tmp = _iniFile.read ("misc", "SOCKET_TIMEOUT");
	if (tmp.empty ())
		return -1;
	ini.socket_timeout = atoi (tmp.c_str());

	tmp = _iniFile.read ("misc", "CONNECT_LIMIT");
	if (tmp.empty ())
		return -1;
	ini.ip_connect_limit = atoi (tmp.c_str());

	tmp = _iniFile.read ("misc", "SOCKET_BUFSIZE");
	if (tmp.empty ())
		return -1;
	ini.socket_bufsize = atoi (tmp.c_str());
	
	
	tmp = _iniFile.read ("misc", "THREAD_NUM");
	if (tmp.empty ())
		return -1;
	ini.thread_num = atoi (tmp.c_str());
	
	

	tmp = _iniFile.read ("misc", "ACCEPT_BACKLOG");
	if (tmp.empty ())
		ini.backlog = 10;
	else
		ini.backlog = atoi (tmp.c_str());

	tmp = _iniFile.read ("PORT_IN", "IP");
	if (tmp.empty ())
	{
		memset (bind_port_in.ip, 0x0, sizeof ini.log_prename);
		return -1;
	}
	else
		strncpy (bind_port_in.ip, tmp.c_str(), sizeof ini.log_prename);
	
	tmp = _iniFile.read ("PORT_IN", "PORT");
	if (tmp.empty ())
		return -1;
	bind_port_in.port = atoi (tmp.c_str());

	tmp = _iniFile.read ("PORT_OUT", "IP");
	if (tmp.empty ())
	{	
		memset (bind_port_out.ip, 0x0, sizeof ini.log_prename);
		return -1;
	}
	else
		strncpy (bind_port_out.ip, tmp.c_str(), sizeof ini.log_prename);
	
	tmp = _iniFile.read ("PORT_OUT", "PORT");
	if (tmp.empty ())
		return -1;
	bind_port_out.port = atoi (tmp.c_str());

	return 0;
}

void Option::print ()
{
	printf ("LOG_DIR:%s\n", ini.log_dir);
	printf ("LOG_MAX_SIZE:%d\n", ini.log_size);
	printf ("LOG_MAX_NUM:%d\n", ini.log_num);
	printf ("LOG_PRIORITY:%d\n", ini.log_priority);
	printf ("LOG_PRENAME:%s\n\n", ini.log_prename);

	printf ("SOCKET_TIMEOUT:%d\n", ini.socket_timeout);
	printf ("SOCKET_BUFSIZE:%d\n", ini.socket_bufsize);
	printf ("SOCKET_backlog:%d\n\n", ini.backlog);
}


