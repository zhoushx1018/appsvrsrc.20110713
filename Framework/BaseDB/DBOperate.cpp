#include <stdlib.h>
#include <string.h>
#include "mysql.h"
#include "DBOperate.h"
#include "Log.h"

DBOperate::DBOperate()
:_mysql(NULL)
,_results(NULL)
,_rowNum(0)
,_fieldNum(0)
,_affectRows(0)
{
}

DBOperate::~DBOperate()
{
	FreeResult();
}

//设置句柄
void DBOperate::SetHandle(MYSQL * input)
{
	_mysql = input ;
}

//释放数据库资源
//	同一 DBOperate 对象，做多个 select
//	下一个select 之前应显示调用该 FreeResult()，以释放结果集
void DBOperate::FreeResult()
{
	if( _results)
	{
		mysql_free_result(_results);
		_results = NULL;
	}
}

//数据库 select
//param ptrSql sql语句
//return  0 成功   1 数据库记录不存在  其他返回值 失败
int DBOperate::QuerySQL( const char * ptrSql )
{
	return Operate( ptrSql, true );
}

//数据库 updaet,insert
//param ptrSql sql语句
//return  0 成功  否则 失败 
int DBOperate::ExceSQL( const char * ptrSql )
{
	return Operate( ptrSql, false );
}


//数据库操作
//param ptrSql sql语句
//param hasResults		是否有结果集
//return  0 成功   1 数据库记录不存在  其他返回值 失败

int DBOperate::Operate( const char * ptrSql, bool hasResults )
{
	//释放先前执行的结果集
	FreeResult();

	if( NULL == _mysql )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"invalid mysql connection!!!! DBOperate::_mysql is NULL,ptrSql[%s] ", ptrSql);
		return -1;
	}

	if( mysql_real_query( _mysql, ptrSql, strlen(ptrSql) ) )
		return -1;

	if( hasResults )
	{//select 语句处理

		//存储结果集
		_results = mysql_store_result(_mysql);
		if( NULL == _results )
			return -1;

		//记录数
		_rowNum = mysql_num_rows(_results);

		//第一行记录
		_record = mysql_fetch_row(_results);

		//列数
		_fieldNum = mysql_num_fields(_results);

		//记录不存在，返回 1
		if( _rowNum == 0 )
			return 1;
	}
	else
	{//update,insert 语句处理
		_affectRows = mysql_affected_rows( _mysql );
		if( -1 == (LONG)_affectRows )
			return -1;
	}


	return 0;
}


//获取记录数
UInt32 DBOperate::RowNum()
{
	return _rowNum;
}

//获取字段数
UInt32 DBOperate::FieldNum()
{
	return _fieldNum;
}

//获取影响记录数
Int64 DBOperate::AffectRows()
{
	return _affectRows;
}

//mysql的 last insert id, 用于 auto increasement列
UInt32 DBOperate::LastInsertID()
{
	return mysql_insert_id(_mysql);
}




//0...n-1列
int DBOperate::GetIntField(int nField, int nNullValue)
{
	if ( NULL == _results ||
			 (nField + 1 > _fieldNum ) ||
			  NULL == _record ||_record[nField]==NULL)
  	return nNullValue;

	return atoi(_record[nField]);
}


const char* DBOperate::GetStringField(int nField, const char* szNullValue)
{
	if ( NULL == _results ||
			 (nField + 1 > _fieldNum ) ||
			  NULL == _record )
		return szNullValue;

	return _record[nField];
}


double DBOperate::GetFloatField(int nField, double fNullValue)
{
	if ( NULL == _results ||
		 (nField + 1 > _fieldNum ) ||
		  NULL == _record )
	return fNullValue;

	return atof(_record[nField]);
}


//是否仍有数据记录 
//return true 是  false 否
bool DBOperate::HasRowData()
{
	if( NULL == _record )
		return false;

	return true;
}

//下一行
void DBOperate::NextRow()
{
	if( NULL == _results )
		return;
		
	_record = mysql_fetch_row(_results);
}


