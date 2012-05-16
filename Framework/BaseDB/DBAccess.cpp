#include "Log.h"
#include "DBAccess.h"

int DBAccess::nConnect()
{
	for(int i=0; i<m_connectcount; i++)
	{
		if(NULL == mysql_real_connect(&m_MYSQL_BB[i].m_mysql,m_szHostIP,m_szUserName,m_szPasswd,m_szDefDBName,0,NULL,0))
		{
			LOG (LOG_ERROR,"mysql_real_connect return NULL, errno=%d,%s", 
				mysql_errno(&m_MYSQL_BB[i].m_mysql), mysql_error(&m_MYSQL_BB[i].m_mysql));
			m_MYSQL_BB[i].m_nIsConnect = 0;
			return -1;
		}
		else
		{
			//mysql_options(&(m_MYSQL_BB[i].m_mysql), MYSQL_OPT_RECONNECT, NULL);
			LOG (LOG_DEBUG,"mysql_real_connect[%d] success!", i);
			m_MYSQL_BB[i].m_nIsConnect = 1;
		}
	}
	
	return 0;
}

MYSQL_RES* DBAccess::nSelect(char* szSQL)
{
	int nRtn = 0;

	for(int index=0; index<m_connectcount; index++)
	{
		if(index >= m_connectcount)
		{
			LOG (LOG_ERROR,"Error, all of the mysql connection lost![%d]", m_connectcount);
			return NULL;
		}
		
		m_Index++;
		if(m_Index >= m_connectcount)
			m_Index = 0;
			
		if(m_MYSQL_BB[m_Index].m_nIsConnect != 1)
		{
			continue;
		}
		
		//释放上次请求内容
		if(m_MYSQL_BB[m_Index].m_res != NULL)
		{
			mysql_free_result(m_MYSQL_BB[m_Index].m_res);
			m_MYSQL_BB[m_Index].m_res = NULL;
		}
		
		nRtn = mysql_real_query(&m_MYSQL_BB[m_Index].m_mysql,szSQL,strlen(szSQL));
		if(nRtn == 0)
		{
			break;
		}
		else
		{
			LOG (LOG_ERROR,"mysql_real_query[%s] return %d, errno=%d,%s", 
				szSQL, nRtn, mysql_errno(&m_MYSQL_BB[m_Index].m_mysql), 
				mysql_error(&m_MYSQL_BB[m_Index].m_mysql));
			if(!mysql_ping(&m_MYSQL_BB[m_Index].m_mysql))
			{
				LOG (LOG_ERROR,"mysql_ping error! reconnect!");
				if(NULL == mysql_real_connect(&m_MYSQL_BB[m_Index].m_mysql,m_szHostIP,m_szUserName,m_szPasswd,m_szDefDBName,0,NULL,0))
				{
					LOG (LOG_ERROR,"mysql_real_connect return NULL, errno=%d,%s", 
						mysql_errno(&m_MYSQL_BB[m_Index].m_mysql), 
						mysql_error(&m_MYSQL_BB[m_Index].m_mysql));
					m_MYSQL_BB[m_Index].m_nIsConnect = 0;
				}
				else
				{
					LOG (LOG_DEBUG,"mysql_real_connect[%d] success!", m_Index);
					m_MYSQL_BB[m_Index].m_nIsConnect = 1;
				}
				continue;
			}
			else
			{
				continue;
			}
		}
		
	}
	
	m_MYSQL_BB[m_Index].m_res = mysql_store_result(&m_MYSQL_BB[m_Index].m_mysql);

	return m_MYSQL_BB[m_Index].m_res;
}

int DBAccess::nUpdate(char *szSQL)
{
	int nRtn = 0;
	
	for(int index=0; index<m_connectcount; index++)
	{
		if(index >= m_connectcount)
		{
			LOG (LOG_ERROR,"Error, all of the mysql connection lost![%d]", m_connectcount);
			return -1;
		}
		
		m_Index++;
		if(m_Index >= m_connectcount)
			m_Index = 0;
			
		if(m_MYSQL_BB[m_Index].m_nIsConnect != 1)
		{
			continue;
		}

		//释放上次请求内容
		if(m_MYSQL_BB[m_Index].m_res != NULL)
		{
			mysql_free_result(m_MYSQL_BB[m_Index].m_res);
			m_MYSQL_BB[m_Index].m_res = NULL;
		}

		nRtn = mysql_real_query(&m_MYSQL_BB[m_Index].m_mysql,szSQL,strlen(szSQL));
		if(nRtn == 0)		
		{
			return 0;
		}
		else
		{
			LOG (LOG_ERROR,"mysql_real_query[%s] return %d, errno=%d,%s", 
				szSQL, nRtn, mysql_errno(&m_MYSQL_BB[m_Index].m_mysql), 
				mysql_error(&m_MYSQL_BB[m_Index].m_mysql));
			if(!mysql_ping(&m_MYSQL_BB[m_Index].m_mysql))
			{
				LOG (LOG_ERROR,"mysql_ping error! reconnect!");
				if(NULL == mysql_real_connect(&m_MYSQL_BB[m_Index].m_mysql,m_szHostIP,m_szUserName,m_szPasswd,m_szDefDBName,0,NULL,0))
				{
					LOG (LOG_ERROR,"mysql_real_connect return NULL, errno=%d,%s", 
						mysql_errno(&m_MYSQL_BB[m_Index].m_mysql), 
						mysql_error(&m_MYSQL_BB[m_Index].m_mysql));
					m_MYSQL_BB[m_Index].m_nIsConnect = 0;
				}
				else
				{
					LOG (LOG_DEBUG,"mysql_real_connect[%d] success!", m_Index);
					m_MYSQL_BB[m_Index].m_nIsConnect = 1;
				}
				continue;
			}
			else
			{
				continue;
			}
		}
	}
	
	return -1;
}


