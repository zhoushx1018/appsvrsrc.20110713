//DB的 select 、update 操作
//不加入线程锁

#ifndef DBOPERATE_h
#define DBOPERATE_h

#include "mysql.h"
#include "OurDef.h"

class DBOperate
{
public:
	DBOperate();
	~DBOperate();


	void FreeResult();


	void SetHandle( MYSQL* input);
	


	int QuerySQL( const char * ptrSql );


	int ExceSQL( const char * ptrSql );
	

	int Operate( const char * ptrSql, bool hasResults = false );


	UInt32 RowNum();


	UInt32 FieldNum();


	Int64 AffectRows();
	

	UInt32 LastInsertID();


	int GetIntField(int nField, int nNullValue=0);
	const char* GetStringField(int nField, const char* szNullValue="");
  double GetFloatField(int nField, double fNullValue=0.0);


	bool HasRowData();


	void NextRow();

private:
	//msyql 连接句柄
	MYSQL* _mysql;
	
	//结果集
	MYSQL_RES*  _results;

	//单条记录
	MYSQL_ROW  _record;

	//记录数
	UInt32 _rowNum;

	//字段数
	UInt32 _fieldNum;

	//影响记录数
	ULONG _affectRows;
	
};


#endif


