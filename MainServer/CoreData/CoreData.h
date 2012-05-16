//核心数据

#ifndef COREDATA_H
#define COREDATA_H
#include <string>
#include <map>
#include <vector>
#include "Mutex.h"
#include "OurDef.h"
#include "GWProxy.h"
#include "ConnectionPool.h"
#include "Map.h"
#include  "Account.h"
#include "Team.h"
#include "ArchvPK.h"
#include "./Friend/FriendSvc.h"
#include "./Trade/ArchvTrade.h"

using namespace std;


class CoreData
{
public:
	typedef map<UInt32, UInt32> MAPROLEMAPID;
	typedef map<UInt32, Account> MAPAccount;
	typedef map<UInt32,Team> MAPROLETEAM;
	typedef map<UInt32,UInt32> MAPROLETEAMID;
public:
	CoreData(ConnectionPool * cp );
	~CoreData();
	void StopAndWait();
	int Init(void * service);
	
	void ProcessIsAdult();
	//------------S_C 交易-----------------------------------------
	void NotifyNpcStatus( UInt32 roleID );

	void NotifyAllRoleInfo(UInt32 roleID);

	void NotifyRoleAddLevl(UInt32 roleID);//[MsgType:206]角色升级通知
	
	//----业务处理-------------------------------------------------
	int ProcessRoleLogin( UInt32 roleID, string passwd, UInt64 clientConnID );
	int ProcessRoleLogout( UInt32 roleID );
	RolePtr ProcessGetRolePtr( UInt32 roleID );

	int RoleTeamset(UInt32 roleID,UInt32 teamID,UInt32 leaderID,Byte flag);

	Int32 RoleExpAdd(UInt32 roleID,Int32 input);

	int TradeInfo(TradeItem tradeItem,UInt32 roleID);//交易信息
	Monster ProcessGetMonster( int mapID, UInt32 mID );//怪物相关
	int ProcessRoleEnterMap( UInt32 roleID, UInt32 newMapID, UInt16 X, UInt16 Y);
	int ProcessRoleMove( UInt32 roleID, ArchvRoute &art );
	int ProcessCalcAllRolePos(UInt32 roleID );
	int ChangeCreatureStatus( int mapID, List<ArchvCreatureStatus>& lcs );//怪物相关

	UInt32 FromNameToFindID(UInt32 RoleID,string Name);
	Byte GetAccount(UInt32 roleID,UInt32 & LastloginTime,UInt32 & TopTime);
	Byte GetAccountAccountID(UInt32 AccountID,UInt32 & LastloginTime,UInt32 & TopTime);
	int UpdateAccount(UInt32 roleID,Account& l);
	int AccountIsAdult( UInt32 accountID, Byte  isAdult ,string Name,string cardID);
	int AccountPlaytimeAccess( UInt32 accountID ); 

	ArchvPosition GetSenceMonster(UInt32 mapID,UInt32 MonsterID,list<SenceMonster>& Monsters);//获取怪具体状况,
	list<ItemList> GetPublicDrop(UInt32 mapID,char type);
	
	void updateToptime(UInt32 accountID);
	void NotifyCtAdult(List<UInt32> &it,Byte Type);
	void NotifyCtAdultTOflag(List<UInt32> &it);
	void NotifyRoleExp(UInt32 roleID,UInt32 input);
	void NotifyRoleLev(UInt32 roleID,List<UInt32>& it);
	void NotifyEnterMap(list<UInt32>& itor,UInt32 mapID,UInt16 x,UInt16 y);
	void Notifyinfo(UInt32 roleID);

	void GetRoleIDs( list<UInt32>& lrid );
	void GetRoleIDs( UInt32 roleID, list<UInt32>& lrid );
	void GetMapRoleIDs( UInt32 mapID, list<UInt32>& lrid );
	void GetMapRoleIDs( UInt32 mapID, UInt32 roleID, list<UInt32>& lrid );
	
		
	int GetRoleTeamID(UInt32 RoleID,UInt32& teamID);//return -1没有找到，0成功
	Team GetTeams(UInt32 teamID);//返回team;
	Team GetTeamsFromRoleID(UInt32 RoleID);
	
	int AddTeamRole(UInt32 teamID,UInt32 RoleaID);//增加成员
	int CreateTeam(UInt32 LeaderRoleID,UInt32 RoleID);//创建队伍
	int DeleteTeam(UInt32 TeamID);//删除队伍
	int DeleteTeamRole(UInt32 teamID,UInt32 RoleID);//删除成员
	int ChangtoBeLeader(UInt32 teamID,UInt32 RoleID);//提升队长
	int ChangtoFlag(UInt32 teamID,UInt32 roleID,Byte type);
	int LogoutToTeam(UInt32 teamID,UInt32 roleID);

	void AddPKBrif(UInt32 pkID,PKBriefList& pkinfo);
	int DeletePkBrif(UInt32 pkID);
	void AddRolePKID(UInt32 pkID,PKBriefList& pkinfo);
	void deleterolePKID(PKBriefList pkinfo);

	PKBriefList GetPkInfo(UInt32 pkID);

	int FindRoleMapID( UInt32 roleID );
	int GetRoleMapID( UInt32 roleID);

	UInt32 IfRoleInPK(UInt32 roleID);//查看角色是否在PK

	void EraseRolePKID(UInt32 roleID);
		
private:
	int CheckMapID( UInt32 mapID );
	
	void EraseRoleMapID( UInt32 roleID );
	void EraseRoleMapAccount( UInt32 roleID);
	void UpdateRoleMapID( UInt32 roleID, UInt32 mapID );
	void AddRoleMapID( UInt32 roleID, UInt32 mapID );
	
	
	void EraseRoleIDTeamID(UInt32 roleID);

	static void * OnTimerIsAdult(void * object);
	static void HandleEvent1( void * obj, void * arg, int argLen);

private:
	//----------系统属性--------
	
	//线程锁
	mutable MutexLock mutex_;

	ConnectionPool *_cp;

	//主业务处理
	MainSvc * _mainSvc;

	//----------业务属性--------
	//有效地图总数
	int _mapCount;
	
	//地图数组指针
	vector<Map*> _vecMap;

	//roleID,mapID的映射
	MAPROLEMAPID _mapRoleMapID;

	//账号信息,roleID,和Account的映射
	MAPAccount _mapAccount;
	
	//组队情况
	MAPROLETEAM _mapTeam;

	//ROLEID与teamID的映射
	MAPROLETEAMID _mapRoleTeam;

	//pk记录数据
	map<UInt32,PKBriefList> _mapPkInfo;

	map<UInt32,UInt32> _mapRolePKID;
	
	//teamID编号
	UInt32 lastTeamID;
	
	//停止标志	true 停止    false 运行,Map中检测防沉迷的，搬到CoreData中
	bool _isStop;
	
	pthread_t _thrTimerRoleIsAdult;//防沉迷的检测
	
	int _intervalIsAdult;//防沉迷检测的间隔时间

	std::list<NewTimer*> _timerList;
};


#endif


