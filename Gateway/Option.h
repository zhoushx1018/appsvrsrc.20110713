#ifndef OPTION_H
#define OPTION_H
#include "IniFile.h"
#include "System.h"

struct Bind_Option_t
{
	char 	ip[16];	
	u_short port;
	int	type;
};

struct Ini_Option_t
{
	char	log_dir[128];
	char	log_prename[32];
	int 	log_size;
	int		log_num;
	short	log_priority;

	int	socket_timeout;
	int	socket_bufsize;
	int	backlog;
	int	ip_connect_limit;
	int thread_num;
};

class Option
{
public:
	Option();
	int read(const char* config_file);
	void print();
protected:
	IniFile _iniFile;
};

extern Bind_Option_t bind_port_in;
extern Bind_Option_t bind_port_out;
extern Ini_Option_t ini;
extern Option option;


#endif

