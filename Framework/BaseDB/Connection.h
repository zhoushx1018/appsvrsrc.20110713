#ifndef CONNECTION_H
#define CONNECTION_H

#include <mysql.h>

class ConnectionPool;

class Connection
{
public:
	friend class ConnectionPool;
public:
	Connection();
	
	~Connection();

	Connection(const Connection& conn);

	void operator=(const Connection& conn);

	operator MYSQL*();

	operator MYSQL*() const;

	long SetTransation();

	long Begin();

	long Commit();

	long Rollback();

	MYSQL * GetHandle()const;

private:
	void Destroy();

private:
	MYSQL* _handle; 
	ConnectionPool* _pool;
};


#endif

