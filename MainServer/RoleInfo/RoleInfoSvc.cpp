#include "RoleInfoSvc.h"
#include "MainSvc.h"
#include "ArchvTask.h"
#include "DBOperate.h"
#include "ArchvTask.h"
#include "CoreData.h"
#include "ArchvRole.h"
#include "SSClientManager.h"
#include "Role.h"
#include "Timer.h"
#include "./Pet/ArchvPet.h"
#include "./Task/TaskSvc.h"

#include "NewTimer.h"

struct RoleAdultTimerCallBackParam
{
	int roleID;
	int type;
};

RoleInfoSvc::RoleInfoSvc(void* service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;
}

RoleInfoSvc::~RoleInfoSvc()
{
	for (std::list<NewTimer*>::iterator it = _timerList.begin(); it != _timerList.end(); ++it)
	{
		delete *it;
	}
	_timerList.clear();
}

void RoleInfoSvc::OnProcessPacket(Session& session,Packet& packet)
{

DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 301:	//查询角色属性 from DB
			ProcessGetRoleInfoFromDB(session,packet);
			break;

		case 302:	//查询角色简要属性
			ProcessGetRoleBriefInfo(session,packet);
			break;

		case 303://查询人物的技能
			ProcessGetRoleSkill(session,packet);
			break;

		case 304://人物的技能升级
			ProcessRoleSkillAdd(session,packet);
			break;

		case 305:	//查询角色属性 from Cache
			ProcessGetRoleInfoFromCache(session,packet);
			break;

		default:
			ClientErrorAck(session,packet,ERR_SYSTEM_PARAM);
			LOG(LOG_ERROR,__FILE__,__LINE__,"MsgType[%d] not found",packet.MsgType);
			break;
		}
}


//客户端错误应答
//@param  session 连接对象
//@param	packet 请求包
//@param	RetCode 返回errorCode 值
void RoleInfoSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
{
	//组应答数据
	DataBuffer	serbuffer(1024);
	Packet p(&serbuffer);
	Serializer s(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();

	s<<RetCode;
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

DEBUG_PRINTF1( "C_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

}



//调整X轴坐标
//	场景坐标-->pk坐标
//@param	X	场景X坐标
//@param  originX 场景pk原点 X坐标
//@return  X坐标
int RoleInfoSvc::AdjustCoordinateX( UInt32 X, UInt32 originX )
{
	int iTmp = X - originX;

	if( iTmp <= 0 )
		return 0;
	else if( iTmp > PKSCREEN_XLENGTH )
		return PKSCREEN_XLENGTH;
	else
		return iTmp;
}



//调整Y轴坐标
//	场景坐标-->pk坐标
//@param	Y	场景Y坐标
//@param  originX 场景pk原点 Y坐标
//@return  Y坐标
int RoleInfoSvc::AdjustCoordinateY( UInt32 Y, UInt32 originY )
{
	int iTmp = Y - originY;

	if( iTmp <= 0 )
		return 0;
	else if( iTmp > PKSCREEN_YLENGTH )
		return PKSCREEN_YLENGTH;
	else
		return iTmp;
}




void RoleInfoSvc::ProcessGetRoleInfoFromDB(Session& session,Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(2048);
	int iRet = 0;

	ArchvRoleInfo ri;
	ArchvRoleBonus ri1;
	UInt32 roleID = 0;
	RolePtr role(new Role());
	Byte IsAdult=0;
	UInt32 LastloginTime=0,TopTime=0;
	List<UInt32> it;
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	UInt32 id = packet.RoleID;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
DEBUG_PRINTF2( "DB Poll GetBusyListSize[%d], GetIdleListSize[%d]", _cp->GetBusyListSize(), _cp->GetIdleListSize());


DEBUG_PRINTF( "hello world +++++++++++++++++++++++");

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>roleID;
//	LOG(LOG_ERROR,__FILE__,__LINE__,"Role[%d]",roleID);
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

DEBUG_PRINTF1( " --roleID[%d] \n", roleID );
	iRet = role->InitRoleCache( roleID, 0, _cp );
	if(iRet)
	{
		RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"InitRoleCache error, roleID[%d]", roleID  );
		goto EndOf_Process;
	}


	//宠物信息


	sprintf( szSql, "select PetID,PetType,PetName,Level from Pet where roleID=%d and IsUse=1;", role->ID());
	iRet=dbo.QuerySQL(szSql);
	if(iRet==0)
	{
		if(dbo.RowNum()==1)
		{
			ri.petbrif.petID=dbo.GetIntField(0);
			ri.petbrif.PetType=dbo.GetIntField(1);
			ri.petbrif.PetName=dbo.GetStringField(2);
			ri.petbrif.Level=dbo.GetIntField(3);
			ri.petbrif.IsOut=1;//数据没确定
		}
		else
		{
			//数据不只是一条
			LOG(LOG_ERROR,__FILE__,__LINE__,"data erro more than two Pet in the  Role out");
		}
	}
	else if(iRet==1)
	{//数据不存在
		ri.petbrif.PetName="";
	}
	else
	{
	//错误
		LOG(LOG_ERROR,__FILE__,__LINE__,"Querysql erro! ");
	}
	IsAdult=GetIfIsAdult(role->ID());
	ri.roleId = role->ID();
	ri.roleName = role->Name();
	ri.level = role->Level();
	ri.exp = role->Exp();
	ri.maxExp = role->MaxExp();
	ri.proID= role->ProID();
	ri.guildID = role->GuildID();
	ri.glory = role->Glory();
	ri.hp = role->Hp();
	ri.mp = role->Mp();
	ri.maxHp = role->MaxHp();
	ri.maxMp = role->MaxMp();
	ri.attackPowerHigh = role->AttackPowerHigh();
	ri.attackPowerLow = role->AttackPowerLow();
	ri.defence = role->Defence();
	ri.mDefence = role->MDefence();
	ri.critRate = role->CritRate();
	ri.crime  = role->Crime();
	ri.addPoint = role->AddPoint();
	ri.strength = role->Strength();
	ri.intelligence = role->Intelligence();
	ri.agility = role->Agility();
	ri.lastMapID = role->MapID();
	ri.lastX = role->LastX();
	ri.lastY = role->LastY();
	ri.hitRate = role->HitRate();
	ri.dodgeRate = role->DodgeRate();
	ri.attackSpeed = role->AttackSpeed();
	ri.MPRegen = (role->MpRegen())/10;
	ri.HPRegen = (role->HpRegen())/10;
	ri.moveSpeed=role->MoveSpeed();
	ri.IsAdult=IsAdult;
	ri.vipLevel = role->VIP();


	ri1.agility=role->AgilityBonus();
	ri1.attackPowerHigh=role->AttackPowerHighBonus();
	ri1.attackPowerLow=role->AttackPowerLowBonus();
	ri1.attackSpeed=role->AttackSpeedBonus();
	ri1.critRate=role->HitRateBonus();
	ri1.defence=role->DefenceBonus();
	ri1.dodgeRate=role->DodgeRateBonus();
	ri1.hitRate=role->HitRateBonus();
	ri1.HPRegen=(role->MpRegenBonus())/10;
	ri1.MPRegen=(role->HpRegenBonus())/10;
	ri1.intelligence=role->IntelligenceBonus();
	ri1.maxHp=role->MaxHpBonus();
	ri1.maxMp=role->MaxMpBonus();
	ri1.mDefence=role->MDefenceBonus();
	ri1.moveSpeed=role->MovSpeedBonus();
	ri1.strength=role->StrengthBonus();

	/*LOG(LOG_ERROR,__FILE__,__LINE__,"MsyType[301]--role[%d]--vip[%d]",roleID,ri.vipLevel);
	LOG(LOG_ERROR,__FILE__,__LINE__,"level[%d]",role.Level());
	LOG(LOG_ERROR,__FILE__,__LINE__,"exp[%d]",role.Exp());
	LOG(LOG_ERROR,__FILE__,__LINE__,"maxExp[%d]",role.MaxExp());
	LOG(LOG_ERROR,__FILE__,__LINE__,"hp[%d]--maxhp[%d]",role.Hp(),role.MaxHp());
	LOG(LOG_ERROR,__FILE__,__LINE__,"mp[%d]--maxmp[%d]",role.Mp(),role.MaxMp());*/



EndOf_Process:

	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode 为0 才会返回包体剩下内容
		s<<ri;
		s<<ri1;


	}

	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

DEBUG_PRINTF1( "C_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
	if(roleID==id)
	{
			if(IsAdult==0)
			{// 对没有身份证注册的，发送首次登入S-C,延迟5秒
				/*
					Timer * timer = new Timer(0);
					timer->SetCallbackFun( HandleEvent1, this,	id );
					timer->CountDown(5);																			//倒计时 5秒
				*/
				
				NewTimer* pTimer = new NewTimer;
				pTimer->Type(NewTimer::OnceTimer);
				pTimer->Interval(1);
				RoleAdultTimerCallBackParam param;
				param.roleID = roleID;
				param.type = 3;
				pTimer->SetCallbackFun(HandleEvent1, this, &param, sizeof(param));
				pTimer->start(&(_mainSvc->_tm));
				_timerList.push_back(pTimer);											
			}
	}
}

 void RoleInfoSvc::HandleEvent1( void * obj, void * arg, int argLen)
{
	RoleInfoSvc * currObj = (RoleInfoSvc *)obj;
	RoleAdultTimerCallBackParam param = *(RoleAdultTimerCallBackParam*)arg;
	currObj->NotifyCtAdult(param.roleID,param.type);
}





void RoleInfoSvc::ProcessGetRoleBriefInfo(Session& session,Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	int iRet = 0;

	UInt32 roleID = packet.RoleID;
	ArchvRoleBriefInfo rbi;

DEBUG_PRINTF2( "DB Poll GetBusyListSize[%d], GetIdleListSize[%d]", _cp->GetBusyListSize(), _cp->GetIdleListSize());

	//序列化类
	Serializer s(packet.GetBuffer());
//	s>>roleID;

	RolePtr pRole=_mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

DEBUG_PRINTF1( " --roleID[%d] \n", roleID );
	if( 0 == pRole->ID() )
	{
		RetCode = ERR_APP_ROLENOTEXISTS;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error,role not exists, roleID[%d]", roleID  );
		goto EndOf_Process;
	}

	rbi.roleId = pRole->ID();
	rbi.roleName = pRole->Name();
	rbi.level = pRole->Level();
	rbi.proID = pRole->ProID();
	rbi.hp = pRole->Hp();
	rbi.mp = pRole->Mp();
	rbi.mapID = pRole->MapID();
	rbi.lastX = pRole->LastX();
	rbi.lastY = pRole->LastY();

EndOf_Process:

	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode 为0 才会返回包体剩下内容
		s<<rbi;
	}

	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

DEBUG_PRINTF1( "C_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

}

//msgtype 303技能查询
void RoleInfoSvc::ProcessGetRoleSkill(Session& session,Packet& packet)
{

			UInt32	RetCode = 0;
			DataBuffer	serbuffer(1024);
			int iRet = 0;
			char szSql[1024];
			UInt32 roleID = packet.RoleID;

			List<ArchvSkill> SK;
			ArchvSkill skill;
			int i;
			Connection con;
			DBOperate dbo;
			//序列化类
			Serializer s(packet.GetBuffer());
			//s>>SK;
			if( s.GetErrorCode()!= 0 )
			{
				RetCode = ERR_SYSTEM_SERERROR;
				LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
				goto EndOf_Process;
			}

			con = _cp->GetConnection();
			dbo.SetHandle(con.GetHandle());
			sprintf( szSql, "select SkillID,SkillLev from RoleSkill where RoleID=%d;", roleID);
			iRet=dbo.QuerySQL(szSql);
			if(iRet==0)
			{
						while(dbo.HasRowData())
						{
								skill.skillID=dbo.GetIntField(0);
								skill.skillLevel=dbo.GetIntField(1);
								SK.push_back(skill);
							dbo.NextRow();
						}

			}
			else
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL erro" );
			}

		EndOf_Process:

		//组应答数据
		Packet p(&serbuffer);
		s.SetDataBuffer(&serbuffer);
		serbuffer.Reset();

		p.CopyHeader(packet);
		p.Direction = DIRECT_C_S_RESP;
		p.PackHeader();


		s<<RetCode;
		if( 0 == RetCode )
		{//RetCode 为0 才会返回包体剩下内容
			s<<SK;
		}

		p.UpdatePacketLength();

		//发送应答数据
		if( session.Send(&serbuffer) )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
		}

		DEBUG_PRINTF1( "C_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
		DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );


}

//msgtype 304 技能加点
void RoleInfoSvc::ProcessRoleSkillAdd(Session& session, Packet& packet) {
	UInt32 RetCode = 0;
	DataBuffer serbuffer(1024);
	int iRet = 0;
	char szSql[1024];
	UInt32 roleID = packet.RoleID;
	ArchvSkill a;
	a.skillID = 201;
	a.skillLevel = 1;

	List<ArchvSkill>::iterator itor;
	List<ArchvSkill> SK;
	List<ArchvSkill> Sk1; //存储人物技能的
	int i;

	//序列化类
	//先计算天赋，后计算角色
	Serializer s(packet.GetBuffer());
	s >> SK;

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	if (s.GetErrorCode() != 0) {
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR, __FILE__, __LINE__, "serial error");
		goto EndOf_Process;
	}
	for (itor = SK.begin(); itor != SK.end(); itor++) {
		if (itor->skillID > 300) {
			for (int i = itor->skillLevel; i > 0; i--) {
				pRole->RoleAddSKill(itor->skillID);
			}
		} else {
			Sk1.push_back(*itor);
		}
	}

	for (itor = Sk1.begin(); itor != Sk1.end(); itor++) {
		for (int i = itor->skillLevel; i > 0; i--) {
			pRole->RoleAddSKill(itor->skillID);
		}
	}

	EndOf_Process:

	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();

	s << RetCode;
	if (0 == RetCode) { //RetCode 为0 才会返回包体剩下内容
						//s<<SK;
	}

	p.UpdatePacketLength();

	//发送应答数据
	if (session.Send(&serbuffer)) {
		LOG(LOG_ERROR, __FILE__, __LINE__, "session.Send error ");
	}

	DEBUG_PRINTF1( "C_S ack pkg ----- MsgType[%d]  \n", packet.MsgType);
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__,
			__LINE__);
	if (0 == RetCode) {
		NotifySKill(roleID, SK);
		NotifyRolePoint(roleID, pRole->AddPoint());
		if (Sk1.size() != SK.size())
			Notifyinfo(roleID, pRole);

		//触发相关任务状态
		_mainSvc->GetTaskSvc()->OnRoleLearnSkill(roleID);
	}
}





void RoleInfoSvc::ProcessGetRoleInfoFromCache(Session& session,Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(2048);
	int iRet = 0;

	ArchvRoleInfo ri;
	ArchvRoleBonus ri1;
	UInt32 roleID;

	Byte IsAdult=0;
	UInt32 LastloginTime=0,TopTime=0;
	List<UInt32> it;
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	UInt32 id = packet.RoleID;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>roleID;


	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	//LOG(LOG_ERROR,__FILE__,__LINE__,"Role[%d]",roleID);
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//刷新角色所在地图,所有角色的当前位置
	_mainSvc->GetCoreData()->ProcessCalcAllRolePos(roleID);

	//查询角色信息
	pRole->Print();
	if(pRole->ID()==0)
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"role not found, roleID[%d]", roleID  );
		goto EndOf_Process;
	}

DEBUG_PRINTF3( " --roleID[%d] lastX[%d], lastY[%d] \n",
	roleID, pRole->LastX(), pRole->LastY() );

	//宠物信息
	sprintf( szSql, "select PetID,PetType,PetName,Level \
										from Pet \
										where roleID=%d \
											and IsUse = 1;", pRole->ID());
	iRet=dbo.QuerySQL(szSql);
	if(iRet==0)
	{
		if(dbo.RowNum()==1)
		{
			ri.petbrif.petID=dbo.GetIntField(0);
			ri.petbrif.PetType=dbo.GetIntField(1);
			ri.petbrif.PetName=dbo.GetStringField(2);
			ri.petbrif.Level=dbo.GetIntField(3);
			ri.petbrif.IsOut=1;//数据没确定
		}
		else
		{
			//数据不只是一条
			LOG(LOG_ERROR,__FILE__,__LINE__,"data erro more than two Pet in the  Role out");
		}
	}
	else if(iRet==1)
	{//数据不存在
		ri.petbrif.PetName="";
	}
	else
	{
	//错误
		LOG(LOG_ERROR,__FILE__,__LINE__,"Querysql erro! ");
	}
	IsAdult=GetIfIsAdult(pRole->ID());
	ri.roleId = pRole->ID();
	ri.roleName = pRole->Name();
	ri.level = pRole->Level();
	ri.exp = pRole->Exp();
	ri.maxExp = pRole->MaxExp();
	ri.proID= pRole->ProID();
	ri.guildID = pRole->GuildID();
	ri.glory = pRole->Glory();
	ri.hp = pRole->Hp();
	ri.mp = pRole->Mp();
	ri.maxHp = pRole->MaxHp();
	ri.maxMp = pRole->MaxMp();
	ri.attackPowerHigh = pRole->AttackPowerHigh();
	ri.attackPowerLow = pRole->AttackPowerLow();
	ri.defence = pRole->Defence();
	ri.mDefence = pRole->MDefence();
	ri.critRate = pRole->CritRate();
	ri.crime  = pRole->Crime();
	ri.addPoint = pRole->AddPoint();
	ri.strength = pRole->Strength();
	ri.intelligence = pRole->Intelligence();
	ri.agility = pRole->Agility();
	ri.lastMapID = pRole->MapID();
	ri.lastX = pRole->LastX();
	ri.lastY = pRole->LastY();
	ri.hitRate = pRole->HitRate();
	ri.dodgeRate = pRole->DodgeRate();
	ri.attackSpeed = pRole->AttackSpeed();
	ri.MPRegen = (pRole->MpRegen())/10;
	ri.HPRegen = (pRole->HpRegen())/10;
	ri.moveSpeed=pRole->MoveSpeed();
	ri.IsAdult=IsAdult;
	ri.vipLevel = pRole->VIP();


	ri1.agility=pRole->AgilityBonus();
	ri1.attackPowerHigh=pRole->AttackPowerHighBonus();
	ri1.attackPowerLow=pRole->AttackPowerLowBonus();
	ri1.attackSpeed=pRole->AttackSpeedBonus();
	ri1.critRate=pRole->HitRateBonus();
	ri1.defence=pRole->DefenceBonus();
	ri1.dodgeRate=pRole->DodgeRateBonus();
	ri1.hitRate=pRole->HitRateBonus();
	ri1.HPRegen=(pRole->MpRegenBonus())/10;
	ri1.MPRegen=(pRole->HpRegenBonus())/10;
	ri1.intelligence=pRole->IntelligenceBonus();
	ri1.maxHp=pRole->MaxHpBonus();
	ri1.maxMp=pRole->MaxMpBonus();
	ri1.mDefence=pRole->MDefenceBonus();
	ri1.moveSpeed=pRole->MovSpeedBonus();
	ri1.strength=pRole->StrengthBonus();

	//LOG(LOG_ERROR,__FILE__,__LINE__,"MsyType[305]-role[%d]--vip[%d]",roleID,ri.vipLevel);

EndOf_Process:

	//组应答数据
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode 为0 才会返回包体剩下内容
		s<<ri;
		s<<ri1;


	}

	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

DEBUG_PRINTF1( "C_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
}



void RoleInfoSvc::GetSkill( UInt32 RoleID, List<ArchvSkill>& SK)
{
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	UInt16 ProID;
	ArchvSkill skill;
	UInt16 Skill_StartNum;
	int i;

	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	//读取人物的技能包括天赋技能，以及等级
	sprintf( szSql, "select SkillID,SkillLev from RoleSkill where RoleID=%d and SkillID<301 and SkillID>312;", RoleID);
	iRet=dbo.QuerySQL(szSql);
	if(iRet==1)
	{//查询的数据部存在
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL Not find[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
	}
		if(iRet==0)
		{
				while(dbo.HasRowData())
				{
						skill.skillID=dbo.GetIntField(0);
						skill.skillLevel=dbo.GetIntField(1);
						SK.push_back(skill);
						dbo.NextRow();
				}
		}
		else
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL Not find or erro![%s] ,szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		}
		//基本技能

		//装备技能函数
		sprintf( szSql, "select SkillID,SkillLevel \
							from ItemSkill where ItemID in (select ItemID from Equip where RoleID=%d)", RoleID);
		iRet=dbo.QuerySQL(szSql);

		if(iRet<0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		}
		if(iRet==0)
		{
			//有装备技能
			while(dbo.HasRowData())
			{
				skill.skillID=dbo.GetIntField(0);
				skill.skillLevel=dbo.GetIntField(1);
				SK.push_back(skill);
				dbo.NextRow();
			}
		}


	}

void RoleInfoSvc::NotifySKill(UInt32 roleID,List<ArchvSkill>& Sk)
{
			int iRet = 0;
			char szSql[1024];
			char sztemp[128];
			UInt16 count=0;

			List<ArchvSkill>::iterator itor;
			List<ArchvSkill> sk1;
			DataBuffer	serbuffer(8196);
			Serializer s( &serbuffer );
			Packet p(&serbuffer);
			serbuffer.Reset();
			p.Direction = DIRECT_S_C_REQ;
			p.SvrType = 1;
			p.SvrSeq = 1;
			p.MsgType = 304;
			p.UniqID = 217;
			List<UInt32> it;
			p.PackHeader();

			ArchvSkill skill;
			int i;
			Connection con;
			DBOperate dbo;
			//序列化类

			con = _cp->GetConnection();
			dbo.SetHandle(con.GetHandle());
			sprintf( szSql, "select SkillID,SkillLev from RoleSkill where RoleID =%d and SkillID in(", roleID);
			count=Sk.size();
			for(itor=Sk.begin();itor!=Sk.end();itor++)
			{
				if(count==1)
				{
					sprintf( sztemp, "%d);",itor->skillID);
					strcat(szSql,sztemp);
				}
				else
				{
					sprintf( sztemp, "%d, ",itor->skillID);
					strcat(szSql,sztemp);
				}
				count--;
			}

			iRet=dbo.QuerySQL(szSql);
			if(iRet==0)
			{
				while(dbo.HasRowData())
				{
						skill.skillID=dbo.GetIntField(0);
						skill.skillLevel=dbo.GetIntField(1);
						sk1.push_back(skill);
					dbo.NextRow();
				}

			}
			else
			{

			}
			s<<sk1;
			p.UpdatePacketLength();
			it.push_back(roleID);
			if( _mainSvc->Service()->Broadcast( it, &serbuffer))
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
			}

			DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, it.size() );
			DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}
void RoleInfoSvc::Notifyinfo(UInt32 roleID)
{
			DataBuffer	serbuffer(8196);
			ArchvRoleChange ch;

			RolePtr pRole= _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
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

			ch.level = pRole->Level();
			ch.exp = pRole->Exp();
			ch.maxExp = pRole->MaxExp();
			ch.glory = pRole->Glory();
			ch.hp = pRole->Hp();
			ch.mp = pRole->Mp();
			ch.maxHp = pRole->MaxHp();
			ch.maxMp = pRole->MaxMp();
			ch.attackPowerHigh = pRole->AttackPowerHigh();
			ch.attackPowerLow = pRole->AttackPowerLow();
			ch.defence = pRole->Defence();
			ch.mDefence = pRole->MDefence();
			ch.critRate = pRole->CritRate();
			ch.crime  = pRole->Crime();
			ch.addPoint = pRole->AddPoint();
			ch.strength = pRole->Strength();
			ch.intelligence = pRole->Intelligence();
			ch.agility = pRole->Agility();
			ch.hitRate = pRole->HitRate();
			ch.dodgeRate = pRole->DodgeRate();
			ch.attackSpeed = pRole->AttackSpeed();
			ch.MPRegen = pRole->MpRegen();
			ch.HPRegen = pRole->HpRegen();
			ch.moveSpeed=pRole->MoveSpeed();

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

void RoleInfoSvc::Notifyinfo(UInt32 roleID,RolePtr& role)
{
			DataBuffer	serbuffer(8196);
			ArchvRoleChange ch;
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

void RoleInfoSvc::NotifyExp(UInt32 roleID)
{
	DataBuffer	serbuffer(1024);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	List<UInt32> it;
	it.push_back(roleID);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 306;
	p.UniqID = 222;
	p.PackHeader();

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	s<<pRole->Exp();
	p.UpdatePacketLength();
	if(_mainSvc->Service()->Broadcast(it, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}

//	LOG(LOG_DEBUG,__FILE__,__LINE__,"3333333  roleID[%d] exp[%d] ", roleID, pRole->Exp());

	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, it.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}

void RoleInfoSvc::NotifyRolePoint(UInt32 roleID,UInt32 point)
{
	DataBuffer	serbuffer(1024);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	List<UInt32> it;
	it.push_back(roleID);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 308;
	p.UniqID = 218;
	p.PackHeader();
	s<<point;
	p.UpdatePacketLength();
	if(_mainSvc->Service()->Broadcast(it, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, it.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}

void RoleInfoSvc::NotifyCtAdult(UInt32 roleID,Byte Type)
{
	DataBuffer	serbuffer(1024);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	List<UInt32> it;
	it.push_back(roleID);
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
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, it.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}
void RoleInfoSvc::RoleExpAddToDB(UInt32 roleID,int num)
{
		char szSql[256];
		Connection con;
		DBOperate dbo;
		int iRet = 0;
		UInt32 maxExp=0;
		UInt32 exp=0;
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
		sprintf(szSql,"select Exp,MaxExp from Role where RoleID=%d",roleID);

		iRet=dbo.QuerySQL(szSql);
		if( 0 == iRet )
		{
			exp=dbo.GetIntField(0);
			maxExp=dbo.GetIntField(1);
		}
		else
		{
				LOG(LOG_ERROR,__FILE__,__LINE__,"sql erro or not found sql [%s]",szSql);
				return;
		}

		if( exp+num<maxExp)
		{
				sprintf( szSql, "update Role set Exp=exp+num where roleID=%d ",roleID );
				iRet=dbo.ExceSQL(szSql);
				if(iRet!=0)
				{
					LOG(LOG_ERROR,__FILE__,__LINE__,"sql erro or not found sql [%s]",szSql);
					return;
				}
		}
		else
		{
			RolePtr role(new Role());
			role->InitRoleCache(roleID,0,_cp);
			role->RoleExpAdd(num);
			role->Cache2DB();
		}


}


Byte RoleInfoSvc::GetIfIsAdult(UInt32 roleID)
{
		char szSql[256];
		Connection con;
		DBOperate dbo;
		int iRet = 0;
		Byte is=0;
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
		sprintf( szSql, "select IsAdult from Account where AccountID=(select AccountID from Role where RoleID=%d)",roleID );
		iRet=dbo.QuerySQL(szSql);
		if( 0 == iRet )
		{
			is=dbo.GetIntField(0);
		}
		else
		{
				LOG(LOG_ERROR,__FILE__,__LINE__,"GetAdult erro" );
		}



		return is;

}
