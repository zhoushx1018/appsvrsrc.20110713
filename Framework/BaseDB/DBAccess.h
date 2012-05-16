#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <mysql.h>


#ifndef _MYSQL_PROCESS_BASE_
#define _MYSQL_PROCESS_BASE_

#define MAX_CONNECT_COUNT 10

struct MYSQL_BB
{
	int			m_nIsConnect;		//是否已经连接上,0表示没有连接
	MYSQL 		m_mysql;			//mysql连接对象
	MYSQL_RES 	*m_res;				//数据集指针
};

class DBAccess
{
public:
	DBAccess()
	{
		memset(m_szHostIP, 0, sizeof(m_szHostIP));
		memset(m_szUserName, 0, sizeof(m_szUserName));
		memset(m_szPasswd, 0, sizeof(m_szPasswd));
		memset(m_szDefDBName, 0, sizeof(m_szDefDBName));
	};

	
	~DBAccess()
	{
		for(int i=0; i<m_connectcount; i++)
		{
			if(m_MYSQL_BB[i].m_nIsConnect == 0)
				continue;
			if(m_MYSQL_BB[i].m_res != NULL)
			{
				mysql_free_result(m_MYSQL_BB[i].m_res);
				m_MYSQL_BB[i].m_res = NULL;
			}
			mysql_close(&(m_MYSQL_BB[i].m_mysql));
		}
	};

	void nInit(char* szHostIP, char* szUserName, char* szPasswd, char* szDefDBName, int connectcount)
	{
		strncpy(m_szHostIP, szHostIP, sizeof(m_szHostIP)-1);
		strncpy(m_szUserName, szUserName, sizeof(m_szUserName)-1);
		strncpy(m_szPasswd, szPasswd, sizeof(m_szPasswd)-1);
		strncpy(m_szDefDBName, szDefDBName, sizeof(m_szDefDBName)-1);
		if(connectcount > MAX_CONNECT_COUNT)
			m_connectcount = MAX_CONNECT_COUNT;
		else
			m_connectcount = connectcount;
		m_Index = 0;
		for(int i=0; i<m_connectcount; i++)
		{
			mysql_init(&(m_MYSQL_BB[i].m_mysql));
			m_MYSQL_BB[i].m_res = NULL;
			m_MYSQL_BB[i].m_nIsConnect = 0;
		}
	};

	int nConnect();

	MYSQL_RES* nSelect(char *szSQL);

	int nUpdate(char *szSQL);
	
private:
	int		m_connectcount;
	int		m_Index;
	MYSQL_BB	m_MYSQL_BB[MAX_CONNECT_COUNT];

	char 	m_szHostIP[16+1];
	int 	m_nPort;					//暂时不实现
	char	m_szUserName[64+1];
	char	m_szPasswd[64+1];
	char	m_szDefDBName[32+1];
};


#endif


