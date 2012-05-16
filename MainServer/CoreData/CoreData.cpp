#include "CoreData.h"
#include "MainSvc.h"
#include "Log.h"
#include <stdlib.h>
#include <math.h>
#include "DBOperate.h"
#include "Map.h"
#include "IniFile.h"
#include "Role.h"
#include "Monster.h"
#include "Account.h"
#include <pthread.h>
#include "Team.h"
#include "./Team/TeamSvc.h"
#include "./RoleInfo/RoleInfoSvc.h"
#include "ArchvPK.h"
#include "NewTimer.h"


struct RoleAdultTimerCallBackParam1
{
	int roleID;
	int type;
};


CoreData::CoreData(ConnectionPool * cp)
:mutex_()
,_mainSvc(NULL)
,_cp(cp)
,_mapCount(0)
,_intervalIsAdult(60)
//,_intervalIsAdult(30)
//,_intervalIsAdult(5)
,_isStop(true)
,lastTeamID(1)
{}

CoreData::~CoreData()
{
	//释放地图内存
	vector<Map *>::iterator it;
	for( it = _vecMap.begin(); it != _vecMap.end(); it++ )
	{
 		delete *it;
	}
	StopAndWait();

	//所以人的登入时间更新
	for (std::list<NewTimer*>::iterator it = _timerList.begin(); it != _timerList.end(); ++it)
	{
		delete *it;
	}
	_timerList.clear();
}

//初始
//@return 0 成功  非0 失败
int CoreData::Init( void * service )
{
	_isStop=false;
	//获取mainSvc
	_mainSvc = static_cast<MainSvc*>(service);

	//获取参数
	IniFile iniFile;
	string strMapnum;							//地图总个数

	if(!iniFile.open( _mainSvc->Service()->GetConfFilePath()))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"iniFile.open() error!" );
		return -1;
	}

	if( iniFile.read("CoreData","mapnum", strMapnum) ) return -1;
	_mapCount = atoi(strMapnum.c_str());

	//参数校验
	if( _mapCount <= 0 || _mapCount	> MAXMAPNUM )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"error param !!__mapCount[%d]", _mapCount );
		return -1;
	}

	//地图初始化
 	Map * currMap = NULL;
	for( int i = 0; i < _mapCount; i++ )
	{
		currMap = new Map();
		_vecMap.push_back(currMap);
		int iRet = currMap->Init(service,i+1, _cp);
		if(iRet)
			return -1;
	}
	if( pthread_create(&_thrTimerRoleIsAdult,NULL,OnTimerIsAdult ,this))
	{
			LOG (LOG_ERROR, __FILE__, __LINE__, "create threads occurr error.");
 			return -1;
	}
	DEBUG_PRINTF1( "_vecMap.size()[%d]========= \n", _vecMap.size() );
	return 0;

}


//校验地图ID
//@return	0 成功    非0 失败
int CoreData::CheckMapID( UInt32 mapID )
{
	if( mapID <= 0 || mapID > _mapCount )
		return -1;

	return 0;
}

//@brief 查找角色所在 mapID
//return 0 角色不存在 非0 角色所在的MapID
int CoreData::FindRoleMapID( UInt32 roleID )
{
	MutexLockGuard lock(mutex_);
	MAPROLEMAPID::iterator it;
	it = _mapRoleMapID.find(roleID);
	if( _mapRoleMapID.end() != it )
		return it->second;

	return 0;
}
//@brief 查找角色所在 mapID
//return 0 角色不存在 非0 角色所在的MapID
//方便在业务中做查询

int CoreData::GetRoleMapID( UInt32 roleID)
{
		MutexLockGuard lock(mutex_);
		MAPROLEMAPID::iterator it;
		it = _mapRoleMapID.find(roleID);
		if( _mapRoleMapID.end() != it )
			return it->second;

		return 0;
}


//@brief 删除 role,mapId 映射记录
//	role,mapId 映射记录的增删改,只用 EraseRoleMapID, UpdateRoleMapID 即可
//@return	空
void CoreData::EraseRoleMapID( UInt32 roleID )
{
	MAPROLEMAPID::iterator it;

	MutexLockGuard lock(mutex_);
	it = _mapRoleMapID.find(roleID);
	if( _mapRoleMapID.end() != it )
		_mapRoleMapID.erase(it);
}
void CoreData::EraseRoleMapAccount( UInt32 roleID)
{
	MAPAccount::iterator it;

	MutexLockGuard lock(mutex_);
	it = _mapAccount.find(roleID);
	if( _mapAccount.end() != it )
	{
		if(UpdateAccount(roleID,it->second)!=0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"updateAdult erro" );
		}
		_mapAccount.erase(it);
	}
}
void* CoreData::OnTimerIsAdult(void * object)
{
	CoreData * service = static_cast<CoreData *>(object);
	while(!service->_isStop)
	{
		sleep(service->_intervalIsAdult);
		service->ProcessIsAdult();
	}

	return NULL;
}

void CoreData::StopAndWait()
{
	_isStop = true;

}

void CoreData::ProcessIsAdult()
{

	MutexLockGuard lock(mutex_);

	map<UInt32,Account>::iterator it;
	UInt32 alltime=0,nowtime=time(NULL);
	List<UInt32> lirt;
	List<UInt32> lirt1;
	List<UInt32> lirt2;
	List<UInt32> lirt3;
	//LOG (LOG_ERROR, __FILE__, __LINE__, "hello time %d",time(NULL));
	for( it = _mapAccount.begin(); it !=  _mapAccount.end(); it++ )
	{
		//	LOG (LOG_ERROR, __FILE__, __LINE__, "hello time %d",time(NULL));
		if((it->second).IsAdult()==1)
		{
			continue;
		}
		else
		{
			//alltime=nowtime-((it->second).LoginTime())+((it->second).AccountToploginTime());
			alltime=nowtime-(it->second).LoginTime()+(it->second).AccountToploginTime();
			if ((it->second).AccountToploginTime() >= 3600*3)
			{
				LOG (LOG_ERROR, __FILE__, __LINE__, "RoleID %d already 3h Time",time(NULL),it->first);
				continue;
			}
			if (alltime > 3600 && alltime <= 3600 + 60)
			//if (alltime > 30 && alltime <= 30 + 5)
			//if (alltime > 1800 && alltime <= 1800 + _intervalIsAdult)
			{
			//1小时
				LOG (LOG_ERROR, __FILE__, __LINE__, "hello time %d  RoleID %d 1h Time",time(NULL),it->first);
				lirt.push_back(it->first);
			}
			else if (alltime > 3600*2 && alltime <= 3600*2+60)
			//if (alltime > 60 && alltime <= 60 + 5)
			//else if (alltime > 3600 && alltime <= 3600+ _intervalIsAdult)
			{
				LOG (LOG_ERROR, __FILE__, __LINE__, "hello time %d  RoleID %d 2h Time",time(NULL),it->first);
				lirt1.push_back(it->first);	
			}
			else if (alltime > 3600*2 + 60*55 && alltime <= 3600*2 + 60*56)
			//if (alltime >= 60*2 && alltime <= 60*2 + 5)
			//else if (alltime > 3600*2 && alltime <= 3600*2+_intervalIsAdult)
			{
			// 2小时55
				LOG (LOG_ERROR, __FILE__, __LINE__, "hello time %d  RoleID %d 2h55 Time",time(NULL),it->first);
				lirt2.push_back(it->first);
			}
			else if (alltime > 3600*3 && alltime <= 3600*3 +60)
			//else if (alltime >= 3600*3 && alltime <= 3600*3 +5)
			//else if (alltime > 3600*2 && alltime <= 3600*2+_intervalIsAdult)
			{
			//3小时
				LOG (LOG_ERROR, __FILE__, __LINE__, "hello time %d  RoleID %d 3h Time",time(NULL),it->first);
				lirt3.push_back(it->first);
			}
		}
	}

	//结束之后发送广播
	if(lirt.size()>0)
	{
		NotifyCtAdult(lirt,5);
		LOG (LOG_ERROR, __FILE__, __LINE__, "_____" );
	}
	if(lirt1.size()>0)
	{
		NotifyCtAdult(lirt1,6);
		LOG (LOG_ERROR, __FILE__, __LINE__, "_____" );
	}
	if(lirt2.size()>0)
	{
		NotifyCtAdult(lirt2,1);
		LOG (LOG_ERROR, __FILE__, __LINE__, "_____" );
	}
	if(lirt3.size()>0)
	{
		NotifyCtAdult(lirt3,2);
		LOG (LOG_ERROR, __FILE__, __LINE__, "_____" );
	}
}

//@brief 更新 role,mapId 映射记录
//	先删除,再插入		这样做的好处是,不过多判断该映射是否存在相应记录,使得处理流程复杂化
//	role,mapId 映射记录的增删改,只用 EraseRoleMapID, UpdateRoleMapID 即可
//@return	空
void CoreData::UpdateRoleMapID( UInt32 roleID, UInt32 mapID )
{
	EraseRoleMapID(roleID);
	AddRoleMapID(roleID, mapID);
}

//add role map id
void CoreData::AddRoleMapID( UInt32 roleID, UInt32 mapID )
{
	MutexLockGuard lock(mutex_);
	_mapRoleMapID.insert( make_pair( roleID, mapID ));
}

//@brief	角色登陆
//@return	0 成功  非0 失败
int CoreData::ProcessRoleLogin( UInt32 roleID, string passwd, UInt64 clientConnID )
{
	MutexLockGuard lock(mutex_);
	int iRet = 0;
	Byte is=0;
	UInt32 lastlogintime=0,toptime=0;

	//初始化角色缓存
	RolePtr role(new Role());

	iRet = role->InitRoleCache( roleID, clientConnID, _cp );

	if( iRet )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"InitRoleCache error!! roleID[%d]", roleID );
		return -1;
	}

	//密码校验
	//if( role->Password() != passwd )
	//{
	//	LOG(LOG_ERROR,__FILE__,__LINE__,"passwd error!! roleID[%d], passwd[%s]", roleID, passwd.c_str());
	//	return -1;
	//}
	is=GetAccount(roleID,lastlogintime,toptime);
	if (!is)
	{
		//if (toptime >= 60*3)
		if (toptime >= 3600*3)
		{
			if (time(NULL)-lastlogintime < 3600*5)
			//if (time(NULL) - lastlogintime < 60*5)
			{
				NewTimer* pTimer = new NewTimer;
				pTimer->Type(NewTimer::OnceTimer);
				pTimer->Interval(5);
				RoleAdultTimerCallBackParam1 param;
				param.roleID = roleID;
				param.type = 4;
				pTimer->SetCallbackFun(HandleEvent1, this, &param, sizeof(param));
				pTimer->start(&(_mainSvc->_tm));
				_timerList.push_back(pTimer);
			}
			else
			{
				toptime = 0;
			}
		}		
	}
	Account roleaccout(lastlogintime,toptime,is,time(NULL));
	_mapAccount.insert( make_pair( roleID,roleaccout ));
	return 0;
}

 void CoreData::HandleEvent1( void * obj, void * arg, int argLen)
{
	CoreData * currObj = (CoreData *)obj;
	RoleAdultTimerCallBackParam1 param = *(RoleAdultTimerCallBackParam1*)arg;
	List<UInt32> ilist;
	ilist.push_back(param.roleID);
	currObj->NotifyCtAdult(ilist,param.type);
}



//@brief	角色登出
//@return	0 成功  非0 失败
int CoreData::ProcessRoleLogout( UInt32 roleID )
{
	int iRet = 0;
	UInt32 teamID=0;
	//查找角色所在地图
	int mapID = FindRoleMapID( roleID );

	if( 0 == mapID )
	{
//		LOG(LOG_ERROR,__FILE__,__LINE__,"FindRoleMapID error!! roleID[%d]", roleID );
		return -1;
	}

	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!! mapID[%d]", mapID );
		return -1;
	}

	//退出地图,业务处理
	RolePtr pRole=_vecMap[mapID-1]->ProcessRoleExitMap(roleID);
	teamID=pRole->TeamID();
	if(teamID!=0)
	{

		LogoutToTeam(teamID,roleID);
		EraseRoleIDTeamID(roleID);
	}

	if(pRole->Status()==2)
	{
		//正在战斗下线
			UInt32 pkID=0;
			map<UInt32,UInt32>::iterator it1;
			it1=_mapRolePKID.find(roleID);
			if(it1!=_mapRolePKID.end())
			{
				pkID= it1->second;
			}

			PKBriefList pklist;
			map<UInt32,PKBriefList>::iterator it;
			it=_mapPkInfo.find(pkID);
			if(it!=_mapPkInfo.end())
			{
				pklist=it->second;
			}
			if(pklist.player1.size()==1&&pklist.player2.size()==1)
			{
				//双方都是单个，一方下线,只是考虑了人和怪的战斗
				list<UInt32>::iterator it2;
				it2=pklist.player1.begin();
				if(roleID==*it2)
				{
					it2=pklist.player2.begin();

					//改变怪状态
					_vecMap[mapID-1]->SetMonsterStatus(*it2, 1);
				}
			}

	}
  //好友下线通知
   _mainSvc->GetFriendSvc()->OnFriendOffLine(roleID);

	//删除 role,mapID映射
	EraseRoleMapID(roleID);
	EraseRoleMapAccount(roleID);
	return 0;
}


//@brief	获取角色信息
//@return	处理成功  返回 Role.ID不为0的RolePtr
//				处理失败  返回空的RolePtr
RolePtr CoreData::ProcessGetRolePtr( UInt32 roleID )
{
	//查找角色所在地图
	int mapID = FindRoleMapID( roleID );
	if( 0 == mapID )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"FindRoleMapID error!! roleID[%d]", roleID );
		return RolePtr(new Role());
	}

	//mapID 校验
	int iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!!roleID[%d], mapID[%d]", roleID, mapID );
		return RolePtr(new Role());
	}

	return _vecMap[mapID-1]->ProcessGetRolePtr(roleID);
}


//@brief	获取coredata  在线的所有 roleID
void CoreData::GetRoleIDs( list<UInt32>& lrid )
{
	MutexLockGuard lock(mutex_);
	MAPROLEMAPID::iterator it;

	lrid.clear();
	for( it = _mapRoleMapID.begin(); it != _mapRoleMapID.end(); it++ )
	{
		lrid.push_back(it->first);
	}
}

//@brief	获取coredata  在线的所有 roleID
//				不包含参数 roleID 在内
void CoreData::GetRoleIDs( UInt32 roleID, list<UInt32>& lrid )
{
	MutexLockGuard lock(mutex_);
	MAPROLEMAPID::iterator it;

	lrid.clear();
	for( it = _mapRoleMapID.begin(); it != _mapRoleMapID.end(); it++ )
	{
		if( it->first != roleID )
			lrid.push_back(it->first);
	}
}

//@brief	获取角色所在地图  在线的所有 roleID
void CoreData::GetMapRoleIDs( UInt32 mapID, list<UInt32>& lrid )
{
	MutexLockGuard lock(mutex_);
	int iRet = 0;

	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!!mapID[%d]", mapID );
		return;
	}

	lrid.clear();
	return _vecMap[mapID-1]->GetRoleIDs(lrid);
}

//@brief	获取角色所在地图  在线的所有 roleID
//				不包含参数 roleID 在内
void CoreData::GetMapRoleIDs( UInt32 mapID, UInt32 roleID, list<UInt32>& lrid )
{
	MutexLockGuard lock(mutex_);
	int iRet = 0;

	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!!mapID[%d]", mapID );
		return;
	}

	lrid.clear();
	return _vecMap[mapID-1]->GetRoleIDs( roleID, lrid);
}


//@brief	获取怪信息
//@return	处理成功  返回 ID不为0的 对象
//				处理失败  返回 ID为0的 对象
Monster CoreData::ProcessGetMonster( int mapID, UInt32 mID )
{
	MutexLockGuard lock(mutex_);
	int iRet = 0;
	Monster monster;

	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!!mID[%d], mapID[%d]", mID, mapID );
		return monster;
	}
	if(mID>10000000)
	{
		mID=mID%10000000;
		mID=mID/10000;
	}
	return _vecMap[mapID-1]->ProcessGetMonster(mID);
}


list<ItemList> CoreData::GetPublicDrop(UInt32 mapID,char type)
{
		MutexLockGuard lock(mutex_);
		return _vecMap[mapID-1]->GetPublicDrop(type);
}

//@brief	角色更换地图
//@return	0 成功  非0 失败
int CoreData::ProcessRoleEnterMap( UInt32 roleID, UInt32 newMapID, UInt16 X, UInt16 Y)
{
	int iRet = 0;
	list<UInt32> its;
	Byte flag=0;
	list<TeamRole> teamRole;
	list<TeamRole>::iterator itor;
	list<UInt32>::iterator itor1;
	//新的mapID 校验
	iRet = CheckMapID(newMapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"newMapID error!! roleID[%d],newMapID[%d]", roleID, newMapID );
		return -1;
	}

	//查找角色所在地图
	int mapID = FindRoleMapID( roleID );
	if( 0 == mapID )
	{//角色未进入任何地图
		RolePtr pRole(new Role());
		//初始化角色信息,从DB获取角色信息
		iRet = pRole->InitRoleCache( roleID, 0, _cp);
		if( iRet )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"InitRoleCache error!! roleID[%d]", roleID );
			return -1;
		}
		//itor.push_back(roleID)
		//角色登陆时间
		pRole->LoginTime(time(NULL));
		iRet = _vecMap[newMapID-1]->ProcessRoleEnterMap(pRole);

		AddRoleMapID( pRole->ID(), pRole->MapID());
//DEBUG_PRINTF1( "first 111 enter map, role->MapID()[%d] \n", pRole->MapID() );
	}
	else
	{//角色已进入某地图
//DEBUG_PRINTF2( "change 222 map , mapID[%d], newMapID[%d]  \n", mapID, newMapID );
		//地图ID 重复的校验
		if( mapID == newMapID )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"mapID duplicate!!!!! roleID[%d], mapID[%d],newMapID[%d]", roleID, mapID, newMapID );
			return 1;
		}

		//旧的mapID 校验
		iRet = CheckMapID(mapID);
		if(iRet)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!! roleID[%d], mapID[%d]", roleID, mapID );
			return -1;
		}

		//退出旧地图,从旧地图获取role的信息
		RolePtr pRole = _vecMap[mapID-1]->ProcessRoleExitMap(roleID);

		//更新角色位置
		pRole->MapID(newMapID);
		pRole->LastX(X);
		pRole->LastY(Y);
		//更新角色移动路径,方位
		ArchvPosition pos(X,Y);
		ArchvRoute route( time(NULL), pos);
		pRole->Route(route);
		pRole->Pos(pos);
		//没有组队
		iRet = _vecMap[newMapID-1]->ProcessRoleEnterMap(pRole);
		if(pRole->TeamFlag()==2)
		{
			//队长
			flag=1;
			MAPROLETEAM::iterator it;
			MutexLockGuard lock(mutex_);
			it=_mapTeam.find(pRole->TeamID());

			if(_mapTeam.end() != it)
			{
				teamRole =it->second.GetMemberRoleID();
				//找到
				for(itor=teamRole.begin();itor!=teamRole.end();itor++)
				{
					if(itor->status==1)
					{
						//成员都退出地图
						its.push_back(itor->roleId);
					}
				}
			}
			else
			{
				//没有找到
				LOG(LOG_ERROR,__FILE__,__LINE__,"TeamID error or you are not the leader!! roleID[%d], TeamID[%d]", roleID, pRole->TeamID() );
				return -1;
			}
		}
		//更新 role,mapID映射
		UpdateRoleMapID( pRole->ID(), pRole->MapID());
	}


	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"RoleEnterMap error!! roleID[%d]", roleID );
	}

	if (its.size() != 0)
		NotifyEnterMap(its, newMapID, X, Y);
	return 0;
}



//@brief	角色移动
//@return	0 成功  非0 失败
int CoreData::ProcessRoleMove( UInt32 roleID, ArchvRoute &art )
{
	int iRet = 0;

	//查找角色所在地图
	int mapID = FindRoleMapID( roleID );
	if( 0 == mapID )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"FindRoleMapID error!! roleID[%d]", roleID );
		return -1;
	}

	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!! mapID[%d]", mapID );
		return -1;
	}

	//角色地图移动,业务处理
	_vecMap[mapID-1]->ProcessRoleMove(roleID, art);

	return 0;
}

//@brief	刷新角色所在地图所有角色的当前坐标
//@return	无
int CoreData::ProcessCalcAllRolePos(UInt32 roleID )
{
	int iRet = 0;

	//查找角色所在地图
	int mapID = FindRoleMapID( roleID );
	if( 0 == mapID )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"FindRoleMapID error!! roleID[%d]", roleID );
		return -1;
	}

	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!! mapID[%d]", mapID );
		return -1;
	}

	//角色地图移动,业务处理
	_vecMap[mapID-1]->ProcessCalcAllRolePos();

	return 0;
}

int CoreData::TradeInfo(TradeItem tradeItem,UInt32 roleID)
{
	int iRet = 0;
		//查找角色所在地图
	int mapID = FindRoleMapID( roleID );
	if( 0 == mapID )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"FindRoleMapID error!! roleID[%d]", roleID );
		return -1;
	}
		//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!! mapID[%d]", mapID );
		return -1;
	}
		//角色地图移动,业务处理
	_vecMap[mapID-1]->TradeInfo(tradeItem,roleID);

	return 0;
}

UInt32 CoreData::FromNameToFindID(UInt32 roleID,string Name)
{
	int iRet = 0;
	UInt32 ID=0;
	//查找角色所在地图
	int mapID = FindRoleMapID( roleID );
	if( 0 == mapID )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"FindRoleMapID error!! roleID[%d]", roleID );
		return -1;
	}

	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!! mapID[%d]", mapID );
		return -1;
	}

	//角色地图移动,业务处理
	ID=_vecMap[mapID-1]->FromNameTOFindID(Name);
	if(ID==0)
	{
		return 0;
	}
	else
	{
			mapID=FindRoleMapID(ID);
			if( 0 == mapID )
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"FindRoleMapID error!! roleID[%d]", roleID );
				return -1;
			}

			//mapID 校验
			iRet = CheckMapID(mapID);
			if(iRet)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!! mapID[%d]", mapID );
				return -1;
			}
	}

	return ID;
}

//返回1有升级
Int32 CoreData::RoleExpAdd(UInt32 roleID,Int32 input)
{
	int iRet = 0;
	List<UInt32> it;
	//查找角色所在地图
	int mapID = FindRoleMapID( roleID );
	if( 0 == mapID )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"FindRoleMapID error!! roleID[%d]", roleID );

		//直接更新DB
		_mainSvc->GetRoleInfoSvc()->RoleExpAddToDB(roleID,input);
		return 0;
	}

	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!! mapID[%d]", mapID );
		return -1;
	}

	RolePtr pRole = ProcessGetRolePtr(roleID);
	iRet=pRole->RoleExpAdd(input);
	if(iRet>=1)
	{
		NotifyRoleLev(roleID,it);
		Notifyinfo(roleID);
		NotifyRoleAddLevl(roleID);
	}
	else
	{
		NotifyRoleExp(roleID,input);
	}

	//角色地图移动,业务处理
	return iRet;

}
//@Monsters 为空为没有找到
ArchvPosition CoreData::GetSenceMonster(UInt32 mapID,UInt32 MonsterID,list<SenceMonster>& Monsters)
{
	MutexLockGuard lock(mutex_);
	int iRet = 0;
	ArchvPosition pos;
	pos.X=0;
	pos.Y=0;
	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!! mapID[%d]", mapID );
		return pos;
	}

	//角色地图移动,业务处理
	return _vecMap[mapID-1]->GetSenceMonster(MonsterID,Monsters);

}


//@brief	改变生物状态
//@return	0 成功  非0 失败
int CoreData::ChangeCreatureStatus( int mapID, List<ArchvCreatureStatus>& lcs )
{
	int iRet = 0;

	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!! mapID[%d]", mapID );
		return -1;
	}
	//返回0没有找到，返回RoleID
	//修改生物状态
	List<ArchvCreatureStatus>::iterator it;
	for( it = lcs.begin(); it != lcs.end(); it++ )
	{
		if( 1 == it->creatureFlag )
		{//角色
			_vecMap[mapID-1]->SetRoleStatus( it->ID, it->status );
		}
		else if( 2 == it->creatureFlag )
		{//怪物
			_vecMap[mapID-1]->SetMonsterStatus( it->ID, it->status );
		}
	}

	return 0;
}


void CoreData::NotifyNpcStatus( UInt32 roleID )
{
	int iRet = 0;

	//查找角色所在地图
	int mapID = FindRoleMapID( roleID );
	if( 0 == mapID )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"FindRoleMapID error!! roleID[%d]", roleID );
		return;
	}

	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!!roleID[%d], mapID[%d]", roleID, mapID );
		return;
	}

	_vecMap[mapID-1]->NotifyCtStatusUnicast( roleID, 0x04 );

	return;
}

//@brief [MsgType:206] 角色升级广播给场景内所有其他角色
//@param  roleID 角色ID
void CoreData::NotifyRoleAddLevl(UInt32 roleID)
{
	int iRet = 0;

	//查找角色所在地图
	int mapID = FindRoleMapID( roleID );
	if( 0 == mapID )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"FindRoleMapID error!! roleID[%d]", roleID );
		return;
	}

	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!!roleID[%d], mapID[%d]", roleID, mapID );
		return;
	}

	_vecMap[mapID-1]->NotifyRoleAddLevl(roleID);

	return;
}


//@brief [MsgType:0202]（单播）进入场景，场景所有其他角色的列表
//	仅仅发送给玩家角色
//@param	roleID	角色ID
//@return	无
void CoreData::NotifyAllRoleInfo( UInt32 roleID )
{
	int iRet = 0;

	//查找角色所在地图
	int mapID = FindRoleMapID( roleID );
	if( 0 == mapID )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"FindRoleMapID error!! roleID[%d]", roleID );
		return;
	}

	//mapID 校验
	iRet = CheckMapID(mapID);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!!roleID[%d], mapID[%d]", roleID, mapID );
		return;
	}

	_vecMap[mapID-1]->ProcessNotifyAllRoleInfo( roleID );

	return;
}


void CoreData::NotifyCtAdult(List<UInt32> &it,Byte Type)
{
	DataBuffer	serbuffer(1024);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 9902;
	p.UniqID = 218;
	p.PackHeader();
	s<<Type;
	p.UpdatePacketLength();
	if(_mainSvc->Service()->Broadcast(it, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}

	LOG (LOG_ERROR, __FILE__, __LINE__, "__Type[%d]___", Type );
	
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]\n", p.MsgType, it.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );


}
void CoreData::NotifyRoleExp(UInt32 roleID,UInt32 input)
{
	DataBuffer	serbuffer(1024);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	List<UInt32> it;
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 306;
	p.UniqID = 218;
	p.PackHeader();
	s<<input;
	it.push_back(roleID);
	p.UpdatePacketLength();
	if(_mainSvc->Service()->Broadcast(it, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, it.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}
void CoreData::NotifyRoleLev(UInt32 roleID,List<UInt32>& it)
{
	DataBuffer	serbuffer(1024);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 307;
	p.UniqID = 218;
	p.PackHeader();
	s<<roleID;
	it.push_back(roleID);
	p.UpdatePacketLength();
	if(_mainSvc->Service()->Broadcast(it, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, it.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}

void CoreData::NotifyEnterMap(list<UInt32>& itor,UInt32 mapID,UInt16 x,UInt16 y)
{
	DataBuffer	serbuffer(1024);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 205;
	p.UniqID = 218;
	p.PackHeader();
	s<<mapID<<x<<y;
	p.UpdatePacketLength();
	if(_mainSvc->Service()->Broadcast(itor, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, itor.size() );
}

void CoreData::Notifyinfo(UInt32 roleID)
{
			DataBuffer	serbuffer(8196);
			ArchvRoleChange ch;
			int iRet = 0;

	//查找角色所在地图
		int mapID = FindRoleMapID( roleID );
		if( 0 == mapID )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"FindRoleMapID error!! roleID[%d]", roleID );
			return ;
		}

		//mapID 校验
		iRet = CheckMapID(mapID);
		if(iRet)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"mapID error!!roleID[%d], mapID[%d]", roleID, mapID );
			return ;
		}

		RolePtr role = _vecMap[mapID-1]->ProcessGetRolePtr(roleID);
			List<UInt32> it;
			Serializer s( &serbuffer );
			Packet p(&serbuffer);
			serbuffer.Reset();
			p.Direction = DIRECT_S_C_REQ;
			p.SvrType = 1;
			p.SvrSeq = 1;
			p.MsgType = 301;
			p.UniqID = 219;
			p.PackHeader();

			ch.level = role->Level();
			ch.exp = role->Exp();
			ch.maxExp = role->MaxExp();
			ch.glory = role->Glory();
			ch.hp = role->Hp();
			ch.mp = role->Mp();
			ch.maxHp = role->MaxHp();
			ch.maxMp = role->MaxMp();
			ch.attackPowerHigh = role->AttackPowerHigh();
			ch.attackPowerLow = role->AttackPowerLow();
			ch.defence = role->Defence();
			ch.mDefence = role->MDefence();
			ch.critRate = role->CritRate();
			ch.crime  = role->Crime();
			ch.addPoint = role->AddPoint();
			ch.strength = role->Strength();
			ch.intelligence = role->Intelligence();
			ch.agility = role->Agility();
			ch.hitRate = role->HitRate();
			ch.dodgeRate = role->DodgeRate();
			ch.attackSpeed = role->AttackSpeed();
			ch.MPRegen = role->MpRegen();
			ch.HPRegen = role->HpRegen();
			ch.moveSpeed=role->MoveSpeed();

			s<<ch;
			p.UpdatePacketLength();
			it.push_back(roleID);
			if( _mainSvc->Service()->Broadcast( it, &serbuffer))
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
			}

			DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, it.size() );
			DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );

}

Byte CoreData::GetAccount(UInt32 roleID,UInt32 & LastloginTime,UInt32 & TopTime)
{
		char szSql[256];
		Connection con;
		DBOperate dbo;
		int iRet = 0;
		Byte is=0;
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
		sprintf( szSql, "select IsAdult,LastloginTime,TopTime from Account where AccountID=(select AccountID from Role where RoleID=%d)",roleID );
		iRet=dbo.QuerySQL(szSql);
		if( 0 == iRet )
		{
			while(dbo.HasRowData())
			{
				is=dbo.GetIntField(0);
				LastloginTime=dbo.GetIntField(1);
				TopTime=dbo.GetIntField(2);
				dbo.NextRow();
			}
		}
		else
		{
				LOG(LOG_ERROR,__FILE__,__LINE__,"GetAdult erro" );
		}



		return is;
}
//获取account类的成员
Byte CoreData::GetAccountAccountID(UInt32 AccountID,UInt32 & LastloginTime,UInt32 & TopTime)
{
		char szSql[256];
		Connection con;
		DBOperate dbo;
		int iRet = 0;
		Byte is=0;
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
		sprintf( szSql, "select IsAdult,LastloginTime,TopTime from Account where AccountID=%d;",AccountID );
		iRet=dbo.QuerySQL(szSql);
		if( 0 == iRet )
		{
			while(dbo.HasRowData())
			{
				is=dbo.GetIntField(0);
				LastloginTime=dbo.GetIntField(1);
				TopTime=dbo.GetIntField(2);
				dbo.NextRow();
			}
		}
		else
		{
				LOG(LOG_ERROR,__FILE__,__LINE__,"GetAdult erro" );
		}



		return is;
}

int CoreData::UpdateAccount(UInt32 roleID,Account & l)
{
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	Byte is=0;
	UInt32 lastlogintime=time(NULL), toptime=0;

	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());	
	toptime= l.AccountToploginTime()+(lastlogintime-l.LoginTime());	
	sprintf( szSql, "update  Account set LastloginTime=%d,TopTime=%d where AccountID=(select AccountID from Role where RoleID=%d);",lastlogintime,toptime,roleID );
	iRet=dbo.ExceSQL(szSql);
	if(iRet!=0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"updateAdult erro" );
		return -1;
	}
	return 0;
}
void CoreData::NotifyCtAdultTOflag(List<UInt32> &it)
{
	DataBuffer	serbuffer(1024);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 9903;
	p.UniqID = 210;
	p.PackHeader();
	p.UpdatePacketLength();
	if(_mainSvc->Service()->Broadcast(it, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, it.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );



}

//更新是否成人标志。
//return 0成功，非0失败

int CoreData::AccountIsAdult( UInt32 accountID, Byte  isAdult ,string Name,string cardID)
{			LOG(LOG_ERROR,__FILE__,__LINE__,"selectAdult erro" );
			MutexLockGuard lock(mutex_);
			char szSql[1024];
			Connection con;
			DBOperate dbo;
			int iRet = 0;
			Byte flag=0;
			Byte is=0;
			UInt32 ID=0;
			List<UInt32> its;
			MAPAccount::iterator it;
			con = _cp->GetConnection();
			dbo.SetHandle(con.GetHandle());

		//判断帐号是否已经注册

			sprintf( szSql, "select CardID from Account where CardID='%s';",cardID.c_str());
			iRet=dbo.QuerySQL(szSql);
			if(iRet==0)
			{
				return 1;
			}
			else if(iRet!=1)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"selectAdult erro" );
				return -1;
			}

			sprintf( szSql, "select RoleID from Role where AccountID=%d;",accountID);
			iRet=dbo.QuerySQL(szSql);
			if(iRet!=0)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"selectAdult erro" );
				return -1;
			}
			else
			{
					if(iRet==1)
					{
						LOG(LOG_ERROR,__FILE__,__LINE__,"selectAdult erro" );
							return -1;
					}
					else
					{
							while(dbo.HasRowData())
							{
									ID=dbo.GetIntField(0);

									it = _mapAccount.find(ID);
									if( _mapAccount.end() != it )
									{

										(it->second).IsAdult(isAdult);
										flag=1;
										break;
									}

									dbo.NextRow();
							}
					}

			}


			sprintf( szSql, "update  Account set IsAdult=%d,Name='%s',CardId='%s' where AccountID=%d;",isAdult,Name.c_str(),cardID.c_str(),accountID);
			LOG(LOG_ERROR,__FILE__,__LINE__,"the Name %s ",Name.c_str() );
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
			iRet=dbo.ExceSQL(szSql);
			if(iRet!=0)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"updateAdult erro" );
					return -1;

			}

			if(flag==1)
			{
				its.push_back(ID);
				LOG(LOG_ERROR,__FILE__,__LINE__,"selectAdult erro" );
				NotifyCtAdultTOflag(its);
			}
			return 0;

}

		//@brief 是否能通过防成迷标志
    //@return  0成功，非0失败
int CoreData::AccountPlaytimeAccess( UInt32 accountID )
{
	Int32 Flag=0;
	Byte is=0;
	UInt32 lastlogintime=0,toptime=0;
	is=GetAccountAccountID(accountID,lastlogintime,toptime);
	if(is==0)
	{
			if(toptime>=3600*3)//3小时
			{
				if(time(NULL)-lastlogintime>=3600*5)
				{
					Flag=0;
					//gengxin time0
					updateToptime(accountID);
				}
				else
				{
					Flag=1;
				}
			}
			else
			{
				//第一天登入超过5小时也清0
			//第二天登入全部清0

				if((UInt32)((time(NULL)/3600*24)-(UInt32)(lastlogintime/3600*24))>0)
				{
					updateToptime(accountID);
				}
				else
				{
					//同一天
					if((time(NULL)-lastlogintime)>5*3600 )
						updateToptime(accountID);
				}
				Flag=0;
			}
	}
	else
	{
		Flag=0;
	}

	return Flag;
}
void CoreData::updateToptime(UInt32 accountID)
{
			char szSql[1024];
			Connection con;
			DBOperate dbo;
			int iRet = 0;
			Byte is=0;
			con = _cp->GetConnection();
			dbo.SetHandle(con.GetHandle());
			sprintf( szSql, "update  Account set TopTime=0 where AccountID=%d;",accountID );
			iRet=dbo.ExceSQL(szSql);
			if(iRet!=0)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"updateAdult erro" );
			}
			return;
}

int CoreData::GetRoleTeamID(UInt32 RoleID,UInt32& teamID)//return -1没有找到，0成功
{
	MutexLockGuard lock(mutex_);
	MAPROLETEAMID::iterator it;
	it = _mapRoleTeam.find(RoleID);
	if( _mapRoleTeam.end() != it )
	{
			teamID=it->second;
	}
	else
	{
		teamID=0;
		return -1;
	}
	return 0;
}


Team CoreData::GetTeams(UInt32 teamID)//返回team;
{
	MutexLockGuard lock(mutex_);
	Team team;
	MAPROLETEAM::iterator it;
	it=_mapTeam.find(teamID);

	if(_mapTeam.end() != it)
	{
	  return it->second;
	}
	else
	{
		return team;
	}


}


Team CoreData::GetTeamsFromRoleID(UInt32 RoleID)
{
	MutexLockGuard lock(mutex_);
		Team team;
		MAPROLETEAM::iterator it;
		MAPROLETEAMID::iterator itor;
		UInt32 teamID;

		itor= _mapRoleTeam.find(RoleID);
		if( _mapRoleTeam.end() != itor)
		{
			teamID=itor->second;
		}
		else
		{
			return team;
		}

		it=_mapTeam.find(teamID);

		if(_mapTeam.end() != it)
		{
			return it->second;
		}
		else
		{
			return team;
		}

}


int CoreData::AddTeamRole(UInt32 teamID,UInt32 RoleID)//增加成员
{
	MutexLockGuard lock(mutex_);

	MAPROLETEAM::iterator it;
	it=_mapTeam.find(teamID);

	if(_mapTeam.end() != it)
	{
	 	(it->second).AddToTeam(RoleID);
	 	_mapRoleTeam.insert( make_pair(RoleID,teamID) );

	 	RoleTeamset(RoleID,teamID,(it->second).GetLeaderRoleID(),1);//默认成员加入队伍，即是离队状态
	}
	else
	{
	//	return team;
		return -1;
	}
	return 0;
}

//创建
int CoreData::CreateTeam(UInt32 LeaderRoleID,UInt32 RoleID)//创建队伍
{
	MutexLockGuard lock(mutex_);
	Team team;
	team.AddToTeam(LeaderRoleID);
	team.LeaderRoleID(LeaderRoleID);
	team.AddToTeam(RoleID);
	team.TeamID(lastTeamID);
	_mapTeam.insert( make_pair(lastTeamID,team));
	_mapRoleTeam.insert( make_pair(LeaderRoleID,lastTeamID) );
	_mapRoleTeam.insert( make_pair(RoleID,lastTeamID) );
	RoleTeamset(LeaderRoleID,lastTeamID,LeaderRoleID,2);
	RoleTeamset(RoleID,lastTeamID,LeaderRoleID,1);//组成队伍的时候成员默认离队

	lastTeamID++;
	return 0;
}

int CoreData::DeleteTeam(UInt32 TeamID)//删除队伍
{
	UInt32 LeaderID=0;
	list<TeamRole> TeamRole1;
	list<TeamRole>::iterator itor;
	MAPROLETEAM::iterator it;

	it = _mapTeam.find(TeamID);
	if( _mapTeam.end() != it )
	{
		TeamRole1=(it->second).GetMemberRoleID();
		if(TeamRole1.size()!=0)
		for(itor=TeamRole1.begin();itor!=TeamRole1.end();itor++)
		{
				RoleTeamset(itor->roleId,0,0,0);
				EraseRoleIDTeamID(itor->roleId);
		}
		_mapTeam.erase(it);

	}
	else
	{
		return -1;
	}
	return 0;

}

int CoreData::DeleteTeamRole(UInt32 teamID,UInt32 RoleID)//删除成员
{
	MutexLockGuard lock(mutex_);

	MAPROLETEAM::iterator it;
	it=_mapTeam.find(teamID);

	if(_mapTeam.end() != it)
	{
	 	(it->second).OutTheTeam(RoleID);
	 	EraseRoleIDTeamID(RoleID);
	 	RoleTeamset(RoleID,0,0,0);
	 	if((it->second).GetMemberRoleID().size()==0)
	 	{
	 		DeleteTeam(teamID);
	 	}
	}
	else
	{
	//	return team;
		return -1;
	}
	return 0;
}

void CoreData::EraseRoleIDTeamID(UInt32 roleID)
{
	MAPROLETEAMID::iterator it;
	it= _mapRoleTeam.find(roleID);
	if( _mapRoleTeam.end() != it)
	{
		_mapRoleTeam.erase(it);
	}
}

int CoreData::RoleTeamset(UInt32 roleID,UInt32 teamID,UInt32 leaderID,Byte flag)
{
    RolePtr pRole = ProcessGetRolePtr(roleID);

    pRole->TeamID(teamID);
    pRole->TeamFlag(flag);
    pRole->LeaderRoleID(leaderID);

	return 0;
}


int CoreData::ChangtoFlag(UInt32 teamID,UInt32 roleID,Byte type)
{
	MutexLockGuard lock(mutex_);

    RolePtr pRole = ProcessGetRolePtr(roleID);

	MAPROLETEAM::iterator it;
	it=_mapTeam.find(teamID);

	if(_mapTeam.end() != it)
	{

	    pRole->TeamFlag(type);
	 	(it->second).changeRoleStues(roleID,type);
	}
	else
	{
	//	return team;
		return -1;
	}
	return 0;
}

//返回1自己是队长，返回-1，没有找到队伍,返回0成功
int CoreData::ChangtoBeLeader(UInt32 teamID,UInt32 RoleID)//提升队长
{
	MutexLockGuard lock(mutex_);
	list<TeamRole> TeamRole1;
	list<TeamRole>::iterator itor;

	MAPROLETEAM::iterator it;
	it=_mapTeam.find(teamID);

	if(_mapTeam.end() != it)
	{
		if((it->second).GetLeaderRoleID()==RoleID)
		{
			return 1;
		}
	 	(it->second).ChangToBeLeader(RoleID);//改变队长

	 	TeamRole1=(it->second).GetMemberRoleID();
	 	for(itor=TeamRole1.begin();itor!=TeamRole1.end();itor++)
		{
	 	   RolePtr pRole = ProcessGetRolePtr(itor->roleId);
	 	   pRole->LeaderRoleID(RoleID);//设置成员的Role中的队长
		}
	 	ProcessGetRolePtr(RoleID)->TeamFlag(2);
	}
	else
	{
	//	return team;
		return -1;
	}
	return 0;
}
int CoreData::LogoutToTeam(UInt32 teamID,UInt32 roleID)
{
	MAPROLETEAM::iterator it;
	it=_mapTeam.find(teamID);
	UInt32 ID;
	list<TeamRole> TeamRole1;
	list<TeamRole>::iterator itor;
	List<UInt32> ito;

	if(_mapTeam.end() != it)
	{
	 	(it->second).OutTheTeam(roleID);
	 	EraseRoleIDTeamID(roleID);
//	 	RoleTeamset(roleID,0,0,0);
	 	LOG(LOG_ERROR,__FILE__,__LINE__,"LogoutToTeam ---  RoleID[%d] %d", roleID,(it->second).GetMemberRoleID().size());
	 	if((it->second).GetMemberRoleID().size()==0)
	 	{
	 		DeleteTeam(teamID);

	 	}
	 	else if(roleID==(it->second).GetLeaderRoleID())
	 	{ //离开的是队长

			TeamRole1 = (it->second).GetMemberRoleID();
			itor = TeamRole1.begin();
			ID = itor->roleId;
			ProcessGetRolePtr(ID)->TeamFlag(2);
			(it->second).ChangToBeLeader(ID);
			for (itor = TeamRole1.begin(); itor != TeamRole1.end(); itor++) {
				RolePtr pRole = ProcessGetRolePtr(itor->roleId);
				pRole->LeaderRoleID(ID); //设置成员的Role中的队长
				ito.push_back(itor->roleId);
				LOG(LOG_ERROR, __FILE__, __LINE__,
						"LogoutToTeam --- RoleID[%d]", itor->roleId);

			}
				_mainSvc->GetTeamSvc()->NotifyTeamChang(roleID,5,ito);//不用给自己发
	 			_mainSvc->GetTeamSvc()->NotifyTeamChang(ID,2,ito);//不用给自己发

	 	}
	 	else
	 	{
	 			TeamRole1=(it->second).GetMemberRoleID();
	 			for(itor=TeamRole1.begin();itor!=TeamRole1.end();itor++)
				{
					//RoleTeamLeaderID(itor->roleId,ID);//设置成员的Role中的队长

						ito.push_back(itor->roleId);
						LOG(LOG_ERROR,__FILE__,__LINE__,"LogoutToTeam ---!! RoleID[%d]", itor->roleId);

				}
	 			_mainSvc->GetTeamSvc()->NotifyTeamChang(roleID,5,ito);//不用给自己发
	 	}
	 	//S-C
	 	//发送离开的S-C

	 //	_mainSvc->GetTeamSvc()->NotifyTeamChang(ID,2,ito);//不用给自己发
	}
	else
	{
	//	return team;
		return -1;
	}
	return 0;
}

void CoreData::AddPKBrif(UInt32 pkID,PKBriefList& pkinfo)
{
	MutexLockGuard lock(mutex_);
	map<UInt32,PKBriefList>::iterator it;
	it=_mapPkInfo.find(pkID);
	if(it!=_mapPkInfo.end())
	{
		_mapPkInfo.erase(it);
	}
	_mapPkInfo.insert(make_pair(pkID,pkinfo));
	AddRolePKID(pkID,pkinfo);
}
//@return -1 没有找到
int CoreData::DeletePkBrif(UInt32 pkID)
{
	MutexLockGuard lock(mutex_);
	map<UInt32,PKBriefList>::iterator it;
	it=_mapPkInfo.find(pkID);
	if(it!=_mapPkInfo.end())
	{

		deleterolePKID(it->second);
			_mapPkInfo.erase(it);
		return 0;
	}
	else
	{
		return -1;
	}
}
void CoreData::AddRolePKID(UInt32 pkID,PKBriefList& pkinfo)
{
	map<UInt32,UInt32>::iterator it;
	list<UInt32>::iterator it1;
	for(it1=(pkinfo.player1).begin();it1!=(pkinfo.player1).end();it1++)
	{
		it=_mapRolePKID.find(*it1);
		if(it!=_mapRolePKID.end())
		{
			_mapRolePKID.erase(it);
		}
		_mapRolePKID.insert(make_pair(*it1,pkID));
	}

	for(it1=(pkinfo.player2).begin();it1!=(pkinfo.player2).end();it1++)
	{
		it=_mapRolePKID.find(*it1);
		if(it!=_mapRolePKID.end())
		{
			_mapRolePKID.erase(it);
		}
		_mapRolePKID.insert(make_pair(*it1,pkID));
	}
}
void CoreData::deleterolePKID(PKBriefList pkinfo)
{
	map<UInt32,UInt32>::iterator it;
	list<UInt32>::iterator it1;
	for(it1=(pkinfo.player1).begin();it1!=(pkinfo.player1).end();it1++)
	{
		it=_mapRolePKID.find(*it1);
		if(it!=_mapRolePKID.end())
		{
			_mapRolePKID.erase(it);
		}
	}

	for(it1=(pkinfo.player2).begin();it1!=(pkinfo.player2).end();it1++)
	{
		it=_mapRolePKID.find(*it1);
		if(it!=_mapRolePKID.end())
		{
			_mapRolePKID.erase(it);
		}
	}
}

//@return 值全为0的对象
PKBriefList CoreData::GetPkInfo(UInt32 pkID)
{
	MutexLockGuard lock(mutex_);
	PKBriefList pklist;
	map<UInt32,PKBriefList>::iterator it;
	it=_mapPkInfo.find(pkID);
	if(it!=_mapPkInfo.end())
	{
		pklist=it->second;
	}
		return pklist;
}

//@return 1为找到,0为没有找到
UInt32 CoreData::IfRoleInPK(UInt32 roleID)//查看角色是否在PK
{
	MutexLockGuard lock(mutex_);

	map<UInt32,UInt32>::iterator it;
	it=_mapRolePKID.find(roleID);
	if(it!=_mapRolePKID.end())
	{
		return it->second;
	}
		return 0;
}


void CoreData::EraseRolePKID(UInt32 roleID)
{
	MutexLockGuard lock(mutex_);

	map<UInt32,UInt32>::iterator it;
	it=_mapRolePKID.find(roleID);
	if(it!=_mapRolePKID.end())
	{
		_mapRolePKID.erase(it);
	}

	return ;
}
