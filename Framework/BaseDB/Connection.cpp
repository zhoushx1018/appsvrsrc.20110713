#include "Connection.h"

#include "ConnectionPool.h"

Connection::Connection()
	:_handle(NULL)
	,_pool(NULL)
{
}

Connection::~Connection()
{
	Destroy();
}

Connection::Connection(const Connection& conn)
{
	Destroy();

	_pool = conn._pool;
	_handle = conn._handle;

	Connection& temp = const_cast<Connection&>(conn);
	temp._handle = NULL;
	temp._pool = NULL;
}

void Connection::operator=(const Connection& conn)
{
	Destroy();

	_pool = conn._pool;
	_handle = conn._handle;

	Connection& temp = const_cast<Connection&>(conn);
	temp._handle = NULL;
	temp._pool = NULL;
}

Connection::operator MYSQL*()
{
	return _handle;
}

Connection::operator MYSQL*() const
{
	return _handle;
}

long Connection::SetTransation()
{
	//static char strSql[] = "";
	//return mysql_real_query(_handle,strSql,sizeof(strSql)-1);
}

long Connection::Begin()
{
	static char strSql[] = "BEGIN";
	return mysql_real_query(_handle,strSql,sizeof(strSql)-1);
}

long Connection::Commit()
{
	static char strSql[] = "COMMIT";
	return mysql_real_query(_handle,strSql,sizeof(strSql)-1);
}

long Connection::Rollback()
{
	static char strSql[] = "ROLLBACK";
	return mysql_real_query(_handle,strSql,sizeof(strSql)-1);
}

void Connection::Destroy()
{
	if(_pool&&_handle)
	{
		_pool->SetIdleHandle(_handle);
	}
}


MYSQL * Connection::GetHandle()const
{
	return _handle;
}

