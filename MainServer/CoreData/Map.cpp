#include "Map.h"
#include "MainSvc.h"
#include "Log.h"
#include <stdlib.h>
#include "DBOperate.h"
#include "Role.h"
#include "Monster.h"
#include "Npc.h"
#include "DebugData.h"
#include <math.h>
#include "ArchvMap.h"
#include <fstream>
#include <iostream>
#include "../Npc/NpcSvc.h"
#include "Account.h"
#include "CoreData.h"
#include "ArchvMap.h"
#include "ArchvBagItemCell.h"
#include "../Mail/ArchvMail.h"
#include "../Mail/MailSvc.h"

#include "TimerManager.h"
#include "NewTimer.h"

Map::Map()
:mutex_()
,_mapID(0)
,_mainSvc(NULL)
,_intervalCalcRolePos(30)
,_intervalCache2DB(30)
,_intervalCheckMonsterStatus(30)
,_intervalMonsterRevive(30)
,_intervalHpMpadd(10)
,_isStop(true)
{}

Map::~Map()
{

	//设置停止标志，并等待子线程退出
	StopAndWait();

	//角色缓存入库
	ProcessAllRoleCache2DB();

	//资源释放
	MutexLockGuard lock(mutex_);
	{
		//释放 角色资源
		MAPROLE::iterator it1;
//		for( it1 = _mapRole.begin(); it1 != _mapRole.end(); it1++)
//			delete it1->second;
		_mapRole.clear();

		//释放 怪资源
		MAPMONSTER::iterator it2;
		for( it2 = _mapMonster.begin(); it2 != _mapMonster.end(); it2++)
			delete it2->second;
		_mapMonster.clear();

		//释放 Npc资源
		MAPNPC::iterator it3;
		for( it3 = _mapNpc.begin(); it3 != _mapNpc.end(); it3++)
			delete it3->second;
		_mapNpc.clear();

	}

}

//设置停止标志，并等待子线程退出
void Map::StopAndWait()
{
	_isStop = true;

	//等待子线程退出
//	pthread_join(_thrTimerCalcRolePos,NULL);
//	pthread_join(_thrTimerCache2DB,NULL);


}

//@初始化
//@param	service				主业务对象
//@param	mapID					地图ID
//@return 0 成功  非0 失败
int Map::Init( void * service, UInt32 mapID, ConnectionPool * cp)
{
	int iRet = 0;

	//基本属性设置
	_mainSvc = static_cast<MainSvc*>(service);
	_mapID = mapID;
	_cp = cp;

	//获取参数
	IniFile iniFile;
	string strJConfigScene;					//json配置文件目录--地图
	string strJConfigMonster;				//json配置文件目录--怪物
	string strIntervalCalcRolePos;	//计算角色当前位置的时间间隔
	string strIntervalDBCheckIn;		//	缓存入DB的时间间隔
	string strIntervalCheckMonsterStatus;//	检查怪状态时间间隔
	string strIntervalMonsterRevive;//小怪复活时间间隔

	if(!iniFile.open( _mainSvc->Service()->GetConfFilePath()))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"iniFile.open() error!" );
		return -1;
	}

	if( iniFile.read("Map","jConfigScene", strJConfigScene) ) return -1;
	if( iniFile.read("Map","jConfigMonster", strJConfigMonster) ) return -1;
	if( iniFile.read("Map","intervalCalcRolePos", strIntervalCalcRolePos) ) return -1;
	if( iniFile.read("Map","intervalDBCheckIn", strIntervalDBCheckIn) ) return -1;
	if( iniFile.read("Map","intervalCheckMonsterStatus", strIntervalCheckMonsterStatus) ) return -1;
	if( iniFile.read("Map","intervalMonsterRevive", strIntervalMonsterRevive) ) return -1;

	_intervalCalcRolePos = atoi(strIntervalCalcRolePos.c_str());
  _intervalCache2DB = atoi(strIntervalDBCheckIn.c_str());
  _intervalCheckMonsterStatus = atoi(strIntervalCheckMonsterStatus.c_str());
  _intervalMonsterRevive = atoi(strIntervalMonsterRevive.c_str());

	//参数校验
	if( _intervalCalcRolePos <= 0 ||
			_intervalCache2DB	<= 0 ||
			_intervalCheckMonsterStatus <= 0 ||
			_intervalMonsterRevive <= 0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"error param!! _intervalCalcRolePos[%d], _intervalCache2DB[%d], _intervalCheckMonsterStatus[%d],_intervalMonsterRevive[%d]", _intervalCalcRolePos, _intervalCache2DB, _intervalCheckMonsterStatus,_intervalMonsterRevive );
		return -1;
	}

	//定时器相关
	_isStop = false;	
	TimerManager& timerMgr = _mainSvc->_tm;

	//todo:针对角色的定时器应该放到角色的列表里，当角色退出的时候删除定时器

	//定时器: 计算角色路径
	NewTimer *timer = _mainSvc->_timerPool.newTimer();
	if (timer != 0)
	{
		timer->Type(NewTimer::LoopTimer);
		iRet = timer->SetCallbackFun( TimerCBCalcAllRolePos, this, NULL, 0);
		if(iRet)
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "SetCallbackFun  error.");
	 		return -1;
		}
		timer->Interval(_intervalCalcRolePos);
		_tm.AddTimer(timer);	
	}
	
	//定时器: 缓存入库
	timer = _mainSvc->_timerPool.newTimer();
	if (timer != 0)
	{
		timer->Type(NewTimer::LoopTimer);
		iRet = timer->SetCallbackFun( TimerCBCache2DB, this, NULL, 0);
		if(iRet)
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "SetCallbackFun  error.");
	 		return -1;
		}
		timer->Interval(_intervalCache2DB);
		_tm.AddTimer(timer);
	}
	
	//定时器: 小怪刷新
	timer = _mainSvc->_timerPool.newTimer();
	if (timer != 0)
	{
		
		timer->Type(NewTimer::LoopTimer);
		iRet = timer->SetCallbackFun( TimerCBCheckMonsterStatus, this, NULL, 0);
		if(iRet)
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "SetCallbackFun  error.");
			return -1;
		}
		timer->Interval(_intervalCheckMonsterStatus);
		_tm.AddTimer(timer);
	}
	
	//定时器: 场景自动回血
	timer = _mainSvc->_timerPool.newTimer();
	if (timer != 0)
	{
		timer->Type(NewTimer::LoopTimer);
		iRet = timer->SetCallbackFun( TimerCBHpMpadd, this, NULL, 0);
		if(iRet)
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "SetCallbackFun  error.");
	 		return -1;
		}
		timer->Interval(_intervalHpMpadd);
		_tm.AddTimer(timer);
	}
	

	//npc 初始化
	iRet = InitNpc(strJConfigScene);
	if(iRet)
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "InitNpc error. mapID[%d]", _mapID);
 		return -1;
	}

	//怪初始化
	iRet = InitMonster( strJConfigScene, strJConfigMonster );
	if(iRet)
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "InitMonster error. mapID[%d]", _mapID);
 		return -1;
	}

	return 0;
}

//@brief	读取json 参数封装
//	参数异常则写日志
//@return	json::Value
Json::Value Map::GetJsonValue( const Json::Value& jValue, const string& key )
{
	Json::Value vRet;
	if(jValue.isMember(key))
		vRet = jValue[key];

	return vRet;
}

//@brief 	字符串去掉前补的 0
void Map::LeftTrim( string& output)
{
	string strTmp(output);
	int i = 0;
	for( i = 0; i < strTmp.size(); i++ )
	{
		if( "0" != strTmp.substr( i, 1 ) )
			break;
	}

	output = strTmp.substr(i);
}



//@brief	初始化场景怪
//@return	0 成功  非0 失败
int Map::InitNpc(const string& jConfigScene )
{
	//读取 json配置参数
	Json::Reader reader;
	Json::Value rootScene;

	//读取 地图配置文件
	ifstream fileScene(jConfigScene.c_str());
	if (!reader.parse(fileScene, rootScene, false))
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "reader.parse error., jConfigScene[%s]", jConfigScene.c_str() );
		return -1;
  }

	//获取地图ID
	char szMapID[64];
	sprintf( szMapID, "%d", _mapID );
	Json::Value jMap = GetJsonValue( rootScene, szMapID );
	if(jMap.empty())
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "jMap is empty, mapID[%d]", _mapID);
		return 0;
	}

	//获取 NPC
  Json::Value jNpcBrief = GetJsonValue( jMap, "npc" );
	if(jNpcBrief.empty())
	{
		LOG(LOG_DEBUG, __FILE__, __LINE__, "jNpcBrief is empty, mapID[%d]", _mapID);
		return 0;
	}

  string strCreatureID;
  string strCreatureType;
  Npc * npc = NULL;
  ArchvPosition pos;
  for( int i = 0; i < jNpcBrief.size(); i++ )
  {
  	Json::Value &curr = jNpcBrief[i];

		//地图参数获取
		strCreatureID = GetJsonValue( curr, "id" ).asString();
		strCreatureType = strCreatureID.substr(1,4);
		pos.X = atoi(GetJsonValue( curr, "posX" ).asString().c_str());
		pos.Y = atoi(GetJsonValue( curr, "posY" ).asString().c_str());

  	//Npc
  	npc = new Npc();
		npc->ID( atoi(strCreatureID.c_str()) );
		npc->Flag(3);
		npc->Type( atoi(strCreatureType.c_str()) );
		npc->Pos(pos);

		//打印测试
		if( 0 == i && 1 == _mapID  )
			npc->Print();

		//插入怪列表
		_mapNpc.insert( make_pair(npc->ID(),npc) );

  }

	if( _mapNpc.size() == 0 )
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "no npc in map!!,mapID[%d], _mapNpc.size()[%d]", _mapID, _mapNpc.size() );
		return -1;
	}

	return 0;
}



//@brief	初始化场景怪
//@return	0 成功  非0 失败
int Map::InitMonster(const string& jConfigScene, const string& jConfigMonster )
{
	//读取 json配置参数
	Json::Reader reader;
	Json::Value rootScene;
	Json::Value rootMonster;
	SenceMonster monsterp;
	SenceMonsterAll monsters;
	UInt32 monsterTypeNum;
	list<UInt32> listNum;
	UInt32 TaskID=0;
	ItemList itms;

	ItemList item;
	list<ItemList> its;
	list<UInt32>::iterator it;
	//读取 地图配置文件
	ifstream fileScene(jConfigScene.c_str());
	if (!reader.parse(fileScene, rootScene, false))
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "reader.parse error.");
		return -1;
  }

	//读取 怪配置文件
	ifstream fileMonster(jConfigMonster.c_str());
  if (!reader.parse(fileMonster, rootMonster, false))
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "reader.parse error.");
		return -1;
  }

	//获取地图ID
	char szMapID[64];
	sprintf( szMapID, "%d", _mapID );
	Json::Value jMap = GetJsonValue( rootScene, szMapID );
	if(jMap.empty())
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "jMap is empty, mapID[%d]", _mapID);
		return 0;
	}

	//读取 小怪 参数
  Json::Value jMonsterBrief = GetJsonValue( jMap, "monsters" );
 	if(jMonsterBrief.empty())
 	{
		LOG(LOG_DEBUG, __FILE__, __LINE__, "jMonsterBrief is empty, mapID[%d]", _mapID);
		return 0;
	}

	Json::Value jItemDrop=GetJsonValue( rootMonster, "PublicDropType" );
	char Ttype=0;
//	LOG(LOG_DEBUG,__FILE__,__LINE__,"jItemDrop.size[%d]",jItemDrop.size());
	for(int i=0;i< jItemDrop.size();i++)
	{		its.clear();

		Json::Value &cxrr = jItemDrop[i];//获取
		string a=GetJsonValue( cxrr, "DropType" ).asString();

		Ttype=a[0];
		Json::Value JItemlist;
		JItemlist=GetJsonValue( cxrr, "ItemList" );
		for(int j=0;j<JItemlist.size();j++)
		{
			item.ItemID=atoi(GetJsonValue( JItemlist[j], "ItemID" ).asString().c_str());
			item.num=atoi(GetJsonValue( JItemlist[j], "rate" ).asString().c_str());
			its.push_back(item);
		}
		_mapItemdrop.insert( make_pair(Ttype,its) );

	}



	string strCreatureID;
	string strCreatureType;
	string strPCreatureType;
	string strID;
	ArchvPosition pos;
	for(int i=0;i< jMonsterBrief.size(); i++)
	{
		Json::Value &curr = jMonsterBrief[i];//获取
		strCreatureID = GetJsonValue( curr, "id" ).asString();//获取ID
		monsters.Monsterp.clear();
		strCreatureType = strCreatureID.substr(1,4);
		LeftTrim(strCreatureType);			//去除前补的 0
		pos.X = atoi(GetJsonValue( curr, "posX" ).asString().c_str());
		pos.Y = atoi(GetJsonValue( curr, "posY" ).asString().c_str());
		monsters.X=pos.X;
		monsters.Y=pos.Y;
		monsters.MonsterID=atoi(strCreatureID.c_str());
		monsters.MonsterType=atoi(strCreatureType.c_str());
		monsters.Status=1;//1活的,2战斗中，4死的
		if(GetallTheNum(monsters.MonsterType,listNum)!=1)
		{
			listNum.push_back(monsters.MonsterType);
		}


		Json::Value partners=GetJsonValue( curr, "monsterpartner" );
		for(int j=0;j< partners.size();j++)
		{
			Json::Value &corr =partners[j];
			strPCreatureType=GetJsonValue( corr, "monsterType" ).asString();
			monsterp.MonsterType=atoi(strPCreatureType.c_str());
			monsterp.num=atoi(GetJsonValue( corr, "monsternum" ).asString().c_str());

				if(GetallTheNum(monsterp.MonsterType,listNum)!=1)
				{
					listNum.push_back(monsterp.MonsterType);
				}
			monsters.Monsterp.push_back(monsterp);

		}


		//把要用的怪物存放在这个地图上




		_mapsenceMonster.insert( make_pair(monsters.MonsterID,monsters) );
		//阵营现在不管
	}
  //读取公共物品掉落




  Monster * mstr = NULL;

 for( it = listNum.begin(); it!=listNum.end(); it++ )
  {
		char szTmp[64];
  	sprintf( szTmp, "%d",*it);
  	strID=szTmp;

		LeftTrim(strID);
		//获取该类型怪的描述
  	Json::Value jMonsterDesc = GetJsonValue( rootMonster, strID);
		Json::Value jMonsterItem;
  	//怪物描述获取
  	mstr = new Monster();
		mstr->ID( atoi(strID.c_str()) );							//怪ID
		mstr->Flag(2);
		mstr->Type( atoi(strID.c_str()) );					//生物类型
		mstr->Camp(1);
		//mstr->Camp( atoi(GetJsonValue( jMonsterDesc, "defaultcomap" ).asString().c_str()) );
		mstr->MapID(_mapID);
		mstr->Status(1);						//生物状态
		mstr->MaxHp(atoi(GetJsonValue( jMonsterDesc, "maxHP" ).asString().c_str()) );
		mstr->MaxMp(atoi(GetJsonValue( jMonsterDesc, "maxMP" ).asString().c_str()) );
		mstr->Level(atoi(GetJsonValue( jMonsterDesc, "level" ).asString().c_str()) );
		mstr->Hp(mstr->MaxHp());
		mstr->Mp(mstr->MaxMp());
		mstr->MoveSpeed(atoi(GetJsonValue( jMonsterDesc, "moveSpeed" ).asString().c_str()) );
		mstr->AttackPowerHigh(atoi(GetJsonValue( jMonsterDesc, "attackPowerHigh" ).asString().c_str()) );
		mstr->AttackPowerLow(atoi(GetJsonValue( jMonsterDesc, "attackPowerLow" ).asString().c_str()) );
		mstr->AttackScope(atoi(GetJsonValue( jMonsterDesc, "attackArea" ).asString().c_str()) );
		mstr->AttackSpeed(atoi(GetJsonValue( jMonsterDesc, "attackSpeed" ).asString().c_str()) );
		mstr->BulletSpeed(atoi(GetJsonValue( jMonsterDesc, "attackBulletSpeed" ).asString().c_str()) );
		mstr->HitRate(atoi(GetJsonValue( jMonsterDesc, "hitRate" ).asString().c_str()) );
		mstr->DodgeRate(atoi(GetJsonValue( jMonsterDesc, "dodgeRate" ).asString().c_str()) );
		mstr->Defence(atoi(GetJsonValue( jMonsterDesc, "defense" ).asString().c_str()) );
		mstr->MDefence(atoi(GetJsonValue( jMonsterDesc, "mDefense" ).asString().c_str()) );
		//怪物的一些别的，暂时没有读配置文件
		mstr->CritRate(atoi(GetJsonValue( jMonsterDesc, "critRate" ).asString().c_str()));//暴击
		mstr->Money=atoi(GetJsonValue( jMonsterDesc, "money" ).asString().c_str());
		mstr->Exp=atoi(GetJsonValue( jMonsterDesc, "Exp" ).asString().c_str());
		jMonsterItem=GetJsonValue( jMonsterDesc, "ItemDropTask" );//任务掉落不管
		for(int pp=0;pp<jMonsterItem.size();pp++)
		{
			TaskID=atoi(GetJsonValue( jMonsterItem[pp], "TaskID" ).asString().c_str());
			itms.ItemID=atoi(GetJsonValue( jMonsterItem[pp], "ItemID" ).asString().c_str());
			itms.num=atoi(GetJsonValue( jMonsterItem[pp], "rate" ).asString().c_str());
			mstr->ItemDropTask.insert( make_pair(TaskID,itms) );
		}
		jMonsterItem=GetJsonValue(jMonsterDesc, "ItemDropSpec" );//物品掉落
		for(int pp=0;pp<jMonsterItem.size();pp++)
		{
			itms.ItemID=atoi(GetJsonValue( jMonsterItem[pp], "ItemID" ).asString().c_str());
			itms.num=atoi(GetJsonValue( jMonsterItem[pp], "rate" ).asString().c_str());
			mstr->ItemDropSpec.push_back(itms);
		}
		string publicchar;
		publicchar=GetJsonValue( jMonsterDesc, "ItemDropPublic" ).asString();//公共掉落
		mstr->ItemDropPublic=publicchar[0];


		mstr->Pos(pos);

		_mapMonster.insert( make_pair(mstr->ID(),mstr) );

  }
	if( _mapMonster.size() == 0 )
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "no monster in map!!,mapID[%d], _mapMonster.size()[%d]", _mapID, _mapMonster.size() );
		return -1;
	}


	return 0;
}


//定时器回调
//	定时计算所有角色当前位置,并整理移动路径
void Map::TimerCBCalcAllRolePos( void * obj, void * arg, int argLen )
{
	Map * service = static_cast<Map *>(obj);
	service->ProcessCalcAllRolePos();
}

//定时器回调
//	缓存入库
void Map::TimerCBCache2DB( void * obj, void * arg, int argLen )
{
	Map * service = static_cast<Map *>(obj);
	service->ProcessAllRoleCache2DB();

//	if( 1 == service->MapID())
//		DEBUG_PRINTF2("TimerCBCache2DB, threadID[%d], time[%d]", pthread_self(), time(NULL));

}


//定时器回调
//	定时小怪刷新
void Map::TimerCBCheckMonsterStatus( void * obj, void * arg, int argLen )
{
	Map * service = static_cast<Map *>(obj);
	service->ProcessCheckAllMonsterStatus();
}

//定时器回调
//	自动回血
void Map::TimerCBHpMpadd( void * obj, void * arg, int argLen )
{
	Map * service = static_cast<Map *>(obj);
	service->ProcessMpHpTimeadd();
}

//定时器回调
//	延后发送 S_C 204
void Map::TimerCBS2C204( void * obj, void * arg, int argLen )
{
 	UInt32 roleID = *((UInt32*)arg);
 	Map * service = static_cast<Map *>(obj);
	service->NotifyCtStatusUnicast( roleID, 0x07 );

	DEBUG_PRINTF1( "TimerCBS2C204-------roleID[%d]--> ", roleID );
}


//精确计算两坐标间的直线距离
//	使用勾股定理
//@param	pos1		坐标1
//@param	pos2		坐标2
//@param	direct	返回的方向
//@return	>=0		两坐标的直线距离   <0		失败
double Map::CalcPosDistanceExact( const ArchvPosition &pos1, const ArchvPosition &pos2 )
{
	return sqrt( pow( abs(pos1.X-pos2.X), 2 ) + pow( abs(pos1.Y-pos2.Y), 2 ) );
}

//@brief 组建角色的生物状态广播包
//@return 空
void Map::MakeSCPkgForRole( RolePtr& role, Byte status, List<ArchvCreatureStatus>& lcs )
{
	ArchvCreatureStatus cs;

	cs.creatureFlag = 1;
	cs.creatureType = role->ProID();
	cs.ID = role->ID();
	cs.status = status;

	lcs.push_back(cs);
}

//@brief 组建怪的生物状态广播包
//@return 空
void Map::MakeSCPkgForMonster( SenceMonsterAll& mst, Byte status, List<ArchvCreatureStatus>& lcs )
{
	ArchvCreatureStatus cs;

	cs.creatureFlag = 2;
	cs.creatureType = mst.MonsterType;
	cs.ID = mst.MonsterID;
	cs.status = status;

	lcs.push_back(cs);
}

//@brief	设置对象属性--状态
//@return 0成功 1 对象不存在   其他 失败
int Map::SetRoleStatus( UInt32 roleID, Byte status )
{
	MutexLockGuard lock(mutex_);

	MAPROLE::iterator it;
	it = _mapRole.find(roleID);
	if( _mapRole.end() == it )
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "SetRoleStatus() error! role not found, roleID[%d]", roleID );
		return 1;
	}

	//修改状态
	it->second->Status(status);

	//发送状态广播
	List<ArchvCreatureStatus> lcs;
	MakeSCPkgForRole( it->second, status, lcs );
	NotifyCtStatusBroadcast(lcs);

	return 0;
}

//@brief	设置怪属性--状态
//@return 0成功 1 对象不存在   其他 失败
int Map::SetMonsterStatus( UInt32 monsterID, Byte status )
{
	MutexLockGuard lock(mutex_);

	MapMonsters::iterator it;
	it = _mapsenceMonster.find(monsterID);
	if( _mapsenceMonster.end() == it )
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "SetMonsterStatus() error! monster not found, monsterID[%d]", monsterID );
		return 1;
	}
	//修改状态
	if(status==0)
	{
		it->second.dieTime=time(NULL);
	}
	it->second.Status=status;
	//死亡设置死亡时间
	//发送状态广播
	List<ArchvCreatureStatus> lcs;
	MakeSCPkgForMonster( it->second, status, lcs );
	NotifyCtStatusBroadcast(lcs);

	return 0;
}


//@brief	角色缓存入库
//@return 空
void Map::ProcessAllRoleCache2DB()
{
	MutexLockGuard lock(mutex_);

	MAPROLE::iterator it;
//	LOG (LOG_ERROR, __FILE__, __LINE__, "hello time %d",time(NULL));
	for( it = _mapRole.begin(); it != _mapRole.end(); it++ )
	{
		it->second->Cache2DB();
	}
	//LOG (LOG_ERROR, __FILE__, __LINE__, "hello time %d",time(NULL));
}
void Map::ProcessMpHpTimeadd() {
	MutexLockGuard lock(mutex_);
	MAPROLE::iterator it;
	//	LOG (LOG_ERROR, __FILE__, __LINE__, "hello time %d",time(NULL));

	for (it = _mapRole.begin(); it != _mapRole.end(); it++) {
		if (it->second->Hp() < it->second->MaxHp() || it->second->Mp()
				< it->second->MaxMp()) {
			it->second->HpMpadd(_intervalHpMpadd);
			NotifyHPandMP(it->second->ID(), it->second->Hp(), it->second->Mp());
			//		LOG (LOG_DEBUG, __FILE__, __LINE__, "1111111 RoleID[%d] HP[%d] MP[%d]",it->second->ID(), it->second->Hp(), it->second->Mp());
		}
	}
}

UInt32 Map::MapID()
{
	return _mapID;
}

//@brief	小怪定时复活
//@return 空
void Map::ProcessCheckAllMonsterStatus()
{
	MutexLockGuard lock(mutex_);

	map<UInt32,SenceMonsterAll>::iterator it;
	ArchvCreatureStatus cs;
	List<ArchvCreatureStatus> lcs;
	for( it = _mapsenceMonster.begin(); it != _mapsenceMonster.end(); it++ )
	{
		//死掉的怪,超过复活时间间隔,则复活
		if( 0 == it->second.Status &&
				(time(NULL) - it->second.dieTime > _intervalMonsterRevive) )
		{
			it->second.Status=1;
			cs.creatureFlag = 2;
			cs.creatureType = it->second.MonsterType;
			cs.ID = it->second.MonsterID;
			cs.status = 4;				//怪刷新复活

			lcs.push_back(cs);
		}
	}

	//广播
	if(lcs.size()>0)
		NotifyCtStatusBroadcast(lcs);

}


//计算所有角色当前位置,并整理移动路径
void Map::ProcessCalcAllRolePos()
{
	MutexLockGuard lock(mutex_);

	MAPROLE::iterator it;
	for( it = _mapRole.begin(); it != _mapRole.end();it++ )
	{
		//计算角色当前位置
		it->second->CalcCurrPos();
	}

}

//@brief	单播 该地图所有其他角色的信息
void Map::ProcessNotifyAllRoleInfo(UInt32 roleID)
{
	MutexLockGuard lock(mutex_);

	//其他用户信息列表，发送给进入场景的角色
	NotifyAllRoleInfo( roleID );
}
list<ItemList> Map::GetPublicDrop(char type)//获取公共掉落的物品
{

	MutexLockGuard lock(mutex_);
	MapItemDorp::iterator it;
	list<ItemList> itemlist;

	it = _mapItemdrop.find(type);

	//角色已经在地图中
	if( _mapItemdrop.end() == it )
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "find the type to the monster erro type=%c", type );
		return itemlist;
	}

	itemlist=it->second;
	return itemlist;

}



//@brief	角色进入地图
//@param	role	角色属性
//@return 0 成功  非0 失败
int Map::ProcessRoleEnterMap( RolePtr &pRole )
{
	MutexLockGuard lock(mutex_);

	MAPROLE::iterator it;
	it = _mapRole.find(pRole->ID());

	//角色已经在地图中
	if( _mapRole.end() != it )
	{
		LOG(LOG_ERROR, __FILE__, __LINE__, "ProcessRoleEnterMap() error, Role exists!!!!! roleID[%d]", pRole->ID() );
		return -1;
	}

	//申请内存,插入记录
	pRole->SetConnectPool(_cp);			//场景中的角色,需要DB 操作
	pRole->MapID(_mapID);						//强制性修改角色地图,避免外部赋值有问题
	pRole->AddEnterMapNum();				//进入地图总次数的更新
	_mapRole.insert( make_pair( pRole->ID(), pRole));

	if (pRole->ISVIP() <= 6)
	{
		GiveVipGiftOnEnterMap(pRole);
	}
	//缓存写入DB
	pRole->Cache2DB();

	//发送进入场景广播
	ArchvRoute route(pRole->Route());
	NotifyRoleMove( pRole->ID(), 0, route );


	//定时器 延后N秒发送:
	//	场景所有生物（角色、怪、宠物）状态，发送给进入地图者
	UInt32 roleID = pRole->ID();
	NewTimer* timer = _mainSvc->_timerPool.newTimer();
	if (timer)
	{
		timer->Type(NewTimer::OnceTimer);
		int iRet = timer->SetCallbackFun( TimerCBS2C204, this, &roleID, sizeof(roleID));
		if(iRet)
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "SetCallbackFun  error.");
 			return -1;
		}
		timer->Interval(2);
		_tm.AddTimer(timer);
	}

DEBUG_PRINTF2("ProcessRoleEnterMap success!!!+++++roleID[%d] login, mapID[%d]", pRole->ID(), _mapID );
DEBUG_PRINTF3("roleID[%d],name[%s], proID[%d]", pRole->ID(), pRole->Name().c_str(), pRole->ProID());
DEBUG_PRINTF1("_mapRole->size()[%d]", _mapRole.size() );

	return 0;
}



//@brief	角色离开地图
//@return Role 对象值
RolePtr Map::ProcessRoleExitMap( UInt32 roleID )
{
	MutexLockGuard lock(mutex_);

	MAPROLE::iterator it;
	it = _mapRole.find(roleID);
	if( _mapRole.end() != it )
	{
		//返回对象的赋值
		RolePtr pRole = it->second;

		//更新累计在线时间
		it->second->UpdateTotalOnlineSec();
		//记录角色最后的坐标
		it->second->LastX( it->second->Pos().X);
		it->second->LastY( it->second->Pos().Y);

		//缓存写入DB
		it->second->Cache2DB();

		//释放角色资源
		_mapRole.erase(it);

		//发送离开场景广播
		ArchvRoute route;
		NotifyRoleMove( roleID, 2, route );

		return pRole;
	}
	else
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "RoleExitMap() error! role not found, roleID[%d]", roleID );
		return RolePtr(new Role());
	}

//DEBUG_PRINTF2("ProcessRoleExitMap success!!!-----roleID[%d] login, mapID[%d]", pRole->ID(), _mapID );
//DEBUG_PRINTF1("_mapRole.size()[%d]", _mapRole.size() );
}

//@brief	获取对象属性
//@return RolePtr 对象值
RolePtr Map::ProcessGetRolePtr(UInt32 roleID) {
	MutexLockGuard lock(mutex_);

	MAPROLE::iterator it = _mapRole.find(roleID);
	if (_mapRole.end() != it) {
		return it->second;
	} else {
		LOG(LOG_ERROR, __FILE__, __LINE__,
				"ProcessGetRole() error! role not found, roleID[%d]", roleID);
		return RolePtr(new Role());
	}
}





//@brief	获取怪属性
//@return Monster 对象值
Monster Map::ProcessGetMonster( UInt32 mID )
{
	MutexLockGuard lock(mutex_);
	Monster monster;

	MAPMONSTER::iterator it;
	it = _mapMonster.find(mID);
	if( _mapMonster.end() != it )
	{
		monster = *(it->second);
	}
	else
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "ProcessGetMonster() error! monster not found, mID[%d]", mID );
	}

	return monster;
}


//@brief	角色移动z
//@return Role 对象值
void Map::ProcessRoleMove( UInt32 roleID, ArchvRoute &art )
{
	MutexLockGuard lock(mutex_);

	MAPROLE::iterator it;
	it = _mapRole.find(roleID);
	if( _mapRole.end() == it )
	{
		LOG (LOG_ERROR, __FILE__, __LINE__, "ProcessRoleMove() error! role not found, roleID[%d]", roleID );
		return;
	}

	//用户移动路径，起始时间调整为服务器时间
	art.time = time(NULL);

	//设置移动路径
	it->second->Route(art);

	//设置当前方位
	it->second->Pos(art.listPos.front());

	//计算当前方位
	it->second->CalcCurrPos();

	//计算客户端、服务端当前位置的误差距离
	//	如有需要，给客户端发送校验包
	int iRet = 0;
	ArchvRoute currRoute = it->second->Route();
	ArchvPosition firstPos = art.listPos.front();
	ArchvPosition currPos = currRoute.listPos.front();
	double dDistance = CalcPosDistanceExact( firstPos , currPos );
	if( dDistance >= POSACCURACY )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"client currPos error!!!! roleID[%d], mapID[%d], dDistance[%f], moveSpeed[%d], client currPos.X[%d]Y[%d], server currPos.X[%d]Y[%d] ", roleID, _mapID, dDistance, ROLE_MOVE_SPEED, firstPos.X, firstPos.Y, currPos.X, currPos.Y );
		NotifyCorrectRoute( roleID, currRoute );
	}

	//发送角色移动广播
	NotifyRoleMove( roleID, 1, art );

	return;
}



void Map::TradeInfo(TradeItem tradeItem,UInt32 roleID)
{
  MutexLockGuard lock(mutex_);
	MAPROLE::iterator it;
	it = _mapRole.find(roleID);
	if( _mapRole.end() == it )
	{
	  LOG (LOG_ERROR, __FILE__, __LINE__, "TradeInfo error! role not found, roleID[%d]", roleID );
	  return ;
	}

	it->second->TradeInfo(tradeItem);
}

UInt32 Map::FromNameTOFindID(string Name)
{
	MutexLockGuard lock(mutex_);

	MAPROLE::iterator itor;
	UInt32 RoleID=0;
	for( itor = _mapRole.begin(); itor != _mapRole.end(); ++itor )
	{
		if( Name == itor->second->Name() )
		{
			RoleID=itor->first;
			break;
		}
	}
	return RoleID;
}


//@brief	获取coredata  在线的所有 roleID
void Map::GetRoleIDs( list<UInt32>& lrid )
{
	MutexLockGuard lock(mutex_);
	MAPROLE::iterator it;
	lrid.clear();
	for( it = _mapRole.begin(); it != _mapRole.end(); it++ )
	{
		lrid.push_back(it->first);
	}
}

//@brief	获取coredata  在线的所有 roleID
//				不包含参数 roleID 在内
void Map::GetRoleIDs( UInt32 roleID, list<UInt32>& lrid )
{
	MutexLockGuard lock(mutex_);
	MAPROLE::iterator it;
	lrid.clear();
	for( it = _mapRole.begin(); it != _mapRole.end(); it++ )
	{
		if( it->first != roleID )
			lrid.push_back(it->first);
	}
}
ArchvPosition Map::GetSenceMonster(UInt32 MonsterID,list<SenceMonster>& Monsters)
{
	MutexLockGuard lock(mutex_);
	map<UInt32,SenceMonsterAll>::iterator it;
	SenceMonster monsster;
	ArchvPosition pos;

	it=_mapsenceMonster.find(MonsterID);
	if(_mapsenceMonster.end()==it)
	{
		pos.X=0;
		pos.Y=0;
		LOG (LOG_ERROR, __FILE__, __LINE__, "Donot find the Monster ,MonsterID is %d",MonsterID);
		return pos;
	}
	Monsters.clear();
	Monsters=it->second.Monsterp;
	monsster.MonsterType=(MonsterID%100000000)/10000;
	monsster.num=1;
	Monsters.push_front(monsster);
	pos.X=it->second.X;
	pos.Y=it->second.Y;
	return pos;

}


//@brief	[MsgType:0201]（广播）移动广播（含进入、离开场景）
//	角色移动信息广播
//@param	roleID	角色ID
//@param	MoveType		移动类型：0进入场景 1移动 2离开场景
//@param	art	角色移动路径
//@return	无
void Map::NotifyRoleMove( UInt32 roleID, Byte moveType, ArchvRoute &art )
{
	list<UInt32> lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	UInt32 speed = ROLE_MOVE_SPEED;

	//组包头
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType= 201;
	p.UniqID= 0;

	p.PackHeader();


	//组包体
	s<<roleID<<moveType<<speed<<art;
	p.UpdatePacketLength();

	//需要广播的用户列表
	lrid.clear();
	MAPROLE::iterator itor;

	for( itor = _mapRole.begin(); itor != _mapRole.end(); ++itor )
	{
		//不发送给发起请求的用户
		if( roleID == itor->first )
			continue;

		lrid.push_back( itor->first );
	}


	if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error !!!");
	}

DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );

}

//@brief [MsgType:0202]（单播）进入场景，场景所有其他角色的列表
//@param	roleID	角色ID
//@return	无
void Map::NotifyAllRoleInfo( UInt32 roleID )
{
	list<UInt32>	lrid;
	List<ArchvRoleMoveDesc> larmd;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	int iCount = 0;
	int iTmp = 0;
	ArchvRoleMoveDesc	armd;
DEBUG_PRINTF1( "roleID----[%d]\n", roleID );
	//发送通知的目标用户
	lrid.push_back( roleID);

	//地图中的用户数据
	MAPROLE::iterator itor;
	UInt32 currRoleID = 0;
	UInt32	Time = 0;

	for( itor = _mapRole.begin(); itor != _mapRole.end(); itor++ )
	{
		currRoleID = itor->first;
		iCount++;

		//Time字段数据
		Time = time(NULL);

DEBUG_PRINTF1( "roleID[%d]===========>>>>>>>\n", roleID );

		//用户信息加入到应答包
		if( roleID != currRoleID )
		{
DEBUG_PRINTF2( "roleID[%d], art.listPos.size[%d]\n", roleID, itor->second->Route().listPos.size() );
			armd.roleID = currRoleID;
			armd.speed = ROLE_MOVE_SPEED;
 			armd.route = itor->second->Route();

			larmd.push_back( armd );
		}

		if( iTmp < RNUM_PER_SEND &&
			iCount < (int)_mapRole.size() )
		{
			iTmp++;
			continue;
		}
		else
		{
			iTmp = 0;
		}

		//包缓存清理数据
		serbuffer.Reset();

		//组包头
		p.Direction = DIRECT_S_C_REQ;
		p.SvrType = 1;
		p.SvrSeq =1 ;
		p.MsgType= 202;
		p.UniqID= 0;

		p.PackHeader();

		//组包体
		s<<Time<<larmd;
		p.UpdatePacketLength();

		//发送数据
		if(_mainSvc->Service()->Broadcast( lrid, &serbuffer))
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!!" );
		}

DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
		DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );


		//清空用户状态列表
		lrid.clear();
	}


}

//@brief [MsgType:206] 角色升级通知
//@param roleID 升级的角色
void Map::NotifyRoleAddLevl(UInt32 roleID)
{
    List<UInt32> lrid;
	List<ArchvRoleMoveDesc> larmd;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);

	//地图中的用户列表
	MAPROLE::iterator itor;

	for(itor = _mapRole.begin();itor != _mapRole.end(); itor++)
	{
	    lrid.push_back(itor->first);
	}

	//包缓存清理数据
	serbuffer.Reset();

	//组包头
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq =1 ;
	p.MsgType= 206;
	p.UniqID= 0;

	p.PackHeader();

	//组包体
	s<<roleID;
	p.UpdatePacketLength();

	//发送数据
	if(_mainSvc->Service()->Broadcast( lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!!" );
	}

	//清空地图中的用户列表
	lrid.clear();

}





//@brief [MsgType:0203]（单播）移动路径修正通知
//	仅仅发送给单个玩家角色
//@param	roleID	角色ID
//@return	无
void Map::NotifyCorrectRoute( UInt32 roleID, ArchvRoute &rt )
{
	list<UInt32> lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();


	//组包头
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType= 203;
	p.UniqID= 0;

	p.PackHeader();

	//下发角色ID
	lrid.push_back( roleID );

	//组包体
	s<<rt;
	p.UpdatePacketLength();

	if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error !!!");
	}

DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}




//@brief  [MsgType:0204]（单播）生物（角色、怪、宠物）状态；Npc头顶状态
//	仅仅发送给玩家角色
//@param	roleID	角色ID
//@param	level		角色级别
//@param	flag		发送标志
//								flag& 0x01		发送角色状态
//								flag& 0x02		发送怪状态
//								flag& 0x04		发送npc头顶状态
//@return	无
void Map::NotifyCtStatusUnicast( UInt32 roleID, Byte flag)
{
	MutexLockGuard lock(mutex_);
	list<UInt32> lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();

	List<ArchvCreatureStatus> lbs;

	//组包头
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType= 204;
	p.UniqID= 0;

	p.PackHeader();

	//是否需要广播
	lrid.clear();
	lrid.push_back( roleID );

	//取角色、怪、状态
	MAPROLE::iterator it1;
	MapMonsters::iterator it2;
	MAPNPC::iterator it3;
	ArchvCreatureStatus bs;

	//获取角色状态
	if( flag&0x01 )
	{
		for( it1 = _mapRole.begin(); it1 != _mapRole.end(); it1++ )
		{
			bs.creatureFlag = 1;
			bs.creatureType = it1->second->ProID();
			bs.ID = it1->first;
			bs.status = it1->second->Status();
			lbs.push_back(bs);
		}
	}

	//获取怪状态
	if( flag&0x02 )
	{
		for( it2 = _mapsenceMonster.begin(); it2 != _mapsenceMonster.end(); it2++ )
		{
			bs.creatureFlag = 2;
			bs.creatureType = it2->second.MonsterType;
			bs.ID = it2->second.MonsterID;
			bs.status = it2->second.Status;
			lbs.push_back(bs);
		}
	}

	//获取 Npc 状态
	if( flag&0x04 )
	{
		Byte status = 0;
		MAPROLE::iterator itTmp;
		itTmp = _mapRole.find(roleID);
		if( _mapRole.end() == itTmp )
		{
			LOG (LOG_ERROR, __FILE__, __LINE__, "_mapRole.find error! role not found, roleID[%d]", roleID );
			return;
		}

		for( it3 = _mapNpc.begin(); it3 != _mapNpc.end(); it3++ )
		{
			bs.creatureFlag = 3;
			bs.creatureType = it3->second->Type();
			bs.ID = it3->first;

			_mainSvc->GetNpcSvc()->GetNpcStatus( roleID, itTmp->second->Level(), it3->second->ID(), status);
			if( status <= 3 )
			{
				bs.status = status + 10;
				lbs.push_back(bs);
			}
		}
	}


	//组包体
	s<<lbs;
	p.UpdatePacketLength();

	if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error !!!");
	}

DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}




//@brief [MsgType:0204]（广播）生物（角色、怪、宠物）状态；Npc头顶状态
//	不发送给状态改变者本身
//@param	lbs				生物状态描述列表
//@return	无
void Map::NotifyCtStatusBroadcast( List<ArchvCreatureStatus> &lcs )
{
	list<UInt32> lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();

	//组包头
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType= 204;
	p.UniqID= 0;

	p.PackHeader();

	//组包体
	s<<lcs;
	p.UpdatePacketLength();

	//是否需要广播
	lrid.clear();
	MAPROLE::iterator it;
	List<ArchvCreatureStatus>::iterator itCs;

	for( it = _mapRole.begin(); it != _mapRole.end(); ++it )
	{
		//不发送给状态改变者本身
		for( itCs = lcs.begin(); itCs != lcs.end(); itCs++ )
			if( 1 == itCs->creatureFlag && it->first == itCs->ID )
				continue;

		lrid.push_back( it->first );
	}

	if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error !!!");
	}

DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}

//S-C的血量，和蓝量
void Map::NotifyHPandMP(UInt32 roleID, UInt32 hp, UInt32 mp)
{
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 305;
	p.UniqID = 251;
	p.PackHeader();
	List<UInt32> lrid;
	s<<hp<<mp;

//	LOG(LOG_ERROR,__FILE__,__LINE__,"%d,%d !!",hp,mp);
	p.UpdatePacketLength();
	lrid.push_back(roleID);
	if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}


int Map::GetallTheNum(UInt32 ID,list<UInt32> lis)
{
	int flag=0;
	list<UInt32>::iterator it;
	for(it=lis.begin();it!=lis.end();it++)
	{
		if(ID==*it)
		{
			flag=1;
			break;
		}

	}
	return flag;
}

void Map::GiveVipGiftOnEnterMap(RolePtr& role)
{
	int vipLevel = role->ISVIP();
	if (vipLevel >= 0 && vipLevel <= 6)
	{
		time_t t = time(0);
		struct tm * timeinfo = localtime (&t);
		timeinfo->tm_hour = 0;
		timeinfo->tm_min = 0;
		timeinfo->tm_sec = 0;
		time_t today = mktime(timeinfo);

		Connection con;
		DBOperate dbo;
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());

		char szSql[1024];
		sprintf(szSql, "select DisseminationTime from VipGiftDissemination where RoleID = %d;",
				role->ID());
		int iRet = dbo.QuerySQL(szSql);
		bool bDisseminationOnce = false;

		if(iRet<0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			return;
		}
		int DisseminationTime = 0;
		while (dbo.HasRowData())
		{
			DisseminationTime = dbo.GetIntField(0);
			bDisseminationOnce = true;
			dbo.NextRow();
		}
		//新的一天，发放礼物
		if (today != (time_t)-1 && DisseminationTime < today)
		{
			sprintf(szSql, "select VipLevel, ItemID, ItemNum from VIPItemGiftDesc where VipLevel = %d;",
				vipLevel);

			iRet = dbo.QuerySQL(szSql);
			if(iRet==1)
			{//
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL Not find[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				return;
			}
			if(iRet<0)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				return;
			}

			int itemID = 0;
			int itemNum = 0;
			bool bDisseminate = false;
			while (dbo.HasRowData())
			{
				itemID = dbo.GetIntField(1);
				itemNum = dbo.GetIntField(2);
				ArchvSystemMailItem mail;
				mail.RecvRoleName = role->Name();
				mail.Content = "System Mail";
				mail.Money = 0;
				mail.MoneyType = 0;
				mail.ItemID = itemID;
				mail.Num = itemNum;
				mail.EntityID = 0;
				_mainSvc->GetMailSvc()->OnSendSystemMail(mail);
				bDisseminate = true;
				dbo.NextRow();
			}

			if (bDisseminate)
			{
				if (bDisseminationOnce)
				{
					sprintf(szSql, "update VipGiftDissemination set DisseminationTime = %d where RoleID = %d;",
						time(NULL),role->ID());
				}
				else
				{
					sprintf(szSql, "insert into VipGiftDissemination (RoleID,DisseminationTime) values(%d, %d);",
						role->ID(), time(NULL));
				}
				iRet = dbo.ExceSQL(szSql);
				if(iRet != 0)
				{
					LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error szSql[%s] ",szSql);
				}
			}
		}
	}
}


