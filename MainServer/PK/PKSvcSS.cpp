#include "PKSvcSS.h"
#include "DBOperate.h"
#include "DebugData.h"
#include "Packet.h"
#include "ArchvRole.h"
#include "MainSvc.h"
#include "ArchvPK.h"
#include "ArchvMap.h"
#include "CoreData.h"
#include "PKSvc.h"
#include "ArchvBagItemCell.h"
#include "Role.h"
#include  "Monster.h"
#include "./RoleInfo/RoleInfoSvc.h"
#include "../Task/TaskSvc.h"
#include "../Bag/BagSvc.h"
#include "../Pet/PetSvc.h"
#include <cmath>
#include <algorithm>
#include "./Avatar/AvatarSvc.h"


PKSvcSS::PKSvcSS(MainSvc * mainSvc, ConnectionPool *cp )
:_mainSvc(mainSvc)
,_cp(cp)
{
}

PKSvcSS::~PKSvcSS()
{
}

void PKSvcSS::OnProcessPacket(Session& session,Packet& packet)
{

DEBUG_PRINTF1( "S_S req pkg->->->->->->MsgType[%d]\n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
	 	case 201: // pk结束
			ProcessPKEnd(session,packet);
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
void PKSvcSS::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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

DEBUG_PRINTF1( "S_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

}


//@brief	杀怪任务事件处理
void PKSvcSS::DealKillMonsterTask( list<UInt32>& roleID,UInt32 creaturetype,UInt32 num )
{

//DEBUG_PRINTF2( "DealKillMonsterTask ----- lpkei.size[%d], lcs.size[%d]  \n", lpkei.size(), lcs.size() );

	list<UInt32>::iterator it;
	for( it = roleID.begin(); it != roleID.end(); it++ )
	{
		_mainSvc->GetTaskSvc()->OnAfterKillMonster( *it, creaturetype, num);

		//物品掉落测试,杀怪任务物品获得
		//_mainSvc->GetBagSvc()->GetItem( it2->roleID, 10, lic);
		//	_mainSvc->GetBagSvc()->RoleGetItem(it2->roleID,lic);
	}
}


//@brief	从pk结束包获取信息
//	获取生物状态列表
//	获取pk奖励
/*
//@return 空
void PKSvcSS::GetInfoFromPkEnd( List<ArchvPKEndInfo>& lei, List<ArchvCreatureStatus>& lcs, list<PKEndInfo>& lpkei ,UInt32 mapID,Byte WinOpposition)
{
	List<ArchvPKEndInfo>::iterator it;
	bool hasRole1 = false;		//阵营1 有角色
	bool hasRole2 = false;		//阵营2 有角色
	Byte pkType = 0;
	UInt32 i;
	Monster monster;
	ArchvCreatureStatus cs;
	PKEndInfo pkei;

	for( it = lei.begin(); it != lei.end(); it++ )
	{
			//阵营判断，没有考虑 3：中立；4：全敌对关系
			if( 1 == it->creatureFlag )//是角色
			{

				//lev=_mainSvc->GetCoreData()->ProcessGetRole(it->roleID).Level();
			//	levs.push_back(lev);
				if( 1 == it->opposition )
					hasRole1 = true;

				if( 2 == it->opposition )
					hasRole2 = true;
			}
			if(it->creatureFlag==2)//多个怪物，组队PK没有考虑,1vs1
			{
				monster = _mainSvc->GetCoreData()->ProcessGetMonster(mapID,it->roleID);
			}
	}
	//pkType
	if( hasRole1 && hasRole2 )
		pkei.pkr.pkType = pkType = 2;			//PVP
	else
		pkei.pkr.pkType = pkType = 1;			//PVE

	//生物状态的获取
	lcs.clear();
	for( it = lei.begin(); it != lei.end(); it++ )
	{
		//生物状态
		cs.creatureFlag = it->creatureFlag;
		cs.creatureType = it->creatureType;
		cs.ID = it->roleID;
		LOG(LOG_ERROR,__FILE__,__LINE__,"the creaturestatue ::roleID=%d {}status =%d",it->roleID,it->live);
		if( 1 == it->live )
			cs.status = 1;					//活着,未战斗
		else if( 2 == it->live )
			cs.status = 0;					//死

		lcs.push_back(cs);

		//角色信息
		if( 1 == it->creatureFlag )
		{
			if(pkType==1)//PVE
			{
//				pkei.pkr.exp=GettheRandNum(monster.MinExp,monster.MaxExp);//经验
	//			pkei.pkr.money=GettheRandNum(monster.MinMoney,monster.MaxMoney);//钱
				pkei.pkr.glory=0;//荣誉
				pkei.pkr.items =" 万能药水*20;贵族指环*1 ";//物品的获得不知道怎么做
			}
			else
			{//pvp只能获得荣誉值
				pkei.pkr.glory=0;//荣誉
			}
 			//获取角色pk结束信息
			pkei.roleID = it->roleID;
			pkei.pkei = *it;
			//数据存入
			i=_mainSvc->GetCoreData()->RoleExpAdd(it->roleID,pkei.pkr.exp);//返回是否升级
			if(i==1)
			{
			//有升级，发送升级广播
				_mainSvc->GetRoleInfoSvc()->Notifyinfo(it->roleID);
			}
			_mainSvc->GetBagSvc()->Getmoney(it->roleID,pkei.pkr.money);
			_mainSvc->GetCoreData()->RolePKset(it->roleID,it->hp,it->mp);
			_mainSvc->GetCoreData()->RoleAddGlory(it->roleID,pkei.pkr.glory);//荣誉值待优化
			//还差物品
			lpkei.push_back(pkei);
		}

	}


}

*/
void PKSvcSS::GetInfoFromPkEnd2( List<ArchvPKEndInfo>& lei, List<ArchvCreatureStatus>& lcs, list<PKEndInfo>& lpkei ,UInt32 mapID,UInt32 pkID)
{


		List<ArchvPKEndInfo>::iterator it;
		Byte flag=0;//默认是pve
		Byte pkType = 0;
		UInt32 i;
		Monster monster;
		ArchvCreatureStatus cs;
		PKEndInfo pkei;
		list<UInt32> winers;//赢了一方的ID
		list<UInt32> failer;
		list<UInt32>::iterator itor;
		list<RoleEquipToPk> roleEquiplist;
		list<PetInfoToPk> PetInfolist;
		ArchvCreatureStatus lscinfo;
		RoleEquipToPk roleEquip;
		PetInfoToPk PetInfo;
		PKBriefList pklist;
		//
		for(it=lei.begin();it!=lei.end();it++)//分开角色
		{
			if(it->opposition!=1)//这是winer
			{
				//人物
					//
					if(it->creatureFlag==1)//角色
					{
						roleEquip.RoleID=it->roleID;
						lscinfo.creatureFlag=it->creatureFlag;
						lscinfo.creatureType=it->creatureType;
						lscinfo.ID=it->roleID;
						lscinfo.status=1;
						lcs.push_back(lscinfo);

						if(it->live==2)
						{
							roleEquip.type=0;//死亡的
						}
						else
						{
							roleEquip.type=1;
						}
						roleEquiplist.push_back(roleEquip);//构建所有赢了的角色的武器的耐久点数减少

						RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(it->roleID);

						pRole->RolePKset(it->hp,it->mp);


						winers.push_back(it->roleID);
					}
					else if(it->creatureFlag==4)//宠物
					{
						PetInfo.Hp=it->hp;
						PetInfo.Mp=it->mp;
						PetInfo.PetID=it->roleID;
						SetPetHPMPFullAfterPk(it->roleID);

					}
				//获得奖励、

			}
			else
			{
				//人物
				flag=1;//pvp,输了的人
					if(it->creatureFlag==1)//角色
					{
						roleEquip.RoleID=it->roleID;
						roleEquip.type=2;//没有死亡的
						roleEquiplist.push_back(roleEquip);



						roleEquip.RoleID=it->roleID;
						lscinfo.creatureFlag=it->creatureFlag;
						lscinfo.creatureType=it->creatureType;
						lscinfo.ID=it->roleID;
						lscinfo.status=1;
						lcs.push_back(lscinfo);
						failer.push_back(it->roleID);

					}
					else if(it->creatureFlag==4)//宠物
					{
						PetInfo.Hp=it->hp;
						PetInfo.Mp=it->mp;
						PetInfo.PetID=it->roleID;
					}


			}//end else

		}//end for



			PKEndInfo lrpk;
			List<ArchvPKEndInfo>::iterator itor4;
			if(winers.size()==0)
			{
				//PVE，有奖励，查看战斗

				pklist=_mainSvc->GetCoreData()->GetPkInfo(pkID);
				if(pkID!=pklist.pkID)
				{
					LOG(LOG_ERROR,__FILE__,__LINE__,"PKID erro! pkID is %d !!!!",pkID);
					//
				}
				//
				list<SenceMonster> monsters;

				for(itor=pklist.player2.begin();itor!=pklist.player2.end();itor++)
				{
					_mainSvc->GetCoreData()->GetSenceMonster(mapID,*itor,monsters);
					lscinfo.creatureFlag=2;
					lscinfo.creatureType=(*itor%100000000)/10000;
					lscinfo.ID=*itor;
					lscinfo.status=1;//怪没死
					lcs.push_back(lscinfo);
				}
					for( itor4=lei.begin();itor4!=lei.end();itor4++)
					{
						if(itor4->opposition!=1&&itor4->creatureFlag==1)
						{
							lrpk.roleID=itor4->roleID;
							lrpk.pkei=*itor4;
							lrpk.pkei.live=2;//依然活着
							lrpk.pkr.exp=0;
							lrpk.pkr.money=0;
							lrpk.pkr.pkType=1;//PVP

							lpkei.push_back(lrpk);
							break;
						}
						else
						{
							lrpk.roleID=itor4->roleID;
							lrpk.pkei=*itor4;
							lrpk.pkei.live=1;//依然活着

							lrpk.pkr.exp=0;
							lrpk.pkr.money=0;
							lrpk.pkr.pkType=2;//PVP

							lpkei.push_back(lrpk);
						}
				}
		}


}


void PKSvcSS::GetInfoFromPkEnd1( List<ArchvPKEndInfo>& lei,List<ArchvCreatureStatus>& lcs,list<PKEndInfo>& lpkei,UInt32 mapID,UInt32 PkID)
{
	List<ArchvPKEndInfo>::iterator it;
	Byte flag=0;//默认是pve
	Byte pkType = 0;
	UInt32 i;
	Monster monster;
	ArchvCreatureStatus cs;
	PKEndInfo pkei;
	list<UInt32> winers;//赢了一方的ID
	list<UInt32> failer;
	list<UInt32>::iterator itor;
	list<RoleEquipToPk>	roleEquiplist;
	list<PetInfoToPk> PetInfolist;
	ArchvCreatureStatus lscinfo;
	RoleEquipToPk roleEquip;
	PetInfoToPk PetInfo;
	PKBriefList pklist;
	list<UInt32> winPets;
	//
	for(it=lei.begin();it!=lei.end();it++)//分开角色
	{
//		LOG(LOG_ERROR,__FILE__,__LINE__,"PKID OK!! creature ID is [%d]!",it->roleID);
		if(it->opposition==1)//这是阵营1
		{
			//人物
				//
				if(it->creatureFlag==1)//角色
				{
					roleEquip.RoleID=it->roleID;
					lscinfo.creatureFlag=it->creatureFlag;
					lscinfo.creatureType=it->creatureType;
					lscinfo.ID=it->roleID;
					lscinfo.status=1;
					lcs.push_back(lscinfo);

					if(it->live==2)
					{
						roleEquip.type=0;//死亡的
					}
					else
					{
						roleEquip.type=1;
					}
					roleEquiplist.push_back(roleEquip);//构建所有赢了的角色的武器的耐久点数减少


					RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(it->roleID);

					pRole->RolePKset(it->hp,it->mp);


					winers.push_back(it->roleID);
				}
				else if(it->creatureFlag==4)//宠物
				{
					PetInfo.Hp=it->hp;
					PetInfo.Mp=it->mp;
					PetInfo.PetID=it->roleID;
					//SetPetHPMPFullAfterPk(it->roleID);

					winPets.push_back(it->roleID);
				}
			//获得奖励、

		}
		else
		{
			//人物
			flag=1;//pvp,输了的人
				if(it->creatureFlag==1)//角色
				{
					roleEquip.RoleID=it->roleID;
					roleEquip.type=2;//没有死亡的
					roleEquiplist.push_back(roleEquip);



					roleEquip.RoleID=it->roleID;
					lscinfo.creatureFlag=it->creatureFlag;
					lscinfo.creatureType=it->creatureType;
					lscinfo.ID=it->roleID;
					lscinfo.status=1;
					lcs.push_back(lscinfo);
					failer.push_back(it->roleID);

				}
				else if(it->creatureFlag==4)//宠物
				{
					PetInfo.Hp=it->hp;
					PetInfo.Mp=it->mp;
					PetInfo.PetID=it->roleID;

					SetPetHPMPFullAfterPk(it->roleID);
				}


		}//end else

	}//end for

		if(failer.size()==0)
		{
			//PVE，有奖励，查看战斗
 			pklist=_mainSvc->GetCoreData()->GetPkInfo(PkID);
			if(PkID!=pklist.pkID)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"PKID erro! pkID is %d !!!!",PkID);
				//
			}
			//
			list<SenceMonster> monsters;

			for(itor=pklist.player2.begin();itor!=pklist.player2.end();itor++)
			{
				_mainSvc->GetCoreData()->GetSenceMonster(mapID,*itor,monsters);
				lscinfo.creatureFlag=2;
				lscinfo.creatureType=(*itor%100000000)/10000;
				lscinfo.ID=*itor;
				lscinfo.status=0;//怪死了
				lcs.push_back(lscinfo);
			}

			GetItemFromPkEnd(winers,lei,monsters,lpkei, mapID);//物品奖励

			WinPetAddExp(winPets, lpkei);
		}
		//去掉这次战斗的存储



}//end

UInt32 PKSvcSS::findtheLev(list<RoleInfopk>& lis,UInt32 roleID)
{
	list<RoleInfopk>::iterator it;
	for(it=lis.begin();it!=lis.end();it++)
	{
		if(it->RoleID==roleID)
		return it->Level;
	}
	return 0;
}



void PKSvcSS::AddExpByExpRune(List<ArchvPKEndInfo>::iterator & itor, PKEndInfo & lrpk)
{
	//每一次是使用符文 2小时生效，6002是经验符文ID

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(itor->roleID);
    if(time(NULL) - pRole->GetRoleRuneTime(6002) < 2 * 3600){
        lrpk.pkr.exp = UInt32(lrpk.pkr.exp*1.5); //目前增加50%
    }
}

void PKSvcSS::GetItemFromPkEnd(list<UInt32>& roleIDs,List<ArchvPKEndInfo>& lei,list<SenceMonster>& monsters,list<PKEndInfo>& lpkei,UInt32 mapID)
{
	UInt32 Totallelev=0;
	UInt32 monsterlev=0;
	UInt32 Lev=0;
	list<SenceMonster>::iterator itor1;

	list<UInt32>::iterator itor;
	list<RoleInfopk>::iterator itor2;
	List<ArchvPKEndInfo>::iterator itor4;
	list<RoleInfopk> roleLev;
	list<ItemList>::iterator itor3;
	ItemList taskDrop;
	List<ArchvUnfinishedTask>licUnfinishTask;  //  角色已接未完成任务
	List<ArchvUnfinishedTask>::iterator taskIter;
	list<UInt32>licTaskID;
	list<UInt32>::iterator result;
	map<UInt32,ItemList>::iterator mapIter;
	list<ItemList> publicdrop;
	List<UInt32> items;
	UInt32 numnum=0;
	PKEndInfo lrpk;
	RoleInfopk roleLev1;
	double expdouble=0.0,extraExp = 0.0;
	UInt32 money=0,extraMoney = 0;
	char Publicdrop=0;
	UInt32 ItemID;
	List<UInt32> a[5];
	int i=0;
	//////test
	List<ArchvUnfinishedTask>::iterator tst;
	list<UInt32>::iterator tstiter;
	list<UInt32>taskid;
	/////


	List<VipRole>licVipRole;   //vip pk 有经验加成和金钱加成
	VipRole viprole;
	List<VipRole>::iterator vipIter;
	Byte isVip = 0;
	int iRet = 0;
	UInt32 vipMoney = 0,vipExp = 0;

	srand((unsigned)(time(NULL)));
	for(itor=roleIDs.begin();itor!=roleIDs.end();itor++)
	{

			roleLev1.Level=_mainSvc->GetCoreData()->ProcessGetRolePtr(*itor)->Level();
			Totallelev=roleLev1.Level+Totallelev;
			roleLev1.RoleID=*itor;
			isVip = _mainSvc->GetCoreData()->ProcessGetRolePtr(*itor)->VIP();
			if(isVip >=1 && isVip <= 6)
			{
				viprole.isVip = isVip;
				viprole.roleID = *itor;
				licVipRole.push_back(viprole);
			}
			roleLev.push_back(roleLev1);
	}

	_mainSvc->GetTaskSvc()->GetRoleUnFinishedTask(roleIDs,licUnfinishTask);//获取pk 胜利方的任务信息
		for(tst = licUnfinishTask.begin();tst != licUnfinishTask.end();tst++)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"roleID[%d]--",tst->roleID);
			taskid = tst->licTaskID;
			for(tstiter = taskid.begin(); tstiter != taskid.end(); tstiter++)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"taskID[%d]--",*tstiter);
			}
		}

			Monster monster;
			int randnum;
			for(itor1=monsters.begin();itor1!=monsters.end();itor1++)
			{
						DealKillMonsterTask(roleIDs,itor1->MonsterType,itor1->num);//杀怪任务
						monster=_mainSvc->GetCoreData()->ProcessGetMonster(mapID,itor1->MonsterType);
						monsterlev=monster.Level();
						expdouble=expdouble+itor1->num*monster.Exp/(pow(abs((int)(Totallelev/roleIDs.size()-monsterlev)+1),1.0/3.0));
						money=money+itor1->num*monster.Money;


						//任务掉落
					    for(mapIter = monster.ItemDropTask.begin();mapIter != monster.ItemDropTask.end();mapIter++)
					    {
							//LOG(LOG_ERROR,__FILE__,__LINE__,"task item loss--taskid[%d]",mapIter->first);
							if(licUnfinishTask.size() == 0)
							     break;                        //没有任务物品掉落

							taskDrop = mapIter->second;
							for(taskIter = licUnfinishTask.begin();taskIter != licUnfinishTask.end();taskIter++)
							{
								licTaskID = taskIter->licTaskID;
								result = find(licTaskID.begin(),licTaskID.end(),mapIter->first);
								if(result != licTaskID.end())   //是否存在杀怪任务
								{
									randnum = rand() % 10000;
								    if(randnum < (taskDrop.num)*100)
									{
										items.push_back(taskDrop.ItemID);
										LOG(LOG_ERROR,__FILE__,__LINE__,"task item loss--taskID[%d]-itemid[%d]",mapIter->first,taskDrop.ItemID);
									}
								}


							}

						}
						//专属掉落
						/*for(itor3=monster.ItemDropSpec.begin();itor3!=monster.ItemDropSpec.end();itor3++)
						{
							randnum=rand()%10000;
							if(randnum<(itor3->num)*100)
							{
								items.push_back(itor3->ItemID);
								LOG(LOG_ERROR,__FILE__,__LINE__,"Special item loss--itemid[%d]",itor3->ItemID);

							}
						}


						publicdrop=_mainSvc->GetCoreData()->GetPublicDrop(mapID,monster.ItemDropPublic);
						//公共掉落
						for(itor3=publicdrop.begin();itor3!=publicdrop.end();itor3++)
						{
							randnum=rand()%10000;
							if(randnum<(itor3->num)*100)
							{
								items.push_back(itor3->ItemID);
								LOG(LOG_ERROR,__FILE__,__LINE__,"public Item loss----- itemid[%d]",itor3->ItemID);
							}
						}*/

			}
			numnum=roleLev.size();

			if(numnum>=5)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"Lev is erro Rolenum is %d  !!!!",numnum);
			}
			if(numnum!=1)
			{
				for(itor=items.begin();itor!=items.end();itor++)
				{
					//	lrpk.pkr.items
						randnum=rand()%numnum;
						a[randnum].push_back(*itor);

//							LOG(LOG_ERROR,__FILE__,__LINE__,"Lev is erro RoleID is  !!!!",itor4->roleID);
				}
			}
			//多于一个人，装备进行随机分配
			i=0;
			for( itor4=lei.begin();itor4!=lei.end();itor4++)
			{
				if(itor4->opposition==1&&itor4->creatureFlag==1) //1为阵容1； 1为角色。
				{
						if(roleLev.size()==1)
						{
							lrpk.roleID=itor4->roleID;
							lrpk.pkei=*itor4;
							lrpk.pkei.live=1;//依然活着
							lrpk.pkr.exp=(UInt32)(expdouble);
						    lrpk.pkr.money = money;
						    lrpk.pkr.pkType = 1; //PVP
						    lrpk.pkr.items = items;

							vipMoney = lrpk.pkr.money;
							vipExp = lrpk.pkr.exp;
							LOG(LOG_ERROR,__FILE__,__LINE__,"-----money[%d]-----exp[%d]",money,lrpk.pkr.exp);
							iRet = VipExtraExpAndMoney(itor4->roleID,vipMoney,vipExp,licVipRole);
							if(iRet)
							{
								_mainSvc->GetBagSvc()->GetBindMoney(itor4->roleID, vipMoney);
								_mainSvc->GetCoreData()->RoleExpAdd(itor4->roleID,vipExp);
								lrpk.pkr.money = vipMoney;
								lrpk.pkr.exp = vipExp;
								LOG(LOG_ERROR,__FILE__,__LINE__,"role[%d]-vipMoney[%d]-vipExp[%d]",lrpk.roleID,vipMoney,vipExp);
							}
							else
							{
								_mainSvc->GetBagSvc()->GetBindMoney(itor4->roleID, money);
								_mainSvc->GetCoreData()->RoleExpAdd(itor4->roleID,lrpk.pkr.exp);
								LOG(LOG_ERROR,__FILE__,__LINE__,"role[%d]-money[%d]-exp[%d]",lrpk.roleID,money,lrpk.pkr.exp);

							}
							//_mainSvc->GetBagSvc()->GetBindMoney(itor4->roleID, money);
						    AddExpByExpRune(itor4, lrpk);
						    //_mainSvc->GetCoreData()->RoleExpAdd(itor4->roleID,lrpk.pkr.exp);
							//掉落物品全是他的
							_mainSvc->GetBagSvc()->RoleGetItem(itor4->roleID,lrpk.pkr.items);

							lpkei.push_back(lrpk);
							break;
						}
						else
						{
							lrpk.roleID=itor4->roleID;
							lrpk.pkei=*itor4;
							lrpk.pkei.live=1;//依然活着
							Lev=findtheLev(roleLev,itor4->roleID);
							if(Lev==0)
							{
									LOG(LOG_ERROR,__FILE__,__LINE__,"Lev is erro RoleID is  %d!!!!",itor4->roleID);
							}
							lrpk.pkr.exp=(UInt32)(expdouble*4*Totallelev/(Lev*5*roleLev.size()));
							lrpk.pkr.money=money/roleLev.size();
							lrpk.pkr.pkType=1;//PVP
							lrpk.pkr.items=a[i];
							i++;
							//掉落
							vipMoney = lrpk.pkr.money;
							vipExp = lrpk.pkr.exp;
							iRet = VipExtraExpAndMoney(itor4->roleID,vipMoney,vipExp,licVipRole);
							if(iRet)
							{
								_mainSvc->GetBagSvc()->GetBindMoney(itor4->roleID, vipMoney);
								_mainSvc->GetCoreData()->RoleExpAdd(itor4->roleID,vipExp);
								lrpk.pkr.money = vipMoney;
								lrpk.pkr.exp = vipExp;
							}
							else
							{
								_mainSvc->GetBagSvc()->GetBindMoney(itor4->roleID, money);
								_mainSvc->GetCoreData()->RoleExpAdd(itor4->roleID,lrpk.pkr.exp);

							}

							//_mainSvc->GetBagSvc()->GetBindMoney(itor4->roleID,lrpk.pkr.money);
							//_mainSvc->GetCoreData()->RoleExpAdd(itor4->roleID,lrpk.pkr.exp);
							_mainSvc->GetBagSvc()->RoleGetItem(itor4->roleID,lrpk.pkr.items);
							//LOG(LOG_ERROR,__FILE__,__LINE__,"")
							lpkei.push_back(lrpk);
						}
				}
			}


}

void PKSvcSS::ProcessPKEnd(Session& session,Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(2048);
	UInt32 pkID=0;
	UInt32 mapID = 0;
	UInt16 tmpMapID = 0;
	Byte winOpposition = 0;
	List<ArchvPKEndInfo> lei;
	List<ArchvPKEndInfo>::iterator it;
	List<ArchvCreatureStatus> lcs;
	list<PKEndInfo> lpkei;

	int iRet = 0;
	Connection con;
	DBOperate dbo;

 	//序列化类
	Serializer s(packet.GetBuffer());
	s>>pkID>>tmpMapID>>winOpposition>>lei;

	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	mapID = tmpMapID;

DEBUG_PRINTF1( " ProcessPKEnd--winOpposition[%d] \n", winOpposition );

	//记录日志,用于 debug
	LOG(LOG_DEBUG,__FILE__,__LINE__,"ProcessPKEnd ok!!pkID[%d],mapID[%d],winOpposition[%d]", pkID, tmpMapID, winOpposition  );
	LOG(LOG_DEBUG,__FILE__,__LINE__,"PKEndInfo :______"  );
	for( it = lei.begin(); it != lei.end(); ++it )
	{
		LOG(LOG_DEBUG,__FILE__,__LINE__,"CreatureFlag[%d],CreatureType[%d],RoleID[%d],Opposition[%d],Live[%d],HP[%d],MP[%d]",
			it->creatureFlag, it->creatureType, it->roleID, it->opposition,
			it->live, it->hp, it->mp);
	}

	//从pk结束包获取信息
	if(winOpposition!=1) //阵容1
	{
		GetInfoFromPkEnd2(lei,lcs,lpkei,mapID,pkID);
		//pk1输了
		//不用怪怪物，什么都没有变，有的是惩罚,查看具体情况，是PVP OR PVE
	}
	else
	{
		GetInfoFromPkEnd1( lei, lcs, lpkei ,mapID,pkID);
	}


	// 战斗结束角色装备耐久度损耗，参加战斗的角色都有装备耐久度损耗
	for(it = lei.begin(); it != lei.end(); it++)
	{
		if(it->creatureFlag == 1 && it->live == 1)	 //针对pve和pvp:战斗结束，角色活着，装备耐久度损耗 1
		{
			iRet = _mainSvc->GetAvatarSvc()->OnEquipDurabilityLoss(it->roleID, 1);
			if(iRet)
			{
			   RetCode = ERR_SYSTEM_SERERROR;
			   LOG(LOG_ERROR,__FILE__,__LINE__,"OnEquipDurabilityLoss Failed ! role[%d]",it->roleID);
			   goto EndOf_Process;
			}
		}
		if(it->creatureFlag == 1 && it->live == 2 ) //针对pve:战斗结束，角色死亡，装备耐久度损耗 10
		{
			iRet = _mainSvc->GetAvatarSvc()->OnEquipDurabilityLoss(it->roleID, 10);
			if(iRet)
			{
			  RetCode = ERR_APP_OP;
			  LOG(LOG_ERROR,__FILE__,__LINE__,"OnEquipDurabilityLoss Failed ! role[%d]",it->roleID);
			  goto EndOf_Process;
			}
		}
		/*if ()//针对pvp(现在还没有):战斗结束，角色死亡，装备耐久度损耗 5
		{}*/

	}



	_mainSvc->GetCoreData()->DeletePkBrif(pkID);
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
//		s<<ri;
	}

	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

DEBUG_PRINTF1( "S_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );


	//---------------------------------S_C 通知-----------------------------------------
	//成功处理，则发送广播
	if( 0 == RetCode )
	{
		//发送pk结束广播
		_mainSvc->GetPkSvc()->NotifyPKEnd(lpkei);

		for( it = lei.begin(); it != lei.end(); ++it )
		{
			_mainSvc->GetBagSvc()->NotifyHPandMP(it->roleID);
			_mainSvc->GetRoleInfoSvc()->NotifyExp(it->roleID);

//			LOG(LOG_DEBUG,__FILE__,__LINE__,"2222222  HP[%d] MP[%d] Exp[%d] ", hp,mp,exp);
		}



		//修改生物状态
		_mainSvc->GetCoreData()->ChangeCreatureStatus( mapID, lcs);

		//发送杀怪通知
//		DealKillMonsterTask( lpkei, lcs );

	}

}



void PKSvcSS::UpdateThedurability(list<RoleEquipToPk> roletype)//变化的Hp，MP
{
		list<RoleEquipToPk>::iterator itor;
		char szSql[1024];
		char szcat2[128];
		char szcat[128];
		char szcat1[128];
		Connection con;
		DBOperate dbo;
		int iRet = 0;
		//获取DB连接
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
		sprintf( szcat1, "0");
		sprintf( szcat2, "0");

		for(itor=roletype.begin();itor!=roletype.end();itor++)
		{

			if(itor->type==1)
			{
				sprintf( szcat, ",%d");
				strcat(szcat1,szcat);
			}
			else if(itor->type==2)
			{
				sprintf( szcat, ",%d");
				strcat(szcat2,szcat);
			}
			else
			{
//				sprintf()
			}
		}
		sprintf( szcat, "));");
		strcat(szcat1,szcat);
		strcat(szcat2,szcat);
		sprintf( szSql, "update Entity set Durability=Durability-1 where Durability>1 and EntityID in(select EntityID from Equip where RoleID in (");
		strcat(szSql,szcat1);
		iRet=dbo.ExceSQL(szSql);
		if(iRet!=0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		}

		sprintf( szSql, "update Entity set Durability=Durability-0.1*(select Durability from Item 	where Item.ItemID=Entity.ItemID)	where EntityID in \
								(select EntityID from Equip where RoleID in (");
		strcat(szSql,szcat2);
		iRet=dbo.ExceSQL(szSql);
		if(iRet!=0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		}
		sprintf( szSql, "update Entity set Durability=0 where Durability>1000 \
							and EntityID in(select EntityID from Equip where RoleID in (");
		strcat(szSql,szcat2);
		iRet=dbo.ExceSQL(szSql);
		if(iRet!=0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		}


}

//@param: roleid  角色id
//@param: extraExp   vip获得的额外经验
//@param: extraMoney   vip 获得的额外金钱
//@param:licRoleVip     pk胜利方中是vip 的角色
//@brief: vip 在pk胜利后获得额外经验和金钱
int PKSvcSS::VipExtraExpAndMoney(UInt32 roleID,UInt32 &Exp,UInt32 &Money,List<VipRole>licRoleVip)
{
	List<VipRole>::iterator vipIter;
	int exp = 0,money = 0;
	int flag = 0;

	for(vipIter = licRoleVip.begin();vipIter != licRoleVip.end(); vipIter++)
	{
		if(roleID == vipIter->roleID)
		{
			if(vipIter->isVip == 1)
			{
				Exp = (int)(1.2 * Exp);
				Money = (int)(1.2 * Money);
			}
			else if(vipIter->isVip == 2)
			{
				Exp = (int)(1.3 * Exp);
				Money = (int)(1.3 * Money);
			}
			else if(vipIter->isVip == 3)
			{
				Exp = (int)(1.4 * Exp);
				Money = (int)(1.4 * Money);
			}
			else
			{
				Exp = (int)(1.5 * Exp);
				Money = (int)(1.5 * Money);
			}

			flag = 1;
			break;
		}


	}

	return flag;
}


void PKSvcSS::UpdatePet(list<PetInfoToPk> petinfo)
{
	list<PetInfoToPk>::iterator itor;
		char szSql[1024];
		Connection con;
		DBOperate dbo;
		int iRet = 0;
		//获取DB连接
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
	for(itor=petinfo.begin();itor!=petinfo.end();itor++)
	{
		//更新宠物数据
	}
}


void PKSvcSS::SetPetHPMPFullAfterPk(int petID)
{
	Connection con;
	DBOperate dbo;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//
	char szSql[1024];
	sprintf(szSql, "update Pet set HP = MaxHP, MP = MaxMP where PetID = %d;", petID);
	int iRet = dbo.ExceSQL(szSql);
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return ;
 	}

	_mainSvc->GetPetSvc()->NotifyPetAttrChange(petID);
}

void PKSvcSS::WinPetAddExp(const list<UInt32> &winPets, const list<PKEndInfo> &lpkei)
{
	Connection con;
	DBOperate dbo;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	char szSql[1024];

	for (list<UInt32>::const_iterator petIter = winPets.begin(); petIter != winPets.end(); ++petIter)
	{
		sprintf(szSql, "select RoleID from Pet where PetID = %d;", *petIter);
		int iRet = dbo.QuerySQL(szSql);
		if (iRet == 1)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL pet not have role[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			continue;
		}

		int roleId = 0;
		while (dbo.HasRowData())
		{
			roleId = dbo.GetIntField(0);
			dbo.NextRow();
		}

		for (list<PKEndInfo>::const_iterator endInfoIter = lpkei.begin(); endInfoIter != lpkei.end(); ++endInfoIter)
		{
			if (endInfoIter->roleID == roleId)
			{
				int roleLevel = _mainSvc->GetCoreData()->ProcessGetRolePtr(endInfoIter->roleID)->Level();
				PetAddExp(*petIter, endInfoIter->pkr.exp, roleLevel);
				break;
			}
		}
	}
}

void PKSvcSS::PetAddExp(int petID, int expAdd, int maxLevel)
{
	Connection con;
	DBOperate dbo;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	char szSql[1024];
	sprintf(szSql, "select Level, Exp, MaxExp, Strength,Intelligence,\
		Agility, AttackPowerHigh, AttackPowerLow from Pet where PetID =%d;",petID);
	int	iRet=dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
		return ;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return ;
 	}

	int level = 0;
	int exp = 0;
	int MaxExp = 0;
	int strength = 0;
	int intelligence = 0;
	int agility = 0;
	int attackPowerHigh = 0;
	int attackPowerLow = 0;
	while (dbo.HasRowData())
	{
		level = dbo.GetIntField(0);
		exp = dbo.GetIntField(1);
		MaxExp = dbo.GetIntField(2);
		strength = dbo.GetIntField(3);
		intelligence = dbo.GetIntField(4);
		agility = dbo.GetIntField(5);
		attackPowerHigh = dbo.GetIntField(6);
		attackPowerLow = dbo.GetIntField(7);
		dbo.NextRow();
	}

	if (exp + expAdd >= MaxExp)
	{
		bool bLevelUp = (level + 1) <= maxLevel ? true : false;
		if (bLevelUp)
		{
			exp = exp + expAdd - MaxExp;
			level = level +1;
			sprintf(szSql, "select MaxExp from LevelDesc where Level=%d;",level+1);
			int	iRet=dbo.QuerySQL(szSql);
			if( 1 == iRet )
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
				return ;
			}
			if( iRet < 0 )
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				return ;
 			}
			while(dbo.HasRowData())
			{

				MaxExp = dbo.GetIntField(0) /3;
				dbo.NextRow();
			}
			if (exp >= MaxExp)
			{
				exp = MaxExp - 2;
			}
			strength += 1;
			intelligence += 1;
			agility +=1;
			attackPowerHigh += 2;
			attackPowerLow += 2;
		}
		else
		{
			// 宠物等级不能高于人物等级，暂时先把宠物经验设成升级经验减2
			exp = MaxExp - 2;
		}
	}
	else
	{
		exp = exp + expAdd;
	}

	sprintf(szSql, "update Pet set Level = %d, Exp = %d, MaxExp = %d, \
		Strength = %d,Intelligence = %d,\
		Agility = %d, AttackPowerHigh = %d, AttackPowerLow = %d where PetID = %d;",
		level, exp, MaxExp,strength, intelligence,agility,attackPowerHigh,attackPowerLow,petID);
	iRet = dbo.ExceSQL(szSql);
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return ;
 	}

	_mainSvc->GetPetSvc()->NotifyPetAttrChange(petID);

}



