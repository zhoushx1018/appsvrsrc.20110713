// 角色类
#ifndef ROLE_H
#define ROLE_H

#include "OurDef.h"
#include "Creature.h"
#include "./Trade/ArchvTrade.h"
#include "ArchvRole.h"
#include <map>
#include <tr1/memory>

using namespace std;

//角色升级可加属性点
#define AddAddPoint_Role 1 
#define CONNECTION_FD(x)	(int)(x&0xFFFF)
#define CONNECTION_IP(x)	(x >> 32)
#define CONNECTION_PORT(x)	(int)((x >> 16)&0xFFFF)
//每级数成长属性
#define Strength_ADD(p) (p==1)?5:((p==2)?4:((p==3)?3:((p==4)?3:((p==5)?2:3))))
#define Intelligence_ADD(p)	(p==1)?3:((p==2)?3:((p==3)?5:((p==4)?3:((p==5)?3:4))))
#define Agility_ADD(p) (p==1)?2:((p==2)?3:((p==3)?2:((p==4)?4:((p==5)?5:3))))
//天赋造成的
#define ADD_Skill1(p) (p==1)?5:((p==2)?5:((p==3)?5:((p==4)?5:((p==5)?5:5))))
#define ADD_Skill2(p) (p==1)?10:((p==2)?10:((p==3)?10:((p==4)?10:((p==5)?10:10))))
#define ADD_Skill3(p) (p==1)?15:((p==2)?15:((p==3)?15:((p==4)?15:((p==5)?15:15))))
#define ADD_Skill4(p) (p==1)?10:((p==2)?10:((p==3)?10:((p==4)?10:((p==5)?10:10))))
#define ADD_Skill5(p) (p==1)?5:((p==2)?5:((p==3)?5:((p==4)?5:((p==5)?5:5))))
#define ADD_Skill6(p) (p==1)?50:((p==2)?50:((p==3)?50:((p==4)?50:((p==5)?50:50))))
#define ADD_Skill7(p) (p==1)?10:((p==2)?10:((p==3)?10:((p==4)?10:((p==5)?10:10))))
#define ADD_Skill8(p) (p==1)?10:((p==2)?10:((p==3)?10:((p==4)?10:((p==5)?10:10))))
#define ADD_Skill9(p) (p==1)?5:((p==2)?5:((p==3)?5:((p==4)?5:((p==5)?5:5))))
#define ADD_Skill10(p) (p==1)?2:((p==2)?1:((p==3)?1:((p==4)?1:((p==5)?1:1))))
#define ADD_Skill11(p) (p==1)?15:((p==2)?15:((p==3)?15:((p==4)?15:((p==5)?15:15))))
#define ADD_Skill12(p) (p==1)?20:((p==2)?20:((p==3)?20:((p==4)?20:((p==5)?20:20))))
class ConnectionPool;

class Role
	:public Creature
{
public:
	Role();
	~Role();
public:
	//-------------------------
	int InitRoleCache(UInt32 roleID, UInt64 connID, ConnectionPool * cp );
	void SetConnectPool(ConnectionPool * cp );
	ConnectionPool *  Cp();
	int DB2Cache();
	int Cache2DB();
	int DB2CacheBonus();
	int Cache2Bonus();
	int UpdateaccountDB();
	void UpdateTotalOnlineSec();
	UInt32 HpMpfullflag;//Hp,Mp是否到最大值的标志
	UInt32 HpCDflag;
	UInt32 MpCDflag;
	UInt32 HpMpflag;
	UInt32 LastHpMpTime;


public:
	//获取属性
	UInt32 MapID();
	UInt16 LastX();
	UInt16 LastY();
	UInt32 LoginTime();
	string Password();
	UInt32 ProID();
	UInt32 GuildID();
	UInt32 Glory();
	UInt32 Crime();
	UInt32 TotalOnlineSec();
	UInt32 LastloginTime();
	UInt32 TopCellNum();

	UInt32 TeamID();
	UInt32 LeaderRoleID();
	Byte TeamFlag();
	Byte ISVIP();//返回vip等级，0-6
	Byte VIP();//返回包括0-6，以及11-16


	
	//属性设置
	void MapID(UInt32 input);
	void LastX(UInt16 input);
	void LastY(UInt16 input);
	void LoginTime(UInt32 input);
	void Password(const string& input );
	void ProID(UInt32 input);
	void GuildID(UInt32 input);
	void LastloginTime(UInt32 input);
	void TopCellNum(UInt32 input);
	void IsVIP(Byte input);
	
	void TeamID(UInt32 input);
	void LeaderRoleID(UInt32 input);
	void TeamFlag(Byte input);
	

	//属性加减
	void AddGlory(Int32 input);
	void AddCrime(Int32 input);
	//--------------------------------------------------------
	Int16 IfTheExpToMax();
	//判断经验是否到达上限
	void Role_ADDLev();
	//人物的升级引起的角色属性变化
	void RoleAddStrength(Int32 input);
	void RoleAddIntelligence(Int32 input);
	void RoleAddAgility(Int32 input);
	void RoleAddStrengthBonus(Int32 input);
	void RoleAddIntelligenceBonus(Int32 input);
	void RoleAddAgilityBonus(Int32 input);
	void RoleAddSKill(Int32 skillID);
	void RoleAddtalentpoint(Int32 talentID);//增加人物加天赋以后的属性变化
    void CdTimetoTheFalg(int kind,UInt32 CdTime);//CD
    void SetHpMpfullStatue();//状态
    void HpMpadd(int time1);
	void RolePKset(Int32 Hp,Int32 Mp);
	Int32 RoleExpAdd(Int32 input);
	
	Byte IsOfflineUpdate();
	void IsOfflineUpdate(Byte input);

	//交易
	TradeItem TradeInfo();
	void TradeInfo(TradeItem tradeItem);
	
	UInt32 EnterMapNum();
	void AddEnterMapNum();

	//设置经验符文
	void SetRoleRuneTime(UInt32 itemId);
	int GetRoleRuneTime(UInt32 itemId);

	void PopulateRoleRuneList(List<RoleRune>& lic) ;

	void UseRune(UInt32 itemId);
private:
	string _ip;								//客户端ip
	UInt16 _port;							//客户端端口
	UInt32 _fd;								//客户端fd
	
	UInt32 _mapID;						//角色所在map
	UInt16 _lastX;						//最近x坐标
	UInt16 _lastY;						//最近y坐标
	UInt32 _loginTime;				//登陆时间
	UInt32 _lastloginTime;			//上次退出时间
	UInt32 _topcellnum;      //背包上限数量

	UInt32 _teamID;          //队伍ID
	UInt32 _leaderRoleID;	//角色ID
	Byte _teamFlag;         //组队标志  0 没有组队 1 （队员）离队 2 （队员）已归队 3 队长	

	string _password;						//登陆密码	
	UInt32 _proID;							//职业ID
	UInt32 _guildID;						//行会ID
	UInt32 _glory;							//荣誉
	UInt32 _crime;							//罪恶值
	UInt32 _totalOnlineSec;		//总在线时长 单位 秒
	Byte  _isOfflineUpdate;		//是否在离线挂机 0 否  1 是
	TradeItem _trade;           //角色交易信息
	UInt32 _enterMapNum;			//进入地图的总次数
	Byte _isvip;          //是否是VIP，0，(11，12，13，14，15，16以前是VIP)目前不是，1-6是
	
	map<UInt32, UInt32> runeMap;// id + activetime

	int _expRuneTime; //最近使用经验符文的时间

	int _toughRuneTime1; //坚韧符文1: 500
	int _toughRuneTime2; //坚韧符文2: 1000
	int _toughRuneTime3; //坚韧符文3: 1500
	int _toughRuneTime4; //坚韧符文4:  2000

	int _stoneRuneTime1;  //石化符文
	int _stoneRuneTime2;  //石化符文
	int _stoneRuneTime3;  //石化符文
	int _stoneRuneTime4;  //石化符文

	int _saintRuneTime1;  //神圣符文
	int _saintRuneTime2;  //神圣符文
	int _saintRuneTime3;  //神圣符文
	int _saintRuneTime4;  //神圣符文

	int _intelliRuneTime1;  //聪明符文
	int _intelliRuneTime2;  //聪明符文
	int _intelliRuneTime3;  //聪明符文
	int _intelliRuneTime4;  //聪明符文

	int _correctRuneTime1;  //精准符文
	int _correctRuneTime2;  //精准符文
	int _correctRuneTime3;  //精准符文
	int _correctRuneTime4;  //精准符文

	int _speedRuneTime1;  //急速符文1
	int _speedRuneTime2;  //急速符文2
	int _speedRuneTime3;  //急速符文3
	int _speedRuneTime4;  //急速符文4

	int _focusRuneTime1;  //专注符文1
	int _focusRuneTime2;  //专注符文2
	int _focusRuneTime3;  //专注符文3
	int _focusRuneTime4;  //专注符文4

	int _flyRuneTime1;  //飞翔符文1
	int _flyRuneTime2;  //飞翔符文2
	int _flyRuneTime3;  //飞翔符文3
	int _flyRuneTime4;  //飞翔符文4

	int _angryRuneTime1;  //狂暴符文1
	int _angryRuneTime2;  //狂暴符文2
	int _angryRuneTime3;  //狂暴符文3
	int _angryRuneTime4;  //狂暴符文4

	int _extremeRuneTime1;  //极限符文1
	int _extremeRuneTime2;  //极限符文2
	int _extremeRuneTime3;  //极限符文3
	int _extremeRuneTime4;  //极限符文4

	//数据库连接池
	ConnectionPool* _cp;
	
};

typedef tr1::shared_ptr<Role> RolePtr;


#endif

