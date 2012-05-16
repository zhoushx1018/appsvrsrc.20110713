#ifndef DATA_CONNECTIONPOOL_H
#define DATA_CONNECTIONPOOL_H

#include <list>
#include <pthread.h>
#include <mysql.h>
#include "Connection.h"

typedef std::list<MYSQL*> ConnectionList;

class ConnectionPool
{
public:
	ConnectionPool(unsigned int size);
	
	~ConnectionPool();

	bool Connect(const char * host, const char * user ,const char * password , const char * database);
	
	void SetIdleHandle(MYSQL* handle);

	Connection GetConnection();

	int GetBusyListSize();

	int GetIdleListSize();

private:
	bool _inited;
	unsigned int _size;
	ConnectionList _busyList;
	ConnectionList _idleList;
	pthread_mutex_t _mutex;
	pthread_cond_t _cond;
};

#endif
