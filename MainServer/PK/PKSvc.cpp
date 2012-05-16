#include "PKSvc.h"
#include "MainSvc.h"
#include "ArchvTask.h"
#include "DBOperate.h"
#include "ArchvTask.h"
#include "CoreData.h"
#include "ArchvRole.h"
#include "SSClientManager.h"
#include "Role.h"
#include "ArchvMap.h"
#include "Monster.h"
#include "Team.h"
#include "../Avatar/AvatarSvc.h"
#include "../Avatar/ArchvAvatar.h"

PKSvc::PKSvc(void* service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;
}

PKSvc::~PKSvc()
{
}

void PKSvc::OnProcessPacket(Session& session,Packet& packet)
{
	
DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 401:	//请求PK
			ProcessPKReq(session,packet);
			break;

		default:
			ClientErrorAck(session,packet,ERR_SYSTEM_PARAM);
			LOG(LOG_ERROR,__FILE__,__LINE__,"MsgType[%d] not found",packet.MsgType);
			break;
		}
}


//@brief 客户端错误应答
//@param  session 连接对象
//@param	packet 请求包
//@param	RetCode 返回errorCode 值
void PKSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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



//@brief 调整X轴坐标
//	场景坐标-->pk坐标
//@param	X	场景X坐标
//@param  originX 场景pk原点 X坐标
//@return  X坐标
int PKSvc::AdjustCoordinateX( UInt32 X, UInt32 originX )
{
	int iTmp = X - originX;
	
	if( iTmp <= 0 )
		return 0;
	else if( iTmp >= PKSCREEN_XLENGTH )
		return PKSCREEN_XLENGTH-1;
	else
		return iTmp;
}



//@brief 调整Y轴坐标
//	场景坐标-->pk坐标
//@param	Y	场景Y坐标
//@param  originX 场景pk原点 Y坐标
//@return  Y坐标
int PKSvc::AdjustCoordinateY( UInt32 Y, UInt32 originY )
{
	int iTmp = Y - originY;
	
	if( iTmp <= 0 )
		return 0;
	else if( iTmp >= PKSCREEN_YLENGTH )
		return PKSCREEN_YLENGTH-1;
	else
		return iTmp;
}





//@brief 计算指定两点间的中点
//@return  0 成功  1 查找不成功
ArchvPosition PKSvc::GetMidpoint( 	const ArchvPosition &pos1, const ArchvPosition &pos2 )
{
	ArchvPosition retPos;
	retPos.X = (pos1.X + pos2.X)/2;
	retPos.Y = (pos1.Y + pos2.Y)/2;

	return retPos;
}

//@brief 计算 PK坐标原点
ArchvPosition PKSvc::CalcPKOrigin( const ArchvPosition &input )
{
	ArchvPosition origin;
	
	if( input.X - (PKSCREEN_XLENGTH/2) <= 0 )
		origin.X = 0;
	else
		origin.X = input.X - (PKSCREEN_XLENGTH/2);


	if( input.Y - (PKSCREEN_YLENGTH/2) <= 0 )
		origin.Y = 0;
	else
		origin.Y = input.Y - (PKSCREEN_YLENGTH/2);

 	return origin;
}

//@brief	获取生物状态列表
//@return 空
void PKSvc::GetCtStatus( List<ArchvRolePKInfo>& lrpki, List<ArchvCreatureStatus>& lcs )
{
	List<ArchvRolePKInfo>::iterator it;
	ArchvCreatureStatus cs;
	Byte num=0;
	lcs.clear();
	for( it = lrpki.begin(); it != lrpki.end(); it++ )
	{
		if(it->creatureFlag!=2)
		{
			cs.creatureFlag = it->creatureFlag;
			cs.creatureType = it->creatureType;
			cs.ID = it->roleID;
			cs.status = 2;					//战斗
		}
		else
		{
			if(num==0)
			{
				cs.creatureFlag = it->creatureFlag;
				cs.creatureType = it->creatureType;
				cs.ID = it->roleID;
				cs.status = 2;		
				num++;
			}
			else
			{
				continue;
			}
			
		}
		lcs.push_back(cs);
	}
}


//@brief 组建 S_S pk请求包:  for 角色
//@lrpki		返回的组建结果
//@return 0 成功  非0 失败,type为1表示攻击者，为2表示被攻击者，区分阵营
int PKSvc::MakeSSPkgForRole( 	const ArchvPosition &posPKOrigin, RolePtr &role, List<ArchvRolePKInfo> &lrpki,Byte type)
{
	ArchvRolePKInfo pkInfo;
	ArchvAvatarDescBrief adb;
	List<ArchvAvatarDescBrief> ladb;
	ArchvPosition rolePosition;
	List<UInt32> lrid;
	ArchvSkill skill;
	char szSql[1024];
	Connection con;
  DBOperate dbo;
	int iRet = 0;
		
		//获取DB连接
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());

	//赋值
	pkInfo.controlID = role->ID();
	pkInfo.roleID = role->ID();
	pkInfo.level = role->Level();
	if(type==1)
	{
		pkInfo.opposition = 1;
	}
	else
	{
		pkInfo.opposition = 2;
	}
	pkInfo.creatureFlag = 1;
	pkInfo.creatureType = role->ProID();
//	pkInfo.origin = 1;
	//pkInfo.live = 1;
	pkInfo.maxHP = role->MaxHp()+role->MaxHpBonus();
	pkInfo.maxMP =role->MaxMp()+role->MaxMpBonus();
	pkInfo.hp = role->Hp();
	//+(time(NULL)-role->LastHpMpTime)*(role->HpRegen());
	pkInfo.mp = role->Mp();
	//+(time(NULL)-role->LastHpMpTime)*(role->MpRegen());
	pkInfo.moveSpeed = role->MoveSpeed()+role->MovSpeedBonus();
	

	
//pk坐标处理
//	对超出pk地图范围的坐标进行调整
  pkInfo.currPosX = posPKOrigin.X;
	pkInfo.currPosY = posPKOrigin.Y;
LOG(LOG_DEBUG,__FILE__,__LINE__,"----------->>>>roleID [%d], currPosX x[%d],y[%d]  ",
	role->ID(), pkInfo.currPosX, pkInfo.currPosY );
	
	pkInfo.direct = role->Direct();
	pkInfo.attackPowerHigh = role->AttackPowerHigh()+role->AttackPowerHighBonus();
	pkInfo.attackPowerLow = role->AttackPowerLow()+role->AttackPowerLowBonus();
	pkInfo.attackArea = role->AttackScope();
	pkInfo.attackSpeed = role->AttackSpeed()+role->AttackSpeedBonus();
	pkInfo.attackDisplayTime = 2;
	pkInfo.attackBulletSpeed = role->BulletSpeed();
	pkInfo.hitRate = role->HitRate()+role->HitRateBonus();
	pkInfo.dodgeRate = role->DodgeRate()+role->DodgeRateBonus();
	pkInfo.defense = role->Defence()+role->DefenceBonus();
	pkInfo.mDefense = role->MDefence()+role->MDefenceBonus();
	pkInfo.CritRate=role->CritRate()+role->CritRateBonus();
	pkInfo.Agile=role->Agility()+role->AgilityBonus();
	pkInfo.Strength=role->Strength()+role->StrengthBonus();
	pkInfo.Wisdom=role->Intelligence()+role->IntelligenceBonus();
//	LOG(LOG_ERROR,__FILE__,__LINE__,"PKRoleID:::: %d contronID :::%d  ---------" , pkInfo.roleID,pkInfo.controlID);
	//pkInfo.

/*	lrid.push_back(	pkInfo.roleID );
	iRet = _mainSvc->GetAvatarSvc()->GetEquipBrief( lrid, ladb);
	if(iRet)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"GetEquipBrief error" );
		return -1;
	}
	*/
//	pkInfo.wpnItemID = ladb.front().wpnItemID;
//	pkInfo.flag = ladb.front().coatID;

	//技能
	pkInfo.las.clear();

		
	sprintf(szSql,"select SkillID,SkillLev from RoleSkill where RoleID=%d and SkillID<300 and SkillLev>0",role->ID());
	iRet=dbo.QuerySQL(szSql);
	if(iRet!=0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL role not have skill[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
	}

	while(dbo.HasRowData())
	{
			skill.skillID=dbo.GetIntField(0);
			skill.skillLevel=dbo.GetIntField(1);
			pkInfo.las.push_back(skill);
		
		dbo.NextRow();
	}
	
		
	
	lrpki.push_back(pkInfo);

//	MakeSSPkgForPet(const ArchvPosition & posPKOrigin,UInt32 roleID,List < ArchvRolePKInfo > & lrpki)
	rolePosition.X=pkInfo.currPosX;
	rolePosition.Y=pkInfo.currPosY;
	MakeSSPkgForPet(posPKOrigin,role->ID(),lrpki,type,pkInfo.direct);
	

	return 0;
}


//@brief 组建 S_S pk请求包:  for 怪物
//@lrpki		返回的组建结果
//@return 0 成功  非0 失败
int PKSvc::MakeSSPkgForMonster( 	const ArchvPosition &posPKOrigin, Monster &monster, List<ArchvRolePKInfo> &lrpki,Byte num)
{
	ArchvRolePKInfo pkInfo;
	ArchvAvatarDescBrief adb;
	//List<ArchvAvatarDescBrief> ladb;
	List<UInt32> lrid;
	ArchvSkill skill;
	int iRet = 0;

	//赋值
	pkInfo.controlID = monster.ID();
	pkInfo.roleID = monster.ID();
	pkInfo.level = monster.Level();//没有
	pkInfo.opposition = 4;
	pkInfo.creatureFlag = 2;
	pkInfo.creatureType = monster.Type();

	pkInfo.maxHP = monster.MaxHp();
	pkInfo.maxMP = monster.MaxMp();
	pkInfo.hp = monster.Hp();
	pkInfo.mp = monster.Mp();
	pkInfo.moveSpeed = monster.MoveSpeed();
	


//pk坐标处理
//	对超出pk地图范围的坐标进行调整
		if(num%2==0)
		pkInfo.currPosY = posPKOrigin.Y+1;
		else
		pkInfo.currPosY = posPKOrigin.Y;
  	pkInfo.currPosX = posPKOrigin.X;

	
LOG(LOG_DEBUG,__FILE__,__LINE__,"----------->>>>roleID [%d], currPosX x[%d],y[%d]  ",
	monster.ID(), pkInfo.currPosX, pkInfo.currPosY );	
	
	pkInfo.direct = monster.Direct();
	pkInfo.attackPowerHigh = monster.AttackPowerHigh();
	pkInfo.attackPowerLow = monster.AttackPowerLow();
	pkInfo.attackArea = monster.AttackScope();
	pkInfo.attackSpeed = monster.AttackSpeed();
	pkInfo.attackDisplayTime = 2;
	pkInfo.attackBulletSpeed = monster.BulletSpeed();
	pkInfo.hitRate = monster.HitRate();
	pkInfo.dodgeRate = monster.DodgeRate();
	pkInfo.defense = monster.Defence();
	pkInfo.CritRate=monster.CritRate();
	pkInfo.mDefense = monster.MDefence();
//	LOG(LOG_ERROR,__FILE__,__LINE__,"PKRoleID:::: %d contronID :::%d  ---------" , pkInfo.roleID,pkInfo.controlID);

	lrpki.push_back(pkInfo);
	if(num>1)
	{
		int pp=-1;
		for(int i=1;i<num;i++)
		{
			pp=-1*pp;
			pkInfo.controlID=monster.ID()+i;
			pkInfo.roleID=monster.ID()+i;
	  	pkInfo.currPosX = posPKOrigin.X;
//	  		LOG(LOG_ERROR,__FILE__,__LINE__,"PKRoleID:::: %d contronID :::%d  ---------" , pkInfo.roleID,pkInfo.controlID);
				pkInfo.currPosY = ((pkInfo.currPosY)+2*pp*i);

			lrpki.push_back(pkInfo);
		}
	}


	return 0;
}

int PKSvc::MakeSSPkgForPet(const ArchvPosition &posPKOrigin, UInt32 roleID, List<ArchvRolePKInfo> &lrpki,Byte type,Byte dirte)
{
		char szSql[1024];
		Connection con;
		DBOperate dbo;
		int iRet = 0;
		UInt32 PetID=0;
		ArchvRolePKInfo petpkInfo;
		//获取DB连接
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
		
		sprintf( szSql, "select	 PetID , PetType,PetKind  ,PetName   ,Level      ,Exp ,MaxExp,   \
							   AddPoint   ,Strength \
							   ,Intelligence   ,Agility        ,MoveSpeed      ,HP             ,MP          \
							   ,MaxHP          ,MaxMP          ,HPRegen        ,MPRegen        ,AttackPowerHigh \
							   ,AttackPowerLow    ,AttackScope       ,AttackSpeed       ,BulletSpeed       ,Defence        \
							   ,MDefence          ,CritRate          ,HitRate           ,DodgeRate             \
							from Pet \
							where RoleID=%d and IsUse=1;", roleID );
	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
//		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
		return -1;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return -1;
 	}
	petpkInfo.controlID=roleID;
	petpkInfo.creatureFlag=4;
	petpkInfo.opposition=type;
	if(posPKOrigin.X-PKSCREEN_XLENGTH/2>0)
	{
		petpkInfo.currPosX=posPKOrigin.X+2;
	}
	else
	{
		petpkInfo.currPosX=posPKOrigin.X-2;
	}
	petpkInfo.currPosY=posPKOrigin.Y;
	while(dbo.HasRowData())
	{
			PetID = dbo.GetIntField(0);
			petpkInfo.roleID=PetID;
		 petpkInfo.creatureType=dbo.GetIntField(1);
		//_petkind=dbo.GetIntField(2) ;力量型什么的
		// Name( dbo.GetStringField(3) );
		petpkInfo.level=dbo.GetIntField(4);
		//Exp( dbo.GetIntField(5) );
		//MaxExp(dbo.GetIntField(6));
		//AddPoint( dbo.GetIntField(7) );
		petpkInfo.Strength=dbo.GetIntField(8);
		petpkInfo.Wisdom=dbo.GetIntField(9);
		petpkInfo.Agile=dbo.GetIntField(10);
		petpkInfo.moveSpeed=dbo.GetIntField(11);
		petpkInfo.hp=dbo.GetIntField(12);
		petpkInfo.mp=dbo.GetIntField(13);
		petpkInfo.maxHP=dbo.GetIntField(14);
		petpkInfo.maxMP=dbo.GetIntField(15);
		// HpRegen( dbo.GetIntField(16) );
		//MpRegen( dbo.GetIntField(17) );
		petpkInfo.attackPowerHigh=dbo.GetIntField(19);
		petpkInfo.attackPowerLow=dbo.GetIntField(20);
		petpkInfo.attackArea=dbo.GetIntField(21);
		petpkInfo.attackBulletSpeed=dbo.GetIntField(22);
		petpkInfo.attackSpeed=dbo.GetIntField(23);
		petpkInfo.defense=dbo.GetIntField(24);
		petpkInfo.mDefense=dbo.GetIntField(25);
		petpkInfo.CritRate=dbo.GetIntField(26);
		petpkInfo.hitRate=dbo.GetIntField(27);
		petpkInfo.dodgeRate=dbo.GetIntField(28);
		
		dbo.NextRow();
	}
	sprintf( szSql, "select	 PetID     ,Strength   ,Intelligence   ,Agility      ,MovSpeed ,MaxHP,MaxMP     \
						   ,HPRegen      ,MPRegen    ,AttackPowerHigh      ,AttackPowerLow   ,AttackSpeed \
						   ,Defence   ,MDefence        ,CritRate      ,HitRate             ,DodgeRae          \
						from PetBonus \
						where PetID = %d ", PetID );
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
		petpkInfo.Strength =petpkInfo.Strength+dbo.GetIntField(1);
		petpkInfo.Wisdom=petpkInfo.Wisdom+dbo.GetIntField(2);
		petpkInfo.Agile=petpkInfo.Agile+dbo.GetIntField(3);
		petpkInfo.moveSpeed=petpkInfo.moveSpeed+dbo.GetIntField(4);
		petpkInfo.maxHP=petpkInfo.maxHP+dbo.GetIntField(5);
		petpkInfo.maxMP=petpkInfo.maxMP+dbo.GetIntField(6);
	//	HpRegenBonus(dbo.GetIntField(7));
	//	MpRegenBonus(dbo.GetIntField(8));
		petpkInfo.attackPowerHigh=petpkInfo.attackPowerHigh+dbo.GetIntField(9);
		petpkInfo.attackPowerLow=petpkInfo.attackPowerLow+dbo.GetIntField(10);
		petpkInfo.attackSpeed=petpkInfo.attackSpeed+dbo.GetIntField(11);
		petpkInfo.defense=petpkInfo.defense+dbo.GetIntField(12);
		petpkInfo.mDefense=petpkInfo.mDefense+dbo.GetIntField(13);
		petpkInfo.CritRate=petpkInfo.CritRate+dbo.GetIntField(14);
		petpkInfo.hitRate=petpkInfo.hitRate+dbo.GetIntField(15);
		petpkInfo.dodgeRate=petpkInfo.dodgeRate+dbo.GetIntField(16);
		
		dbo.NextRow();
	}
		petpkInfo.direct=dirte;
	 	LOG(LOG_ERROR,__FILE__,__LINE__,"PKRoleID:::: %d contronID :::%d  ---------" , petpkInfo.roleID,petpkInfo.controlID);
	lrpki.push_back(petpkInfo);
	return 0;
}
void PKSvc::ProcessPKReq(Session& session,Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	int iRet = 0;
	PKBriefList pkList;
	
	UInt32 roleID = packet.RoleID;
	Byte creatureFlag;
	UInt32 creatureID;

	ArchvRolePKInfo pkInfo;
	List<ArchvRolePKInfo> lrpki;

	UInt32 pkID = 0;
	ArchvCreatureStatus cs;
	List<ArchvCreatureStatus> lcs;
	list<TeamRole> roleIDitor; 
	list<TeamRole>::iterator itor;
	list<UInt32> roleID1,roleID2;
	list<UInt32>::iterator it;
	Team team;
	ArchvPosition posMid;
	ArchvPosition posPKOrigin;
	int iTmp = 0;
	list<SenceMonster> monsterrs; 
	list<SenceMonster>::iterator itor1;
	Byte Status=0;
	RolePtr srcRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	RolePtr desRole;
	Monster desMonster;
	ArchvPosition pos1;
	ArchvPosition pos2;
	int pp=-1;
	int j=1;
 
	//序列化类
	Serializer s(packet.GetBuffer());
	s>>creatureFlag>>creatureID;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
/*	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	*/

LOG(LOG_ERROR,__FILE__,__LINE__,"creatureFlag[%d],creatureID[%d]  ", creatureFlag, creatureID );
	
	//生物标志校验
	if( creatureFlag <=0 || creatureFlag > 4 )
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"creatureFlag error!! creatureFlag[%d] ", creatureFlag );
		goto EndOf_Process;
	}
	//会发生角色无法PK得状态,可以使用定时放开PK限制
	pkID=_mainSvc->GetCoreData()->IfRoleInPK(roleID);


	if(pkID!=0)
	{
		LOG(LOG_DEBUG,__FILE__,__LINE__,"111111111111 pkID=%d  roleID=%d", pkID, roleID  );
		
		RetCode = 2011;//角色的上场战斗没有结束
		LOG(LOG_ERROR,__FILE__,__LINE__,"role status error!! role is in the pk  RoleId=%d", roleID  );
		goto EndOf_Process;
	}



	if( 0 == srcRole->ID() )
	{
		RetCode = ERR_APP_ROLENOTEXISTS;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error,roleID[%d]", roleID  );
		goto EndOf_Process;
	}

	
		if(srcRole->TeamFlag()==0)
		{
			//单独作战，跳过
		}
		else
		{

				if(srcRole->TeamFlag()!=2)
				{
					//失败，不是队长
					RetCode = ERR_APP_ROLENOTEXISTS;
					LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error,roleID[%d]", roleID  );
					goto EndOf_Process;
				}
				team=_mainSvc->GetCoreData()->GetTeams(srcRole->TeamID());
				roleIDitor=team.GetMemberRoleID();
				if(roleIDitor.size()>1)
				{
					//队伍的人数
						for(itor=roleIDitor.begin();itor!=roleIDitor.end();itor++)
						{
							//队伍成员
								if(itor->status==1)
								{
									//队员处于归队状态				
										roleID1.push_back(itor->roleId);
								
								}
								
						}//end for

							pkList.player1=roleID1;//队员，不包括自己
				
			 }//end if
		}//end if else

	pos1 = srcRole->Pos();

	//获取被攻击者信息
	if( 1 == creatureFlag )
	{//被攻击者为角色
	
		//获取被攻击角色的信息
		desRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(creatureID);
		if( 0 == desRole->ID() )
		{
			RetCode = ERR_APP_ROLENOTEXISTS;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error,creatureID[%d]", creatureID  );
			goto EndOf_Process;
		}

		//判断是否同一地图
		if( srcRole->MapID() != desRole->MapID() )
		{
			RetCode = ERR_APP_DATA;
			LOG(LOG_ERROR,__FILE__,__LINE__,"error MapID,srcRole->MapID()[%d],desRole->MapID()[%d] ", srcRole->MapID(), desRole->MapID()  );
			goto EndOf_Process;
		}
		if(desRole->TeamFlag()==0||desRole->TeamFlag()==1)
		{
			//单独作战，跳过
		}
		else
		{
				roleIDitor.clear();
				team=_mainSvc->GetCoreData()->GetTeams(desRole->TeamID());
				roleIDitor=team.GetMemberRoleID();
				if(roleIDitor.size()>1)
				{
					//队伍的人数
					for(itor=roleIDitor.begin();itor!=roleIDitor.end();itor++)
					{
						//队伍成员
							if(itor->status==1||itor->status==2)
							{
								//队员处于归队状态
								if(itor->roleId!=creatureID)
								{
									roleID2.push_back(itor->roleId);
									
								}
								
							}
							
					}
						pkList.player2=roleID2;//队员，不包括自己
				
			 }
		}
		pos2 = desRole->Pos();

	}
	else if( 2 == creatureFlag )
	{//被攻击者为怪

		//获取怪信息
		pos2=_mainSvc->GetCoreData()->GetSenceMonster(srcRole->MapID(),creatureID,monsterrs);
		if(monsterrs.size()==0)
		{
			RetCode = ERR_APP_ROLENOTEXISTS;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetMonster error,creatureID[%d]", creatureID  );
			goto EndOf_Process;
		}
		desMonster = _mainSvc->GetCoreData()->ProcessGetMonster( srcRole->MapID(), creatureID );
		if( 0 == desMonster.ID() )
		{
			RetCode = ERR_APP_ROLENOTEXISTS;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetMonster error,creatureID[%d]", creatureID  );
			goto EndOf_Process;
		}
		desMonster.ID(creatureID);
		
	}

	//获取pk 双方间直线中点坐标
  posMid = GetMidpoint( pos1, pos2 );


	//计算相对坐标原点位置
	//posPKOrigin = CalcPKOrigin( posMid );
	
LOG(LOG_DEBUG,__FILE__,__LINE__,"----------->>>>posMid x[%d],y[%d], srcRole->currPos x[%d],y[%d], desRole->currPos x[%d],y[%d], posPKOrigin x[%d],y[%d]  ",
	posMid.X, posMid.Y, pos1.X, pos1.Y, pos2.X, pos2.Y, posPKOrigin.X, posPKOrigin.Y );

	//组建 S_S pk 请求包: pk发起者
	if(pos1.X-posMid.X>0)//在右边,暂时没有获取对方的位置
	{
		pos1.X=PKSCREEN_XLENGTH/2-10;
		pos1.Y=PKSCREEN_YLENGTH/2;
	}
	else
	{
		pos1.X=PKSCREEN_XLENGTH/2+10;
		pos1.Y=PKSCREEN_YLENGTH/2;
	}
	if(roleID1.size()%2==0)
	{
		pos1.Y=pos1.Y-1;
	}
		iRet = MakeSSPkgForRole( pos1, srcRole, lrpki,1);
	
	if(iRet)
	{
		RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"MakeSSPkgForRole error!! srcRole->ID()[%d] ", srcRole->ID() );
		goto EndOf_Process;
	}
	//队伍队员

	for(it=roleID1.begin();it!=roleID1.end();it++)
	{
		pp=-1*pp;
		srcRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(*it);
		if( 0 == srcRole->ID() )
		{
			RetCode = ERR_APP_ROLENOTEXISTS;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error,creatureID[%d]", creatureID  );
			goto EndOf_Process;
		}
		pos1.Y=pos1.Y+2*j*pp;
		j++;
	//	posPKOrigin.X=
		iRet = MakeSSPkgForRole( pos1, srcRole, lrpki,1);
		if(iRet)
		{
			RetCode = ERR_APP_OP;
			LOG(LOG_ERROR,__FILE__,__LINE__,"MakeSSPkgForRole error!! srcRole->ID()[%d] ", srcRole->ID() );
			goto EndOf_Process;
		}
	}

	//组建 S_S pk 请求包: 被攻击者
	if( 1 == creatureFlag )
	{
			if(pos2.X-posMid.X>0)//在右边,暂时没有获取对方的位置
			{
				pos2.X=PKSCREEN_XLENGTH/2-10;
				pos2.Y=PKSCREEN_YLENGTH/2;
			}
			else
			{
				pos2.X=PKSCREEN_XLENGTH/2+10;
				pos2.Y=PKSCREEN_YLENGTH/2;
			}
		iRet = MakeSSPkgForRole(  pos2, desRole, lrpki,2); //被攻击者为角色
			 pp=-1;
			 j=1;
		for(it=roleID2.begin();it!=roleID2.end();it++)
		{
				pp=-1*pp;
				desRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(*it);
				if( 0 == desRole->ID() )
				{
					RetCode = ERR_APP_ROLENOTEXISTS;
					LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error,creatureID[%d]", creatureID  );
					goto EndOf_Process;
				}
			//	posPKOrigin.X=
				pos2.Y=pos2.Y+2*j*pp;
				j++;
		
				iRet = MakeSSPkgForRole( pos1, desRole, lrpki,2);
				if(iRet)
				{
					RetCode = ERR_APP_OP;
					LOG(LOG_ERROR,__FILE__,__LINE__,"MakeSSPkgForRole error!! srcRole->ID()[%d] ", srcRole->ID() );
					goto EndOf_Process;
				}
		}
		
	}
	else if( 2 == creatureFlag )
	{
		pp=1;
		if(pos2.X-posMid.X>0)//在右边,暂时没有获取对方的位置
			{
				pos2.X=PKSCREEN_XLENGTH/2-5;
				pos2.Y=PKSCREEN_YLENGTH/2;
				pp=-1;
			}
			else
			{
				pos2.X=PKSCREEN_XLENGTH/2+5;
				pos2.Y=PKSCREEN_YLENGTH/2;
				pp=-1;
			}
    iRet = MakeSSPkgForMonster(  pos2, desMonster, lrpki,1); //怪
		UInt32 i=1;
			 j=1;
		if(monsterrs.size()>1)
		{		
				itor1=monsterrs.begin();
				itor1++;//跳过第一条数据
				for(;itor1!=monsterrs.end();itor1++)
				{		
					pos2.X=pos2.X-j*pp*2;
					 desMonster=_mainSvc->GetCoreData()->ProcessGetMonster( desMonster.MapID(), itor1->MonsterType);
						desMonster.ID(creatureID+i);
						iRet = MakeSSPkgForMonster(  pos2, desMonster, lrpki,itor1->num); //怪
						i=itor1->num+i;
				}
	  }
	}
	if(iRet)
	{
		RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"MakeSSPkgForRole error!! desRole->ID()[%d] ", desRole->ID() );
		goto EndOf_Process;
	}


	
	//pk 拉人功能，根据组队、坐标范围来拉人进入pk

	pkID=roleID;

	pkList.player1.push_back(roleID);//队员，不包括自己
	pkList.player2.push_back(creatureID);//队员，不包括自己
	pkList.pkID=pkID;
	_mainSvc->GetCoreData()->AddPKBrif(pkID,pkList);

	//ss 消息处理
	
	Status=_mainSvc->GetCoreData()->ProcessGetRolePtr(roleID)->Status();
	if(Status!=1)
	{
		RetCode = ERR_APP_ROLENOTEXISTS;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessGetRole error,roleID[%d]", roleID  );
		goto EndOf_Process;
	}
	else
	{
		srcRole->Status(2);
	}
	iRet = _mainSvc->GetSSClientManager()->ProcessPkReq( srcRole->MapID(), posMid.X, posMid.Y, lrpki, pkID);
	if(iRet)
	{
		RetCode = iRet;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ProcessPkReq error ,RetCode[%d]", RetCode );		
		goto EndOf_Process;
	}

	
EndOf_Process:

	//状态的重置
	if(RetCode)
	{
		srcRole->Status(1);		//重置为非pk状态
		_mainSvc->GetCoreData()->EraseRolePKID(roleID);				//删除 PKID

		LOG(LOG_DEBUG,__FILE__,__LINE__,"222222222 _mainSvc->GetCoreData()->IfRoleInPK(roleID) [%d]", _mainSvc->GetCoreData()->IfRoleInPK(roleID) );
	}

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
//		s<<11;
	}	

	p.UpdatePacketLength();
	
	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}
	
DEBUG_PRINTF1( "C_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );


	//--------------------------------------------------------------------------
	//成功处理，则修改生物状态
	if( 0 == RetCode )
	{
		GetCtStatus( lrpki, lcs );
		_mainSvc->GetCoreData()->ChangeCreatureStatus( srcRole->MapID(), lcs);
	}
		
	
}



//@brief [方法 ID:0401]pk结束；分别单播给pk参与者
void PKSvc::NotifyPKEnd( list<PKEndInfo>& lpkei )
{
	List<UInt32>lrid;
	list<PKEndInfo>::iterator it;

	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );	
	Packet p(&serbuffer);


	//分别单播,给列表中的角色
	for( it = lpkei.begin(); it != lpkei.end(); it++ )
	{
			LOG(LOG_DEBUG,__FILE__,__LINE__,"pkType[%d],glory[%d],exp[%d],money[%d]",
					it->pkr.pkType, it->pkr.glory, it->pkr.exp, it->pkr.money);

		serbuffer.Reset();

		p.Direction = DIRECT_S_C_REQ;
		p.SvrType = 1;
		p.SvrSeq = 1;
		p.MsgType = 401;
		p.UniqID = 123;

		p.PackHeader();

		s<<(it->pkr)<<(it->pkei);

		p.UpdatePacketLength();
		
		LOG(LOG_ERROR,__FILE__,__LINE__,"PK_END s-c RoleID=%d",it->roleID );
		
		lrid.clear();
		lrid.push_back(it->roleID);
		if( _mainSvc->Service()->Broadcast( lrid, &serbuffer))
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
		}

DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size()[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
	}

}

