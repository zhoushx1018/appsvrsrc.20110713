#include "Role.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ConnectionPool.h"
#include "DBOperate.h"
#include "Log.h"
#include "ArchvPK.h"


Role::Role()
:_port(0),
_fd(0),
_mapID(0),
_lastX(0),
_lastY(0),
_loginTime(0),
_proID(0),
_guildID(0),
_glory(0),
_crime(0),
HpMpfullflag(0),
MpCDflag(0),
HpCDflag(0),
HpMpflag(0),
LastHpMpTime(0),
_totalOnlineSec(0),
_lastloginTime(0),
_topcellnum(0),
_teamID(0),
_leaderRoleID(0),
_teamFlag(0),
_enterMapNum(0),
_isvip(0),
_expRuneTime(false)

{
	Flag(1);
	Status(1);
	SetHpMpfullStatue();
}

Role::~Role()
{

}

//@brief	初始化角色缓存
//@param	connID	客户端连接ID  64位   包含 ip+port+fd信息
//@param	roleID	角色ID
//@return 0 成功  非0 失败
int Role::InitRoleCache(UInt32 roleID, UInt64 connID, ConnectionPool * cp )
{
	in_addr adrtTmp;
	adrtTmp.s_addr = CONNECTION_IP(connID);

	//最原始的属性赋值
	ID(roleID);
	_cp = cp;
	_ip = inet_ntoa( adrtTmp );
	_port = ntohs(CONNECTION_PORT(connID));
	_fd = CONNECTION_FD(connID);

	//从DB读入更多的角色属性
	int iRet = DB2Cache();

	return iRet;
}

void Role::SetConnectPool(ConnectionPool * cp )
{
	_cp = cp;
}

ConnectionPool * Role::Cp()
{
	return _cp;
}


//DB数据读入缓存，角色登陆时执行
//@return 0 成功  非0 失败
int Role::DB2Cache()
{
DEBUG_PRINTF( "InitCache ---------------->" );
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//获取角色信息
	sprintf( szSql, "select	 RoleID     ,RoleName   ,Password   ,Level      ,Exp ,MaxExp,Camp     \
							   ,ProID      ,GuildID    ,Glory      ,AddPoint   ,Strength \
							   ,Intelligence   ,Agility        ,MoveSpeed      ,HP             ,MP          \
							   ,MaxHP          ,MaxMP          ,HPRegen        ,MPRegen        ,AttackPowerHigh \
							   ,AttackPowerLow    ,AttackScope       ,AttackSpeed       ,BulletSpeed       ,Defence        \
							   ,MDefence          ,CritRate          ,HitRate           ,DodgeRate         ,Crime ,LastloginTime,TopCellNum         \
							   ,TotalOnlineSec,		IsOfflineUpdate ,IsVIP \
							from Role \
							where RoleID = %d ", ID() );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
		return -1;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}

	while(dbo.HasRowData())
	{
		//_ID = dbo.GetIntField(0);
		Name( dbo.GetStringField(1) );
		_password = dbo.GetStringField(2);
		Level( dbo.GetIntField(3) );
		Exp( dbo.GetIntField(4) );
		MaxExp(dbo.GetIntField(5));
		Camp(dbo.GetIntField(6));



		_proID = dbo.GetIntField(7);
		_guildID = dbo.GetIntField(8);
		_glory = dbo.GetIntField(9);
		AddPoint( dbo.GetIntField(10) );
		Strength( dbo.GetIntField(11) );

		Intelligence( dbo.GetIntField(12) );
		Agility( dbo.GetIntField(13) );
		MoveSpeed( dbo.GetIntField(14) );
		Hp( dbo.GetIntField(15) );
		Mp( dbo.GetIntField(16) );

		MaxHp( dbo.GetIntField(17) );
		MaxMp( dbo.GetIntField(18) );
		HpRegen( dbo.GetIntField(19) );
		MpRegen( dbo.GetIntField(20) );
		AttackPowerHigh( dbo.GetIntField(21) );

		AttackPowerLow( dbo.GetIntField(22) );
		AttackScope( dbo.GetIntField(23) );
		AttackSpeed( dbo.GetIntField(24) );
		BulletSpeed( dbo.GetIntField(25) );
		Defence( dbo.GetIntField(26) );

		MDefence( dbo.GetIntField(27) );
		CritRate( dbo.GetIntField(28) );
		HitRate( dbo.GetIntField(29) );
		DodgeRate( dbo.GetIntField(30) );
		_crime = dbo.GetIntField(31);
		_lastloginTime=dbo.GetIntField(32);
		_topcellnum=dbo.GetIntField(33);
		_totalOnlineSec = dbo.GetIntField(34);
		_isOfflineUpdate = dbo.GetIntField(35);
		_isvip=dbo.GetIntField(36);


		//记录集下一条记录
		dbo.NextRow();
	}

	//VIP查看是否过期
	if(_isvip<=10&&_isvip>0)
	{
		sprintf( szSql, "select	 VIPLev from VIP where RoleID=%d ", ID() );
		iRet=dbo.QuerySQL(szSql);
		if(iRet==0)
		{
			_isvip=dbo.GetIntField(0);
		}
		else
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
			return -1;
		}
	}
	//获取上次进入地图,及坐标
	sprintf( szSql, "select	 MapID, LastX, LastY \
	 						from RoleLastLoc \
							where RoleID = %d ", ID() );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
		return -1;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}

	while(dbo.HasRowData())
	{
		_mapID = dbo.GetIntField(0);
		_lastX = dbo.GetIntField(1);
		_lastY = dbo.GetIntField(2);

		//记录集下一条记录
		dbo.NextRow();
	}
//	_isAdult=GetIfIsAdult(ID(),_accountlastloginTime,_accounttopTime);//获取是否成人

	//初始化移动路径,当前方位
	ArchvPosition pos(_lastX,_lastY);
	ArchvRoute route;

	route.time = time(NULL);
	route.listPos.clear();
	route.listPos.push_back(pos);
	Route(route);

	Pos(pos);

	//取得符文时间
	sprintf( szSql, "select	 RuneID, activeTime from RoleRune where RoleID = %d; ", ID() );
	iRet = dbo.QuerySQL(szSql);

	if( 1 == iRet )
	{
		LOG(LOG_DEBUG,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}

	while(dbo.HasRowData())
	{
		runeMap[dbo.GetIntField(0)] = dbo.GetIntField(1);

		//记录集下一条记录
		dbo.NextRow();
	}

	return DB2CacheBonus();
}
int Role::DB2CacheBonus()
{

	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	sprintf( szSql, "select	 RoleID     ,Strength   ,Intelligence   ,Agility      ,MovSpeed ,MaxHP,MaxMP     \
						   ,HPRegen      ,MPRegen    ,AttackPowerHigh      ,AttackPowerLow   ,AttackSpeed \
						   ,Defence   ,MDefence        ,CritRate      ,HitRate             ,DodgeRae          \
						from RoleBonus \
						where RoleID = %d ", ID() );
	iRet=dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
		return -1;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}

	while(dbo.HasRowData())
	{
		StrengthBonus(dbo.GetIntField(1));
		IntelligenceBonus(dbo.GetIntField(2));
		AgilityBonus(dbo.GetIntField(3));
		MovSpeedBonus(dbo.GetIntField(4));
		MaxHpBonus(dbo.GetIntField(5));
		MaxMpBonus(dbo.GetIntField(6));
		HpRegenBonus(dbo.GetIntField(7));
		MpRegenBonus(dbo.GetIntField(8));
		AttackPowerHighBonus(dbo.GetIntField(9));
		AttackPowerLowBonus(dbo.GetIntField(10));
		AttackSpeedBonus(dbo.GetIntField(11));
		DefenceBonus(dbo.GetIntField(12));
		MDefenceBonus(dbo.GetIntField(13));
		CritRateBonus(dbo.GetIntField(14));
		HitRateBonus(dbo.GetIntField(15));
		DodgeRateBonus(dbo.GetIntField(16));
			//记录集下一条记录
		dbo.NextRow();
	}
	return 0;
}
int Role::Cache2Bonus()
{
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;


	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	sprintf(szSql, "update RoleBonus set RoleID  =  %d,Strength  =  %d,  Intelligence  =  %d,  Agility = %d, \
					MovSpeed = %d , MaxHP =  %d  ,MaxMP  =%d ,HPRegen=  %d,MPRegen =%d , \
					AttackPowerHigh = %d, 	AttackPowerLow =%d ,AttackSpeed =  %d ,Defence=  %d, \
					MDefence = %d ,CritRate = %d ,HitRate=%d ,DodgeRae=%d where RoleID=%d;",
			ID(),
			StrengthBonus(),
			IntelligenceBonus(),
			AgilityBonus(),
			MovSpeedBonus(),
			MaxHpBonus(),
			MaxMpBonus(),
			HpRegenBonus(),
			MpRegenBonus(),
			AttackPowerHighBonus(),
			AttackPowerLowBonus(),
			AttackSpeedBonus(),
			DefenceBonus(),
			MDefenceBonus(),
			CritRateBonus(),
			HitRateBonus(),
			DodgeRateBonus(),
			ID() );
	iRet = dbo.ExceSQL(szSql);
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}
 	return 0;
}
//HpMp自动恢复属性
void Role::HpMpadd(int time1) {
	UInt32 hg = (HpRegen() + HpRegenBonus()) / 10;
	UInt32 mg = (MpRegen() + MpRegenBonus()) / 10;
	switch (HpMpfullflag) {
	case 0:
		AddHp(hg * time1);
		AddMp(mg * time1);
		break;
	case 1:
		AddMp(mg * time1);
		break;
	case 2:
		AddHp(hg * time1);
		break;
	case 3:
		break;
	default:
		LOG(LOG_ERROR, __FILE__, __LINE__, "CdTimetoTheFalg error!!");
		break;
	}
	SetHpMpfullStatue();
	LastHpMpTime = time(NULL);

}


//缓存写到DB
//@return 0 成功  非0 失败
int Role::Cache2DB()
{
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;


	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//角色表
	sprintf(szSql, "update Role \
		set RoleID        = %d ,RoleName         = '%s' ,Password         = '%s' ,Level            = %d ,Exp              = %d,MaxExp=%d ,Camp=%d \
		 ,ProID            = %d ,GuildID          = %d ,Glory            = %d ,AddPoint         = %d ,Strength         = %d \
		 ,Intelligence     = %d ,Agility          = %d ,MoveSpeed        = %d ,HP               = %d ,MP               = %d \
		 ,MaxHP            = %d ,MaxMP            = %d ,HPRegen          = %d ,MPRegen          = %d ,AttackPowerHigh  = %d \
		 ,AttackPowerLow   = %d ,AttackScope      = %d ,AttackSpeed      = %d ,BulletSpeed      = %d ,Defence          = %d \
		 ,MDefence         = %d ,CritRate         = %d ,HitRate          = %d ,DodgeRate        = %d ,Crime            = %d \
		 ,LastloginTime=%d,TopCellNum=%d \
		 ,TotalOnlineSec   = %d, IsOfflineUpdate = %d ,IsVIP=%d \
		where RoleId = %d ",
		ID(),
		Name().c_str(),
		_password.c_str(),
		Level(),
		Exp(),
		MaxExp(),
		Camp(),
		_proID,
		_guildID,
		_glory,
		AddPoint(),
		Strength(),
		Intelligence(),
		Agility(),
		MoveSpeed(),
		Hp(),
		Mp(),
		MaxHp(),
		MaxMp(),
		HpRegen(),
		MpRegen(),
		AttackPowerHigh(),
		AttackPowerLow(),
		AttackScope(),
		AttackSpeed(),
		BulletSpeed(),
		Defence(),
		MDefence(),
		CritRate(),
		HitRate(),
		DodgeRate(),
		_crime,
		_lastloginTime,
		_topcellnum,
		_totalOnlineSec,
		_isOfflineUpdate,
		_isvip,
		ID() );

	iRet = dbo.ExceSQL(szSql);
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}

	//角色最近进入的地图及坐标
	sprintf(szSql, "update RoleLastLoc \
		set MapID        = %d, \
			LastX         = %d , \
			LastY 				= %d \
		where RoleID = %d ",
		MapID(), LastX(), LastY(), ID() );
	iRet = dbo.ExceSQL(szSql);
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}
 	return Cache2Bonus();

}




//判断经验是否达到最大值，达到升级处理，没有不做处理
//返回值0没有升级，返回值大于1升级，值为级数,返回-1,异常
Int16 Role::IfTheExpToMax()
{
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	UInt16 lastlev=Level();
	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	while(Exp()>MaxExp())
	{

		//达到升级情况，经验减去最大值

		//Max经验值设定为下一级的最大值
		sprintf(szSql, "select MaxExp from LevelDesc where Level=%d",Level()+1);
		iRet=dbo.QuerySQL(szSql);
		if( 1 == iRet )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
			return -1;
		}
		if( iRet < 0 )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			return -1;
 		}
 		AddAddPoint(AddAddPoint_Role);
 		Exp(Exp()-MaxExp());
 		while(dbo.HasRowData())
		{

			MaxExp(dbo.GetIntField(0));
			//记录集下一条记录
			dbo.NextRow();
		}
		AddLevel(1);
		Role_ADDLev();

	}
	return Level()-lastlev;
}
/*
每一点力量增加5点HP，对于矮人、游侠，每一点力量增加2点攻击；
每一点敏捷增加5点护甲，对于弓箭手、长矛手，每一点敏捷增加2点攻击；
每一点智力增加5点MP，对于男巫师和女法师，每一点智力增加2点攻击；
*/
//增加敏捷
void Role::RoleAddAgility(Int32 input)
{
	AddAgility(input);
	AddHitRateBonus(input);
	if(ProID()==3||ProID()==6)
	{
		AddAttackPowerHigh(2*input);
		AddAttackPowerLow(2*input);
	}

}

void Role::RoleAddAgilityBonus(Int32 input)
{
	AddAgilityBonus(input);
	AddHitRateBonus(input);
	if(ProID()==3||ProID()==6)
	{
		AddAttackPowerHighBonus(2*input);
	//	AddAttackPowerLowBonus(2*input);
	}
}


//增加智力
void Role::RoleAddIntelligence(Int32 input)
{
	AddIntelligence(input);
	AddMaxMp(input*12);//智力增加5点MP
	AddMpRegen(input);
	if(ProID()==4||ProID()==5)
	{
		AddAttackPowerHigh(2*input);
		AddAttackPowerLow(2*input);
	}
}
void Role::RoleAddIntelligenceBonus(Int32 input)
{
		AddIntelligenceBonus(input);
		AddMaxMpBonus(input*12);//智力增加5点MP
		AddMpRegenBonus(input);
		if(ProID()==4||ProID()==5)
		{
			AddAttackPowerHighBonus(2*input);
		//	AddAttackPowerLowBonus(2*input);
		}

}



//增加力量
void Role::RoleAddStrength(Int32 input)
{
	AddStrength(input);
	AddMaxHp(input*10);//增加10点HP
	AddCritRate(input*1);
	AddDefence(input);
	if(ProID()==1||ProID()==2)
	{
		AddAttackPowerHigh(2*input);
		AddAttackPowerLow(2*input);
	}
}
void Role::RoleAddStrengthBonus(Int32 input)
{
	AddStrengthBonus(input);
	AddMaxHpBonus(input*10);//增加10点HP
	AddCritRateBonus(input*1);
	AddDefenceBonus(input);

	if(ProID()==1||ProID()==2)
	{
		AddAttackPowerHighBonus(2*input);
		//AddAttackPowerLowBonus(2*input);
	}
}

//人物升级属性的变化
void Role::Role_ADDLev()
{
	RoleAddStrength(Strength_ADD(ProID()));
	RoleAddIntelligence(Intelligence_ADD(ProID()));
	RoleAddAgility(Agility_ADD(ProID()));

}
//人物技能升级属性的变化
void Role::RoleAddSKill(Int32 skillID)
{

	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	int lev=0;
	list<ArchvSkill> lis;
	list<ArchvSkill>::iterator itor;
	ArchvSkill sk;
	if(AddPoint()<=0)
	{//判断是否有加成点
		LOG(LOG_ERROR,__FILE__,__LINE__,"addpoint <=0 ,RoleID[%d] " ,ID());
		return;
	}

	//判断是否满足升级的要求
	sprintf(szSql, "select SkillID,SkillLev from RoleSkill where RoleID=%d;",ID());
	iRet=dbo.QuerySQL(szSql);
	if(iRet!=0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return;
	}
	while(dbo.HasRowData())
	{
		sk.skillID=dbo.GetIntField(0);
		sk.skillLevel=dbo.GetIntField(1);
		if(sk.skillID==skillID)
		{
			lev=sk.skillLevel;
		}
		lis.push_back(sk);
		dbo.NextRow();
	}
	if((skillID>300&&lev==10)||(skillID<300&&lev==6))
	{

			LOG(LOG_ERROR,__FILE__,__LINE__,"the skill is the top lev skillID=%d,roleID=%d",skillID,ID());
			return;

	}
	sprintf(szSql, "select RequiedSkillID,RequiedSkillLev from SkillRequie where ProID=%d and SkillLev=%d and SkillID=%d",ProID(),lev+1,skillID);
	iRet=dbo.QuerySQL(szSql);
	if(iRet<0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"the skill is the top lev skillID=%d,roleID=%d",skillID,ID());
		return;
		//there is not any to the skill
	}
	else if(iRet==0)
	{
		while(dbo.HasRowData())
		{
			sk.skillID=dbo.GetIntField(0);
			sk.skillLevel=dbo.GetIntField(1);
			for(itor=lis.begin();itor!=lis.end();itor++)
			{
				if(itor->skillID==sk.skillID) break;
			}
			if(itor->skillLevel<sk.skillLevel)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"some erro of the skill add");
				return;
			}
			dbo.NextRow();
		}
	}
	AddAddPoint(-1);


	if(skillID>=301&&skillID<=312)
	{
	//技能为特殊技能，有加成点
		RoleAddtalentpoint(skillID-300);
	}
	sprintf(szSql, "update RoleSkill set SkillLev=SkillLev+1 where RoleID=%d and SkillID=%d;",ID(),skillID);

	iRet=dbo.ExceSQL(szSql);
	if( iRet !=0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or erro ,szSql[%s] " , szSql);

	}
}


//人物天赋加点，的属性影响
void Role::RoleAddtalentpoint(Int32 talentID) {
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	int skilllev;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	sprintf(szSql,
			"select SkillID from RoleSkill where RoleID=%d and SkillID=%d;",
			ID(), talentID + 300);
	iRet = dbo.QuerySQL(szSql);
	if (iRet != 0) {
		LOG(LOG_ERROR, __FILE__, __LINE__,
				"ExceSQL data not found or errro!,szSql[%s] ", szSql);

	}
	while (dbo.HasRowData()) {

		skilllev = dbo.GetIntField(0) + 1;
		//记录集下一条记录
		dbo.NextRow();
	}
	//原始的状态

	switch (talentID) {
	case 5:
		RoleAddStrength(ADD_Skill5(skilllev));
		break;
	case 1:
		RoleAddAgility(ADD_Skill1(skilllev));
		break;
	case 9:
		RoleAddIntelligence(ADD_Skill9(skilllev));
		break;
	case 3:
		AddAttackPowerHigh(ADD_Skill3(skilllev));
		AddAttackPowerLow(ADD_Skill4(skilllev));
		break;
	case 7:
		AddMoveSpeed(ADD_Skill7(skilllev));
		break;
	case 2:
		AddCritRate(ADD_Skill2(skilllev));
		break;
	case 6:
		AddDefence(ADD_Skill6(skilllev));
		break;
	case 12:
		AddHitRate(ADD_Skill12(skilllev));
		break;
	case 11:
		AddDodgeRate(ADD_Skill1(skilllev));
		break;
	case 8:
		AddMDefence(ADD_Skill8(skilllev));
		break;
	case 4:
		AddAttackSpeed(ADD_Skill4(skilllev));
		break;
	case 10:
		if (skilllev == 1) {
			AddHpRegen(2);
			AddMpRegen(4);
		} else {
			AddHpRegen(ADD_Skill10(skilllev));
			AddMpRegen(ADD_Skill10(skilllev));
		}
		break;
	default:
		LOG(LOG_ERROR, __FILE__, __LINE__, "RoleAddtalentpoint error!!");
		break;
	}
}
void Role::CdTimetoTheFalg(int kind, UInt32 CdTime) {
	switch (kind) {
	case 1:
		HpCDflag = time(NULL) + CdTime;
		break;
	case 2:
		MpCDflag = time(NULL) + CdTime;
		break;
	case 3:
		HpMpflag = time(NULL) + CdTime;
		break;
	default:
		LOG(LOG_ERROR, __FILE__, __LINE__, "CdTimetoTheFalg error!!");
		break;
	}
}

void Role::SetHpMpfullStatue()
{
		UInt32 hp=Hp();
		UInt32 mp=Mp();
		UInt32 mxh=MaxHp()+MaxHpBonus();
		UInt32 mxm=MaxHp()+MaxHpBonus();
		if(hp==mxh)
		{

			if(mp==mxm)
			{
				HpMpfullflag=3;
			}
			else
			{
				HpMpfullflag=1;
			}
		}
		else
		{
			if(mp==mxm)
			{
				HpMpfullflag=2;
			}
			else
			{
				HpMpfullflag=0;
			}
		}
}

void Role::UpdateTotalOnlineSec()
{
	Int32	onlineSec = 0;

	if( 0 != _loginTime )
		onlineSec = time(NULL) - _loginTime;
		//最近一次登入退出时间
	_totalOnlineSec += onlineSec;
}


void Role::RolePKset(Int32 HP,Int32 MP)
{
	Hp(HP);
	Mp(MP);
}
//返回1升级，返回-1异常，返回0没有升级
Int32 Role::RoleExpAdd(Int32 input)
{
	int iRet=0;
	AddExp(input);
	if(input>0)
	{
		iRet=IfTheExpToMax();
	}
	return iRet;
}

Byte Role::IsOfflineUpdate()
{
	return _isOfflineUpdate;
}

//@param	input		0 停止离线挂机		1 开始离线挂机
void Role::IsOfflineUpdate(Byte input)
{
	_isOfflineUpdate = input;
}

TradeItem Role::TradeInfo()
{
  return _trade;
}

void Role::TradeInfo(TradeItem tradeItem)
{
  _trade = tradeItem;
}

UInt32 Role::EnterMapNum()
{
	return _enterMapNum;
}

void Role::AddEnterMapNum()
{
	_enterMapNum++;
}

void Role::SetRoleRuneTime(UInt32 itemId){
	runeMap[itemId] = time(NULL);

	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int RetCode = 0;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//更新角色符文数据库
	sprintf(
			szSql,
			"select	* from RoleRune where RoleID = %d and RuneID = %d;",
			ID(), itemId);
	RetCode = dbo.QuerySQL(szSql);
	if (1 == RetCode) {
		sprintf(
				szSql,
				"insert into RoleRune(RoleID,RuneID,activeTime) values(%d,%d,%d);",
				ID(), itemId, time(NULL));

		RetCode = dbo.ExceSQL(szSql);
		if (RetCode != 0) {
			LOG(LOG_ERROR, __FILE__, __LINE__,
					"ExceSQL data insert error ,szSql[%s] ", szSql);
			return;
		}
	} else if (RetCode != 0) {
		LOG(LOG_ERROR, __FILE__, __LINE__,
				"QuerySQL data select error ,szSql[%s] ", szSql);
		return;
	} else {
		sprintf(
				szSql,
				"update RoleRune set activeTime = %d where RoleID=%d and RuneID=%d;",
				time(NULL), ID(), itemId);
		RetCode = dbo.ExceSQL(szSql);
		if (RetCode != 0) {
			LOG(LOG_ERROR, __FILE__, __LINE__,
					"ExceSQL data update error ,szSql[%s] ", szSql);
			return;
		}
	}
}

int Role::GetRoleRuneTime(UInt32 itemId){
	return runeMap[itemId];
}

void Role::PopulateRoleRuneList(List<RoleRune>& lic) {
	for (map<UInt32, UInt32>::iterator iter = runeMap.begin(); iter != runeMap.end(); ++iter) {
		int period = time(NULL) - iter->second;
		if (period < 2 * 3600) {
			RoleRune roleRune;
			roleRune.runeID = iter->first;
			roleRune.remainderTime = 2 * 3600 - period;

			lic.push_back(roleRune);
		}
	}
}


UInt32 Role::MapID()
{
	return _mapID;
}
UInt32 Role::LoginTime()
{
	return _loginTime;
}


//@brief 刷新 lastX 并返回
UInt16 Role::LastX()
{
	_lastX = Pos().X;
	return _lastX;
}

//@brief 刷新 lastY 并返回
UInt16 Role::LastY()
{
	_lastY = Pos().Y;
	return _lastY;
}


string Role::Password()
{
	return _password;
}

UInt32 Role::ProID()
{
	return _proID;
}

UInt32 Role::GuildID()
{
	return _guildID;
}

UInt32 Role::Glory()
{
	return _glory;
}

UInt32 Role::Crime()
{
	return _crime;
}

UInt32 Role::TotalOnlineSec()
{
	return _totalOnlineSec;
}
UInt32 Role::LastloginTime()
{
	return _lastloginTime;
}
UInt32 Role::TopCellNum()
{
	return _topcellnum;
}
UInt32 Role::TeamID()
{
	return _teamID;
}
UInt32 Role::LeaderRoleID()
{
	return _leaderRoleID;
}
Byte Role::TeamFlag()
{
	return _teamFlag;
}
Byte Role::ISVIP()//返回vip等级，0-6
{
	if(_isvip>10)
	{
		return 0;
	}
	else
	{
		return _isvip;
	}
}
Byte Role::VIP()//返回包括0-6，以及11-16
{
	return _isvip;
}



void Role::MapID(UInt32 input)
{
	_mapID = input;
}

void Role::LastX(UInt16 input)
{
	_lastX = input;
}

void Role::LastY(UInt16 input)
{
	_lastY = input;
}

void Role::LoginTime(UInt32 input)
{
	_loginTime = input;
}

void Role::LastloginTime(UInt32 input)
{
	_lastloginTime=input;
}
void Role::TopCellNum(UInt32 input)
{
	_topcellnum=input;
}

void Role::Password(const string& input )
{
	_password = input;
}

void Role::ProID(UInt32 input)
{
	_proID = input;
	Type(_proID);
}

void Role::GuildID(UInt32 input)
{
	_guildID = input;
}

void Role::AddGlory(Int32 input)
{
	_glory += input;
}
void Role::AddCrime(Int32 input)
{
	_crime += input;
}



void Role::TeamID(UInt32 input)
{
	_teamID=input;
}
void Role::LeaderRoleID(UInt32 input)
{
	_leaderRoleID=input;
}
void Role::TeamFlag(Byte input)
{
	_teamFlag=input;
}
void Role::IsVIP(Byte input)
{
	_isvip=input;
}
