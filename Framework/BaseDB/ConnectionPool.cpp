#include <sys/time.h>
#include <iostream>
#include "ConnectionPool.h"
#include "Log.h"

ConnectionPool::ConnectionPool(unsigned int size)
	:_size(size)
	,_inited(false)
{
	pthread_mutex_init(&_mutex,NULL);
	pthread_cond_init(&_cond,NULL);
}

ConnectionPool::~ConnectionPool()
{
	//释放资源前，应加锁
	pthread_mutex_lock(&_mutex);
	for(ConnectionList::iterator iter=_idleList.begin();iter!=_idleList.end();++iter)
	{
		mysql_close(*iter);
	}

	for(ConnectionList::iterator iter=_busyList.begin();iter!=_busyList.end();++iter)
	{
		mysql_close(*iter);
	}
	pthread_mutex_unlock(&_mutex);

	pthread_mutex_destroy(&_mutex);
	pthread_cond_destroy(&_cond);
}

bool ConnectionPool::Connect(const char * host, const char * user ,const char * password , const char * database)
{
	if(_inited) return true;

	for(unsigned int i=0;i<_size;i++)
	{
		MYSQL* handle = mysql_init(NULL);
		
		if(!handle)
			return false;

		my_bool value = 1;
		if(mysql_options(handle,MYSQL_OPT_RECONNECT,&value))
			return false;

		if(mysql_options(handle,MYSQL_SET_CHARSET_NAME,"utf8"))
			return false;

		if(mysql_options(handle,MYSQL_REPORT_DATA_TRUNCATION,&value))
			return false;

		if(!mysql_real_connect(handle,host,user,password,database,0,NULL,CLIENT_MULTI_STATEMENTS))
			return false;

		_idleList.push_back(handle);
	}

	_inited = true;

	return true;
}

void ConnectionPool::SetIdleHandle(MYSQL* handle)
{
	if(handle!=NULL)
	{
		pthread_mutex_lock(&_mutex);
		_busyList.remove(handle);
		_idleList.push_back(handle);
		pthread_mutex_unlock(&_mutex);
		pthread_cond_signal(&_cond);
	}
}

Connection ConnectionPool::GetConnection()
{
	pthread_mutex_lock(&_mutex);
	Connection con;

	while(_idleList.size()==0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"warning !! DB idle connect is 0!!!!!!!!!" );
		LOG(LOG_FATAL,__FILE__,__LINE__,"warning !! DB idle connect is 0!!!!!!!!!" );
		pthread_cond_wait(&_cond,&_mutex);
	}

	con._handle = _idleList.front();
	_idleList.pop_front();
	_busyList.push_back(con._handle);

	con._pool = const_cast<ConnectionPool*>(this);

	pthread_mutex_unlock(&_mutex);

	return con;
}

int ConnectionPool::GetBusyListSize()
{
	return _busyList.size();
}

int ConnectionPool::GetIdleListSize()
{
	return _idleList.size();
}



