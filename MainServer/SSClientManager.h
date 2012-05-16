//访问其他服务器 管理类

#ifndef SSCLIENTMANAGER_H
#define SSCLIENTMANAGER_H
#include <string>
#include <map>
#include "Mutex.h"
#include "OurDef.h"
#include "List.h"


using namespace std;


class SSClient;
class ArchvRolePKInfo;
class ArchvRoleInfo;

class ServerInfo
{
public:
	ServerInfo():svrType(0),svrSeq(0), port(0)
	{}
public:
	Byte svrType;
	Byte svrSeq;
	string ip;
	UInt16	port;
};

class SSClientManager
{
public:
	SSClientManager(char* cfg);
	~SSClientManager();

	//初始化
	int Init();


	//------------svrType 1  main server-----------------------------------------
	int	ProcessGetRoleInfo(	UInt32 roleID, ArchvRoleInfo & ari) ;

	//------------svrType 2  PK server-----------------------------------------
	int	ProcessPkReq(	UInt32 mapID,	UInt16 X,	UInt16 Y, List<ArchvRolePKInfo> &lpk, UInt32 &pkID );

	
private:
	
	int GetConf(const char* cfg);

	SSClient * GetClientHandle( Byte svrType, Byte svrSeq );

private:
	//线程锁
	mutable MutexLock mutex_;
	
	//服务器列表
	//	(数组下标+1)表示 svrType
	SSClient	* _arrClient[MAX_SVRTYPE][MAX_SVRSEQ];

	//部署的最大 服务器序号
	int _maxSvrSeq[MAX_SVRTYPE];

	//部署的当前 服务器序号
	int _currSvrSeq[MAX_SVRTYPE];

	//参数文件
	char* _cfg;

	//服务器信息
	list<ServerInfo> _listServer;

};


#endif


