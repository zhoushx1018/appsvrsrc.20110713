
//地图

#ifndef MAP_H
#define MAP_H
#include <string>
#include <map>
#include <vector>
#include "Mutex.h"
#include "OurDef.h"
#include "GWProxy.h"
#include <json/json.h>
#include "ArchvMap.h"
#include "ArchvBagItemCell.h"
#include "ConnectionPool.h"
#include "./Trade/ArchvTrade.h"
#include "TimerManager.h"
#include "Role.h"

using namespace std;

class MainSvc;
class Role;
class Monster;
class Npc;
class ArchvRoute;
class ArchvPosition;
class ArchvCreatureStatus;

//具体的地图
class Map
{
public:
	friend class CoreData;
	typedef	map<UInt32, RolePtr> MAPROLE;
	typedef	map<UInt32, Monster*> MAPMONSTER;
	typedef	map<UInt32, Npc*> MAPNPC;
	typedef map<UInt32,SenceMonsterAll> MapMonsters;  
	typedef map<char,list<ItemList> > MapItemDorp;
  

public:
	Map();
	
	~Map();

	void StopAndWait();
	static double CalcPosDistanceExact( const ArchvPosition &pos1, const ArchvPosition &pos2 );
	void ProcessAllRoleCache2DB();
	void ProcessCalcAllRolePos();
	void ProcessCheckAllMonsterStatus();//怪物相关
	void ProcessMpHpTimeadd();
	UInt32 MapID();

protected:
	int Init( void * service, UInt32 mapID, ConnectionPool *cp);
	Json::Value GetJsonValue( const Json::Value& jValue, const string& key );
	void LeftTrim( string& output);
	int InitNpc(const string& jConfigScene );
	int InitMonster(const string& jConfigScene, const string& jConfigMonster );//怪物相关

	//--------------业务处理---------------------------------------
	int ProcessRoleEnterMap( RolePtr &role );
	RolePtr ProcessRoleExitMap( UInt32 roleID );
	RolePtr ProcessGetRolePtr( UInt32 roleID );
	Monster ProcessGetMonster( UInt32 mID );//怪物相关
	void ProcessRoleMove( UInt32 roleID, ArchvRoute &art );
	void ProcessNotifyAllRoleInfo(UInt32 roleID);

	void TradeInfo(TradeItem tradeItem,UInt32 roleID);



	UInt32 FromNameTOFindID(string Name);//返回RoleID

	

	void GetRoleIDs( list<UInt32>& lrid );
	void GetRoleIDs( UInt32 roleID, list<UInt32>& lrid );

	list<ItemList> GetPublicDrop(char type);//获取公共掉落的物品
	ArchvPosition GetSenceMonster(UInt32 MonsterID,list<SenceMonster>& Monsters);//获取怪具体状况,
	int GetallTheNum(UInt32 ID,list<UInt32> lis);//判断lis中是否有ID，有的话，返回1，没有的话返回0

	void GiveVipGiftOnEnterMap(RolePtr& role);
	
private:
	static void TimerCBCalcAllRolePos( void * obj, void * arg, int argLen );
	static void TimerCBCache2DB( void * obj, void * arg, int argLen );
 	static void TimerCBCheckMonsterStatus( void * obj, void * arg, int argLen );
	static void TimerCBHpMpadd( void * obj, void * arg, int argLen );
	static void TimerCBS2C204( void * obj, void * arg, int argLen );
		
	//static void * OnTimerIsAdult(void * object);
	void MakeSCPkgForRole( RolePtr& role, Byte status, List<ArchvCreatureStatus>& lcs );
	void MakeSCPkgForMonster( SenceMonsterAll& mst, Byte status, List<ArchvCreatureStatus>& lcs );//怪物相关
	int SetRoleStatus( UInt32 roleID, Byte status );
	int SetMonsterStatus( UInt32 monsterID, Byte status );//怪物相关

	//------------S_C 消息-----------------------------------------
	void NotifyRoleMove( UInt32 roleID, Byte moveType, ArchvRoute &art );
	void NotifyAllRoleInfo( UInt32 roleID );
	void NotifyCorrectRoute( UInt32 roleID, ArchvRoute &rt );
	void NotifyCtStatusUnicast( UInt32 roleID, Byte flag);
	void NotifyCtStatusBroadcast( List<ArchvCreatureStatus> &lcs );
	//	/void NotifyCtAdult(List<UInt32> &it,Byte Type);
	void NotifyRoleAddLevl(UInt32 roleID);
	void NotifyHPandMP(UInt32 roleID, UInt32 hp, UInt32 mp);


private:
	//----------系统属性---------
	//定时器系统
	TimerManager _tm;
	
	//线程锁
	mutable MutexLock mutex_;

	//DB连接池
	ConnectionPool *_cp;
	
	//停止标志	true 停止    false 运行
	bool _isStop;

	//主业务处理
	MainSvc * _mainSvc;

	//定时任务时间:		定时计算角色当前位置
	int _intervalCalcRolePos;

	//定时任务时间:		角色缓存入库
	int _intervalCache2DB;

	//自动回血 时间间隔
	int _intervalHpMpadd;

	//定时任务时间:		检查小怪状态时间间隔
	int _intervalCheckMonsterStatus;

	//小怪复活时间间隔
	int _intervalMonsterRevive;
	
	//int _intervalIsAdult;//防沉迷检测的间隔时间
	
	//----------业务属性---------
	//地图编号
	UInt32 _mapID;

	//角色数据集
	MAPROLE _mapRole; //Guarded by mutex_
	
	//怪数据集
	MAPMONSTER _mapMonster;

	//Npc数据集
	MAPNPC _mapNpc;

	//怪物具体数据

	MapMonsters _mapsenceMonster;

	//公共物品掉落情况
	MapItemDorp _mapItemdrop;

	
	
	
};


#endif


