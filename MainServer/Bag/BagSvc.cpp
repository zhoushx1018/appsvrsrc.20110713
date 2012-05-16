
#include "BagSvc.h"
#include "MainSvc.h"
#include "DBOperate.h"
#include "ArchvBag.h"
#include "ArchvBagItemCell.h"
#include "./Task/TaskSvc.h"
#include "CoreData.h"
#include "Role.h"
#include "./StoreBag/StoreBagSvc.h"
#include "./Map/MapSvc.h"
#include "./Avatar/AvatarSvc.h"

#include "NewTimer.h"

struct RuneTimerCallBackParam
{
	UInt32 roleID;
	Int32  valueChange;
};

int ToughRuneHpChange[4] = {500,1000,1500,2000};  //坚韧符文
int StoneRuneChange[4] = {500,1000,1500,2000};    //石化符文
int SaintRuneChange[4] = {100,200,300,400};       //神圣符文
int IntelliRuneChange[4] = {10,20,40,80};         //聪明符文
int	CorrectRuneChange[4] = {50,100,150,200};      //精准符文
int SpeedRuneChange[4] = {50,100,150,200};        //急速符文
int	FocusRuneChange[4] = {50,100,150,200};        //专注符文
int	FlyRuneChange[4] = {10,20,40,80};             //飞翔符文
int	AngryRuneChange[4] ={10,20,40,80};    		  //狂暴符文
int	ExtremeRuneChange[4] = {30,60,120,210};       //极限符文

BagSvc::BagSvc(void* service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;
}

BagSvc::~BagSvc()
{
	for (std::map<UInt32, NewTimer**>::iterator iter = _roleRuneTimerMap.begin(); iter != _roleRuneTimerMap.end(); ++iter)
	{
		NewTimer **pTimer = iter->second;
		for (int i = 0; i < RuneTimerNum; i++)
		{
			delete pTimer[i];
		}
		delete []*pTimer;
	}
}

//???????
void BagSvc::OnProcessPacket(Session& session,Packet& packet)
{
DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 801: //?鱳??????б?
		ProcessGetItem(session,packet);
		break;
		case 802://??????????
		ProcessBuyItem(session,packet);
		break;
		case 803://??????????????
		ProcessUseItem(session,packet);
		break;
		case 804://???????
		ProcessDropItem(session,packet);
		break;
		case 805://??????????????
		ProcessSellItem(session,packet);
		break;
		case 899://????????
		ProcessSortItem(session,packet);
		break;
		case 806:
		ProcesschangeItem(session,packet);
		break;
		case 807:
		ProcesschangeTopcell(session,packet);
		break;
		case 808:
		ProcessputintoStoreBag(session,packet);
		break;
		case 809:
		ProcessMixEquip(session,packet);
		break;
		case 810:
		ProcessUseItemFromItemID(session,packet);
		break;
		case 811:
		ProcessUseRune(session,packet);
		break;
		case 812:
		ProcessQueryBagItem(session,packet);
		break;
		default:
			ClientErrorAck(session,packet,ERR_SYSTEM_PARAM);
			LOG(LOG_ERROR,__FILE__,__LINE__,"MsgType[%d] not found",packet.MsgType);
			break;
		}
}




//???????????
//@param  session ???????
//@param	packet ?????
//@param	RetCode ????errorCode ?
void BagSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
{
	//????????
	DataBuffer	serbuffer(1024);
	Packet p(&serbuffer);
	Serializer s(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();

	s<<RetCode;
	p.UpdatePacketLength();

	//??????????
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF1( "C_S ack pkg ----- MsgType[%d]  \n", packet.MsgType );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

}



void BagSvc::ProcessGetItem(Session& session,Packet& packet)
{
//??????????????
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(2048);
	char szSql[1024];
	Connection con,conSub;
	DBOperate dbo,dboSub;
	Int16 iRet = 0;
	UInt32 roleID = packet.RoleID;
	UInt32 money=0;
	UInt32 Gold=0,BindMoney=0;
	UInt32 Gift=0;
	List<ItemCell> lic;
	Byte PackNum=0;
	ItemCell lic1;//????????????


	//???л???
	Serializer s(packet.GetBuffer());
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//???DB????
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	conSub = _cp->GetConnection();
	dboSub.SetHandle(conSub.GetHandle());

	DEBUG_PRINTF( "C_S ProcessGetItem sucess!!!!!!!!\n" );

	//????????
	sprintf( szSql, "select Bag.CellIndex,Bag.ItemID,Bag.EntityID,Bag.Num,Entity.Durability,Entity.BindStatus from Bag left join Entity on Bag.EntityID=Entity.EntityID where RoleID= %d order by CellIndex asc;", roleID);
	iRet=dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		//goto EndOf_Process;
//		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
	}
	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
 	}
		while(dbo.HasRowData())
		{
					lic1.celIndex=dbo.GetIntField(0);
					lic1.ItemID=dbo.GetIntField(1);
					lic1.EntityID=dbo.GetIntField(2);
					lic1.num=dbo.GetIntField(3);
					lic1.durability = dbo.GetIntField(4);
					lic1.bindStatus = dbo.GetIntField(5);
					if(lic1.EntityID!=0)
					{
							lic1.cdTime=0;

					}

					else
					{
						sprintf( szSql, "select CdTime from Item where ItemID= %d;", lic1.ItemID);
						iRet = dboSub.QuerySQL(szSql);
						if(iRet==0)
						{
							lic1.cdTime=dboSub.GetIntField(0);
						}
					}
			lic.push_back(lic1);
		//?????????????
			dbo.NextRow();
		}
	//???Money


		sprintf(szSql, "select Money,BindMoney,Gold,Gift from RoleMoney where RoleID=%d;",roleID);
		iRet=dbo.QuerySQL(szSql);
		if(0==iRet)
		{
			money=dbo.GetIntField(0);
			BindMoney=dbo.GetIntField(1);
			Gold=dbo.GetIntField(2);
			Gift=dbo.GetIntField(3);
		}
		if(iRet==1)
		{
			//return;
		}


		sprintf(szSql, "select TopCellNum from Role where RoleID=%d;",roleID);
		iRet=dbo.QuerySQL(szSql);
		if(0==iRet)
		{
			PackNum=(Byte)(dbo.GetIntField(0)/25);
			if(PackNum==0)
			{
				PackNum=1;
			}
		}
		if(iRet!=0)
		{
			//return;
		}

EndOf_Process:

	//????????
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;

	if( 0 == RetCode )
	{//RetCode ?0 ???????????????
		s<<money;
		s<<BindMoney;
		s<<Gold;
		s<<Gift;
		s<<PackNum;
		s<<lic;
	}

	p.UpdatePacketLength();

	//??????????
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	return;

}

//802璐拱鐗╁搧
void BagSvc::ProcessBuyItem(Session& session,Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	Int16 iRet = 0;
	UInt16 num = 0;
	UInt16 cellType=0;
	UInt16 itemType=0;
	UInt16 IsStack=0;
	UInt32 price=0;
	UInt32 EntityID=0;
	UInt32 CellIndex=0;

	List<UInt16> cell;
	List<ItemCell> lic;
	ItemCell lic1;
	List<UInt16>::iterator itor;

	UInt32 roleID = packet.RoleID;
	UInt32 itemID=0;
	UInt16 count=0;
	UInt32 numcell=1;
	UInt16 Rarity=0;
	UInt32 BuyType=0;
	Byte flag=0;

	//搴忓垪鍖栫被
	Serializer s(packet.GetBuffer());
	s>>itemID>>num;
	//得到物品ID和数??
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//鑾峰彇DB杩炴帴
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//缁勫悎SQL璇彞锛屾煡璇㈠嚭瀹屾暣鐨勭墿鍝佷俊鎭紝鏌ヨ鍑烘潵鐨勬槸鐗╁搧绫诲瀷鍜岀墿鍝佸崟鍏冩牸绫诲瀷
	sprintf( szSql, "select Rarity,IsStack,Bind,CostMoneyType,CostBuy, Durability,ItemType,CellType,CdTime from Item where ItemID = %d;",itemID );

	iRet = dbo.QuerySQL(szSql);
	if( iRet != 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,",szSql[%s] ", szSql);
		goto EndOf_Process;
 	}
 	//分离出数据做成变?? IsStack 是否堆叠   绑定属性Bind   Durability耐久??

		while(dbo.HasRowData())
		{
			Rarity=dbo.GetIntField(0);

			IsStack=dbo.GetIntField(1);
			lic1.bindStatus=dbo.GetIntField(2);
			BuyType=dbo.GetIntField(3);
			price=dbo.GetIntField(4);
			lic1.durability=dbo.GetIntField(5);
			itemType= dbo.GetIntField(6);
			cellType=dbo.GetIntField(7);
			lic1.cdTime=dbo.GetIntField(8);
			//记录集下一条记??
			dbo.NextRow();
		}


	lic1.EntityID=EntityID;
	lic1.ItemID=itemID;

	//鍙煡閬撶墿鍝佷腑涓�簺鏄笉鍏峰瀹炰綋ID鐨勶紝涓嬮潰鏄繘琛屽瀹炰綋ID琛ㄨ繘琛屾彃鍏ユ暟鎹殑浠ｇ爜
	//判断是否具有实体ID函数，是返回1，不是返??，int isEntity(int itemtype);该函数还没有开始编写代??
	if(itemType==2)//具备实体ID的话进行实体表插入， 更改的话对这里的值做个修??
	{
		flag=0;
		if(IsStack==1)
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data error ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}

		if(Ifhassomany(roleID,cell,num)!=1)
		{
			//鑳屽寘涓嶈冻
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__," Bag has not so many room !" );
			goto EndOf_Process;
		}

 			if(BuyType==3)
			{
				if(DropGold(roleID,price*num)!=1)
				{
					RetCode = ERR_SYSTEM_DBNORECORD;
						LOG(LOG_ERROR,__FILE__,__LINE__,"Gold is not enough or others");
						goto EndOf_Process;
				}
			}
			else if(BuyType==1||BuyType==2)
			{
				if(Dropmoney(roleID,price*num)!=1)
				{
						RetCode = ERR_SYSTEM_DBNORECORD;
						LOG(LOG_ERROR,__FILE__,__LINE__,"Money is not enough or others");
						goto EndOf_Process;
				}
			}

	}
	else
	{
		if(IsStack==0)
		{//不可以堆??
				flag=1;
				if(Ifhassomany(roleID,cell,num)!=1)
				{
				//背包没有那么多空??
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"Bag has no Room to insert,szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
				}
		}
		else
		{
				flag=2;
				numcell=num/TOPSTACK_NUM;//一共这么多个单??
				if(num%TOPSTACK_NUM!=0)
				{
					numcell=numcell+1;
				}

				if(Ifhassomany(roleID,cell,numcell)==0)
				{
					//背包没有那么多空??
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"Bag has no Room to insert,szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
				}

	 	}

 		if(BuyType==3)
		{
			if(DropGold(roleID,price*num)!=1)
			{
				RetCode = ERR_SYSTEM_DBNORECORD;
					LOG(LOG_ERROR,__FILE__,__LINE__,"Gold is not enough or others");
					goto EndOf_Process;
			}
		}
		if(BuyType==1||BuyType==2)
		{
			if(Dropmoney(roleID,price*num)!=1)
			{
					RetCode = ERR_SYSTEM_DBNORECORD;
					LOG(LOG_ERROR,__FILE__,__LINE__,"Money is not enough or others");
					goto EndOf_Process;
			}
		}
 	}
	//鍚戞暟鎹簱閲岄潰杩涜鎻掑叆鏁版嵁!
	for( itor = cell.begin(); itor != cell.end(); itor++ )
	{
	if(flag==0)
	{
			sprintf( szSql,"insert into Entity(ItemID,Durability,BindStatus) values(%d,%d,%d);",itemID,lic1.durability,lic1.bindStatus);
				iRet = dbo.ExceSQL(szSql);
				if( iRet < 0 )
				{
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
		 		}

				EntityID=dbo.LastInsertID();
				CellIndex=*itor;
				sprintf(szSql, "insert into Bag(RoleID,CellType,CellIndex,ItemType,ItemID,EntityID,Num)values(%d,%d,%d,%d,%d,%d,1);",roleID,cellType,*itor,itemType,itemID,EntityID);
				iRet=dbo.ExceSQL(szSql);

				if( 1 == iRet )
				{
					RetCode = ERR_SYSTEM_DBNORECORD;
					LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
					goto EndOf_Process;
				}
				if( iRet < 0 )
				{
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
				}

				lic1.num=1;
				lic1.EntityID=EntityID;
				lic1.celIndex=*itor;
				lic.push_back(lic1);
	}
	if(flag==2)
	{
				if(numcell==1)
				{
					//鏁伴噺灏辨槸涓轰紶杈撶殑鏁伴噺
					if(num%TOPSTACK_NUM==0)
					{
						count=TOPSTACK_NUM;
					}
					else
					{
						count=num%TOPSTACK_NUM;
					}
				}
				else
				{
					count=TOPSTACK_NUM;
				}

				sprintf(szSql, "insert into Bag(RoleID,CellType,CellIndex,ItemType,ItemID,EntityID,Num)values(%d,%d,%d,%d,%d,%d,%d);",roleID,cellType,*itor,itemType,itemID,EntityID,count);
				iRet=dbo.ExceSQL(szSql);

				if( 1 == iRet )
				{
					RetCode = ERR_SYSTEM_DBNORECORD;
					LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
					goto EndOf_Process;
				}
				if( iRet < 0 )
				{
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
				}

				lic1.num=count;
				lic1.celIndex=*itor;
				lic.push_back(lic1);
		}
		else if(flag==1)
		{
				sprintf(szSql, "insert into Bag(RoleID,CellType,CellIndex,ItemType,ItemID,EntityID,Num)values(%d,%d,%d,%d,%d,%d,1);",roleID,cellType,*itor,itemType,itemID,EntityID);
				iRet=dbo.ExceSQL(szSql);

				if( 1 == iRet )
				{
					RetCode = ERR_SYSTEM_DBNORECORD;
					LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
					goto EndOf_Process;
				}
				if( iRet < 0 )
				{
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
				}

				lic1.num=1;
				lic1.celIndex=*itor;
				lic.push_back(lic1);
		}
		numcell--;
	}

	//浠诲姟璁℃暟
	_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(roleID,itemID);


	EndOf_Process:

	//组应答数??
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode ?? 才会返回包体剩下内容
		//s<<lic;
	}

	p.UpdatePacketLength();

	//发送应答数??
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
	if( 0 == RetCode )
	{
		NotifyBag(roleID,lic);
 	}
	return;
}


void BagSvc::ProcessUseItem(Session& session,Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	UInt32 roleID = packet.RoleID;

	UInt16 celIndex=0;
	Int16 iRet;
	UInt16 cellType=0;
	UInt32 IfUseSilverCard = 0;   //是否使用银币??0 未使用，1 使用

	UInt16 rolelev=0;
	UInt16 lev=0;
	UInt32 pro=0;
	UInt16 ItemType=0;
	UInt16 num=0;
	UInt32 cdtime=0;
	UInt32 itemId=0,EntityID=0;
	List<ItemCell> lic;
	//List<ItemCell>::iterator itor;

	Serializer s(packet.GetBuffer());
	s>>celIndex;

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	LOG(LOG_DEBUG,__FILE__,__LINE__,"ProcessUseItem--cellIndex[%d]",celIndex);
	rolelev=pRole->Level();
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//鑾峰彇DB杩炴帴
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());



	sprintf( szSql, "select Bag.ItemType,Bag.ItemID,Bag.EntityID,Bag.Num,Item.MinLevel, \
																				Item.ProID,Item.CdTime from Bag \
																				left join Item on Bag.ItemID=Item.ItemID where \
																					Bag.RoleID= %d and Bag.CellIndex=%d;", roleID,celIndex);
	iRet=dbo.QuerySQL(szSql);
	if(  iRet!=0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
	}

	if(dbo.HasRowData())
	{
		ItemType=dbo.GetIntField(0);
		itemId=dbo.GetIntField(1);
		EntityID=dbo.GetIntField(2);
		num=dbo.GetIntField(3);
		lev=dbo.GetIntField(4);
		pro=dbo.GetIntField(5);
		cdtime=dbo.GetIntField(6);
	}
	if(rolelev<lev)
	{
		//绛夌骇涓嶈冻
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"you level is not enough !! itemlev is %d !!your lev is %d" ,lev,rolelev);
		goto EndOf_Process;
	}
	if(pro!=0)
	{
		//鑱屼笟鍒ゆ柇
		Byte flag=0;
		for(;pro>0;pro=pro/10)
		{
			if(pro%10==pRole->ProID())
			{
				flag=1;
				break;
			}
		}
		if(flag==0)
		{
			//职业不允??
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"the pro can not use it");
			goto EndOf_Process;
		}
	}

		RetCode=UseItemExcludeEquip(roleID,ItemType,cdtime,itemId,pRole);
		if(RetCode!=0)
		{

				LOG(LOG_ERROR,__FILE__,__LINE__,"retcode not ==0");
				goto EndOf_Process;
		}


//-----------------------------------------------------------------------------
	if(num==1)
	{//一个的话删除记??
		num--;
		sprintf( szSql, "delete from Bag where RoleID=%d and CellIndex=%d;",roleID,celIndex);
		iRet=dbo.ExceSQL(szSql);
		if( 1 == iRet )
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}
	}
	else
	{
		//澶氫釜鐨勮瘽杩涜鏁版嵁鏇存柊
		num--;
		sprintf( szSql, "update Bag set Num =%d where RoleID=%d and CellIndex=%d;",num,roleID,celIndex);
		iRet=dbo.ExceSQL(szSql);
		if(iRet!=0)
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}
	}

	//消耗品血量增??
		//浠诲姟璁℃暟
	_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(roleID,itemId);

	EndOf_Process:

	//组应答数??
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;
	LOG(LOG_DEBUG,__FILE__,__LINE__,"RetCode[%d]",RetCode);
	if( 0 == RetCode )
	{//RetCode ?? 才会返回包体剩下内容
		//s<<lic;
	}

	p.UpdatePacketLength();

	//发送应答数??
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
	if( 0 == RetCode )
	{
		//NotifyBag(roleID,lic,0);
		if(ItemType==1)
		{
			_mainSvc->GetTaskSvc()->OnUseItem(roleID,itemId);
			NotifyHPandMP(roleID);
		}

		//银币卡使??
		if(itemId == 6000)
		{
		 // LOG(LOG_DEBUG,__FILE__,__LINE__,"notifymoney ok !!");
		  NotifyMoney(roleID);
		}
 	}

	return;
}

//@brief	????????ItemID
void BagSvc::ProcessUseItemFromItemID(Session & session,Packet & packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	UInt32 roleID = packet.RoleID;
	UInt16 celIndex=0;
	Int16 iRet;
	UInt16 cellType=0;

	UInt16 rolelev=0;
	UInt16 lev=0;
	UInt32 pro=0;

	UInt16 ItemType=0;
	int num=0;
	UInt32 cdtime=0;
	UInt32 itemId=0,EntityID=0;
	List<ItemCell> lic;
	ItemCell itemCell;
	UInt32 EquipPos=0;

	Serializer s(packet.GetBuffer());
	s>>itemId;

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	rolelev=pRole->Level();
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//???DB????
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	sprintf( szSql, "select Bag.CellIndex, Bag.ItemType, \
													Bag.EntityID, Bag.Num, Item.MinLevel, \
													Item.ProID, Item.CdTime, Item.EquipPos \
													from Bag left join Item \
														on Bag.ItemID=Item.ItemID \
													where Bag.RoleID= %d \
													  and Bag.ItemID=%d", roleID,itemId);
	iRet=dbo.QuerySQL(szSql);
	if(  iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;//?????????
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
	}
	if(iRet == 1)
	{
		RetCode = NO_ITEM;// 物品不存在
		LOG(LOG_ERROR,__FILE__,__LINE__,"Query Record not found !,szSql[%s] ",szSql);
		goto EndOf_Process;
	}

	if(dbo.HasRowData())
	{
		celIndex=dbo.GetIntField(0);
		ItemType=dbo.GetIntField(1);
		EntityID=dbo.GetIntField(2);
		num=dbo.GetIntField(3);
		lev=dbo.GetIntField(4);
		pro=dbo.GetIntField(5);
		cdtime= dbo.GetIntField(6);
		EquipPos = dbo.GetIntField(7);
	}
	if(rolelev<lev)
	{
		//???????
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"you level is not enough !! itemlev is %d !!your lev is %d" ,lev,rolelev);
		goto EndOf_Process;
	}
	if(pro!=0)
	{
		//???ж?
		Byte flag=0;
		for(;pro>0;pro=pro/10)
		{
			if(pro%10==pRole->ProID())
			{
				flag=1;
				break;
			}
		}
		if(flag==0)
		{
			//????????
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"the pro can not use it");
			goto EndOf_Process;
		}
	}

	//??????????,?????
	//	????,????д???????
	//	??????,??????????????

	if(2==ItemType)
	{//???
		LOG(LOG_ERROR,__FILE__,__LINE__,"UseItem ----itemid[%d]",itemId);
		RetCode=_mainSvc->GetAvatarSvc()->UseItem(roleID,itemId,EntityID,EquipPos,celIndex);
		if(RetCode!=0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"UseItem error!!!!");
			goto EndOf_Process;
		}
	}
	else
	{//?????
		RetCode=UseItemExcludeEquip(roleID,ItemType,cdtime,itemId,pRole);
		if(RetCode!=0)
		{

			LOG(LOG_ERROR,__FILE__,__LINE__,"retcode not ==0");
			goto EndOf_Process;
		}

		//??????????
		num--;
		if(num <=0)
		{//???????????
			sprintf( szSql, "delete from Bag where RoleID=%d and CellIndex=%d;",roleID,celIndex);
			iRet=dbo.ExceSQL(szSql);
			if( 1 == iRet )
			{
				RetCode = ERR_SYSTEM_DBNORECORD;
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
				goto EndOf_Process;
			}
		}
		else
		{
			//????????????????
			sprintf( szSql, "update Bag set Num =%d where RoleID=%d and CellIndex=%d;",num,roleID,celIndex);
			iRet=dbo.ExceSQL(szSql);
			if(iRet!=0)
			{
				RetCode = ERR_SYSTEM_DBNORECORD;
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
				goto EndOf_Process;
			}
		}

		//???????????
		itemCell.celIndex = celIndex;
		itemCell.ItemID = itemId;
		itemCell.EntityID = EntityID;
		itemCell.cdTime = cdtime;
		itemCell.num = num;

		lic.push_back(itemCell);

		//???????????
		//???????
		_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(roleID,itemId);
	}

EndOf_Process:

	//????????
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode ?0 ???????????????
		//s<<lic;
	}

	p.UpdatePacketLength();

	//??????????
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
	if( 0 == RetCode )
	{
		if(2!=ItemType)
			NotifyBag(roleID,lic);

		if(ItemType==1)
		{
			_mainSvc->GetTaskSvc()->OnUseItem(roleID,itemId);
			NotifyHPandMP(roleID);
		}

 	}
	return;
}

//811浣跨敤绗︽枃
void BagSvc::ProcessUseRune(Session & session,Packet & packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024]={0};
	Connection con;
	DBOperate dbo;
	UInt32 roleID = packet.RoleID;
	UInt16 celIndex=0;
	Int16 iRet;
	UInt16 cellType=0;

	UInt16 rolelev=0;
	UInt16 lev=0;
	UInt32 pro=0;
	UInt16 ItemType=0;
	UInt16 num=0;
	UInt32 cdtime=0;
	UInt32 itemId=0,EntityID=0;
	List<ItemCell> lic;

	UInt16 count=0; //鐜╁闇�浣跨敤鐨勭鏂囨暟閲�

	int remainder =0;
	//List<ItemCell>::iterator itor;

	Serializer s(packet.GetBuffer());
	s>>celIndex>>count;

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	LOG(LOG_DEBUG,__FILE__,__LINE__,"rune input value celIndex[%d],count[%d] ", celIndex, count);

	rolelev=pRole->Level();
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//鑾峰彇DB杩炴帴
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());



	sprintf( szSql, "select Bag.ItemType,Bag.ItemID,Bag.EntityID,Bag.Num,Item.MinLevel, \
																				Item.ProID,Item.CdTime from Bag \
																				left join Item on Bag.ItemID=Item.ItemID where \
																					Bag.RoleID= %d and Bag.CellIndex=%d;", roleID,celIndex);
	iRet=dbo.QuerySQL(szSql);

	if(  iRet==1 )
	{
		RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL empty set,szSql[%s] ", szSql);
		goto EndOf_Process;
	}
	if(  iRet!=0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
	}

	if(dbo.HasRowData())
	{
		ItemType=dbo.GetIntField(0);
		itemId=dbo.GetIntField(1);
		EntityID=dbo.GetIntField(2);
		num=dbo.GetIntField(3);
		lev=dbo.GetIntField(4);
		pro=dbo.GetIntField(5);
		cdtime=dbo.GetIntField(6);
	}

	//涓嶆槸缁忛獙绗︽枃绫诲瀷
	if(ItemType!=10)//符文类型??0
	{

			LOG(LOG_ERROR,__FILE__,__LINE__,"not the rune type");
			RetCode = ERR_SYSTEM_DATANOTEXISTS;
			goto EndOf_Process;
	}

	remainder = num - count;

	LOG(LOG_DEBUG, __FILE__, __LINE__, "rune remainder value remainder[%d] ",
			remainder);

	RetCode = UseRune(roleID, cdtime, itemId);

	if (RetCode != 0) {

		LOG(LOG_ERROR, __FILE__, __LINE__, "retcode is %d", RetCode);
		goto EndOf_Process;
	}

//-----------------------------------------------------------------------------
	if (remainder == 0) {//一个的话删除记??

		sprintf(szSql, "delete from Bag where RoleID=%d and CellIndex=%d;",
				roleID, celIndex);


		LOG(LOG_DEBUG, __FILE__, __LINE__, "1 rune :szSql[%s] ", szSql);
		iRet = dbo.ExceSQL(szSql);

		if (0!= iRet) {
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR, __FILE__, __LINE__,
					"QuerySQL data not found ,szSql[%s] ", szSql);
			goto EndOf_Process;
		}
	} else {
		//澶氫釜鐨勮瘽杩涜鏁版嵁鏇存柊
		sprintf(szSql,
				"update Bag set Num =%d where RoleID=%d and CellIndex=%d;",
				remainder, roleID, celIndex);
		LOG(LOG_DEBUG, __FILE__, __LINE__, ">=2 rune :szSql[%s] ", szSql);
		iRet = dbo.ExceSQL(szSql);
		if (iRet != 0) {
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR, __FILE__, __LINE__,
					"QuerySQL data not found ,szSql[%s] ", szSql);
			goto EndOf_Process;
		}
	}

	EndOf_Process:

	//组应答数??
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s << RetCode;
	if (0 == RetCode) {
		NotifyBuffUpdate(roleID, itemId, 2 * 3600); //目前2个小??
	}

	p.UpdatePacketLength();

	//发送应答数??
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	return;
}


//[MsgType:812]根据EntityID 查询背包物品信息
void BagSvc::ProcessQueryBagItem(Session & session,Packet & packet)
{

	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	int iRet = 0;
	UInt32 roleID = packet.RoleID;
	Connection con;
	DBOperate dbo;

    ItemCell itemInfo;
	UInt32 entityID = 0;

	Serializer s(packet.GetBuffer());
	s>>entityID;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//鑾峰彇DB杩炴帴
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	if(0 == entityID)
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"entityID[%d] is error !",entityID);
		goto EndOf_Process;
	}

	sprintf(szSql,"select Bag.CellIndex,\
						  Entity.ItemID,\
						  Entity.EntityID,\
						  Item.CdTime,\
						  Entity.Durability,\
						  Entity.BindStatus\
                   from Bag,Entity,Item \
                   where Bag.EntityID = %d and \
                   Bag.EntityID = Entity.EntityID and\
                   Entity.ItemID = Item.ItemID;",entityID);
    iRet = dbo.QuerySQL(szSql);
	if(iRet == 1)
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"entityID[%d] has no record !",entityID);
		goto EndOf_Process;
	}
	if(iRet == 0)
	{
		itemInfo.celIndex = dbo.GetIntField(0);
		itemInfo.ItemID = dbo.GetIntField(1);
		itemInfo.EntityID = dbo.GetIntField(2);
		itemInfo.cdTime = dbo.GetIntField(3);
		itemInfo.num = 1;
		itemInfo.durability = dbo.GetIntField(4);
		itemInfo.bindStatus = dbo.GetIntField(5);
	}
	else
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"Query SQL not found ! szSql[%d]",entityID);
		goto EndOf_Process;
	}


	EndOf_Process:

	//组应答数??
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s <<RetCode<<itemInfo;

	p.UpdatePacketLength();

	//发送应答数??
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

}


//@brief	??????????????
UInt32 BagSvc::UseItemExcludeEquip(UInt32 roleID,UInt32 ItemType,UInt32 CdTime,UInt32 itemId,RolePtr& pRole)
{
	assert(pRole);
	List<ItemCell> lic;
	UInt32 RetCode=0;

	if(ItemType==1)//????
	{
		if(itemId == 6000) //????
		{
			if(UseSilverCoinCard(roleID))
			{
				RetCode = ERR_APP_DATA;
				LOG(LOG_ERROR,__FILE__,__LINE__,"UseSilverCoinCard Error !");
				return RetCode;
			}
			else
			{
				return 0;
			}
		}
		else
		{

			if(UseItemToHpMp(roleID,itemId,CdTime,pRole)==-1)
			{
				RetCode = ERR_SYSTEM_DBERROR;
				LOG(LOG_ERROR,__FILE__,__LINE__,"erro ");
				return RetCode;
			}
		}

	}
	else	if(ItemType==3) //??????????????
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ItemId[%d]",itemId);
		UInt32 itemID1=CompiteUse(roleID,itemId,lic); //????????????????????
		if(itemID1!=0)
		{
					lic.clear();
					GetItem(roleID,itemID1,lic,1);
					NotifyBag(roleID,lic);
		}
		else
		{
				RetCode = ERR_SYSTEM_DBNORECORD;
				LOG(LOG_ERROR,__FILE__,__LINE__,"erro " );
				return RetCode;
		}
	}
	else if(ItemType==5)//??????
	{
	}
	else if(ItemType==6)//??÷?Ь
	{
	}
	else if(ItemType==8)//?????????
	{
	}
	else if(ItemType==9)//??????
	{
		List<ItemList> lis;
		if(UseGiftBag(roleID,itemId,lis)!=-1)
		{
			RoleGetItem(roleID,lis);
		}
		else
		{
				RetCode = ERR_SYSTEM_DBNORECORD;
				LOG(LOG_ERROR,__FILE__,__LINE__,"erro ");
				return RetCode;
		}

	}
	else if (ItemType == 10)
	{
	}
	else if(ItemType==12)
	{//VIP??
		if(UseVIPItem(roleID,itemId,pRole->VIP())==-1)
		{
				RetCode = ERR_SYSTEM_DBNORECORD;
				LOG(LOG_ERROR,__FILE__,__LINE__,"usr item errro ");
				return RetCode;
		}
	}
	else if(ItemType==11)//???????
	{
	}
	else if (ItemType == 13) //喊话筒
	{

	}
	else
	{
		// itemType????
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"errro " );
		return RetCode;
	}

	return 0;
}

//浣跨敤绗︽枃
UInt32 BagSvc::UseRune(UInt32 roleID, UInt32 CdTime, UInt32 itemId) {

	LOG(LOG_DEBUG,__FILE__,	__LINE__,"4444444  %d  %d",roleID, itemId);

	List<ItemCell> lic;
	UInt32 RetCode = 0;

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	int lastTime = pRole->GetRoleRuneTime(itemId);
	pRole->SetRoleRuneTime(itemId);

	UInt32 teamID = 0;

	if (_roleRuneTimerMap.find(roleID) == _roleRuneTimerMap.end())
	{
		NewTimer **pTimer = new NewTimer*[RuneTimerNum];
		for (int i = 0; i < RuneTimerNum; i++)
		{
			pTimer[i] = new NewTimer;
		}
		_roleRuneTimerMap[roleID] = pTimer;
	}

	switch (itemId) {
	case 6002:
		//澧炲姞缁忛獙
		//设置经验符文flag，当战斗结束的时候经验增加比例??
		break;
	//坚韧符文
	case 6003:
	case 6004:
	case 6005:
	case 6006:
		{
			//1涓篐P
			RoleUseToughRune(roleID, itemId);
		}
		break;
		//石化符文
		case 6007:
		case 6008:
		case 6009:
		case 6010:
			RoleUseStoneRune(roleID, itemId);
			break;
		//神圣符文
		case 6011:
		case 6012:
		case 6013:
		case 6014:
			RoleUseSaintRune(roleID, itemId);
			break;
		//聪明符文
		case 6015:
		case 6016:
		case 6017:
		case 6018:
			RoleUseIntelliRune(roleID, itemId);
			break;
		//精准符文
		case 6019:
		case 6020:
		case 6021:
		case 6022:
			RoleUseCorrectRune(roleID, itemId);
			break;
		//急速符文：
		case 6023:
		case 6024:
		case 6025:
		case 6026:
			RoleUseSpeedRuneRune(roleID, itemId);
			break;
		//专注符文
		case 6027:
		case 6028:
		case 6029:
		case 6030:
			RoleUseFocusRune(roleID, itemId);
			break;
		//飞翔符文
		case 6031:
		case 6032:
		case 6033:
		case 6034:
			RoleUseFlyRune(roleID, itemId);
			break;
		//狂暴符文
		case 6035:
		case 6036:
		case 6037:
		case 6038:
			RoleUseAngryRune(roleID, itemId);
			break;
		//极限符文
		case 6039:
		case 6040:
		case 6041:
		case 6042:
			RoleUseExtremeRune(roleID, itemId);
			break;
		default:
			break;
	}

	LOG(LOG_DEBUG, __FILE__, __LINE__, "1111111  %d  %d ", roleID, itemId);
	if (_mainSvc->GetCoreData()->GetRoleTeamID(roleID, teamID) == 0) {
		list<TeamRole> team =
				_mainSvc->GetCoreData()->GetTeams(teamID).GetMemberRoleID();
		for (list<TeamRole>::iterator iter = team.begin(); iter != team.end(); ++iter) {
			UInt32 teamRoleID = iter->roleId;
			if (teamRoleID != roleID) {
				NotifyTeamBuffUpdate(teamRoleID, roleID, itemId, 2 * 3600); //目前2个小??
				LOG(LOG_DEBUG, __FILE__, __LINE__, "333333  %d %d %d ",
						teamRoleID, roleID, itemId);
			}
		}
	}

	LOG(LOG_DEBUG,__FILE__,	__LINE__,"2222222  %d  %d",roleID, itemId);

	return RetCode;
}

void BagSvc::ProcessSellItem(Session& session,Packet& packet)
{//鐗╁搧鍑哄敭
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	UInt32 roleID = packet.RoleID;
	Serializer s(packet.GetBuffer());
	UInt16 celIndex = 0;
	Int16 iRet;
	UInt32 entityID;
	UInt16 num = 0,sqlnum;
	Byte sellOption = 0,isVip = 0;
	UInt32 itemID=0;
	UInt32 money=0;
	s>>celIndex>>num>>sellOption;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//鑾峰彇DB杩炴帴
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//出售选项 校验
	if(sellOption != 1 && sellOption != 2)
	{
		RetCode = ERR_APP_DATA;
		LOG(LOG_ERROR,__FILE__,__LINE__,"sellOption error !");
		goto EndOf_Process;
	}

	if(sellOption == 1)
	{
		//用户是否是VIP用户
		sprintf(szSql,"select IsVIP from Role where RoleID = %d;",roleID);
		iRet = dbo.QuerySQL(szSql);
		if(iRet != 0)
		{
			RetCode = ERR_APP_DATA;
		    LOG(LOG_ERROR,__FILE__,__LINE__,"Query SQL failed ! szSql[%d]",szSql);
		    goto EndOf_Process;
		}
		isVip = dbo.GetIntField(0);
		if(isVip < 1 || isVip > 6)
		{
			RetCode = ERR_APP_DATA;
		    LOG(LOG_ERROR,__FILE__,__LINE__,"Role[%d] is not vip user !",roleID);
		    goto EndOf_Process;
		}
	}

	//鏌ヨ鍑虹墿鍝佺殑璧勬枡

	sprintf( szSql, "select CellIndex,ItemID,EntityID,Num from Bag where RoleID= %d and CellIndex=%d;", roleID,celIndex);
	iRet=dbo.QuerySQL(szSql);
	if(0==iRet)
	{

		itemID=dbo.GetIntField(1);
		entityID=dbo.GetIntField(2);
		sqlnum=dbo.GetIntField(3);
			//LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
	}
	else
	{
		RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or erro!,szSql[%s] " , szSql);
		goto EndOf_Process;
	}

	LOG(LOG_ERROR,__FILE__,__LINE__,"sell itemID=%d",itemID);
	//实体??表示是不在实体表中有记录??只是需要删除Bag就可??数量??
	if(entityID==0)
	{
		if(sqlnum==num)
		{	sqlnum=0;
			sprintf( szSql, "delete from Bag where RoleID=%d and CellIndex=%d;", roleID,celIndex);
			iRet=dbo.ExceSQL(szSql);
			if( 1 == iRet )
			{
				RetCode = ERR_SYSTEM_DBNORECORD;
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
				goto EndOf_Process;
			}
		}
		else if(sqlnum>num)
		{

				sqlnum=sqlnum-num;
				sprintf( szSql, "update Bag set Num=%d where RoleID=%d and CellIndex=%d;", sqlnum,roleID,celIndex);
				iRet=dbo.ExceSQL(szSql);
				if( 1 == iRet )
				{
					RetCode = ERR_SYSTEM_DBNORECORD;
					LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
					goto EndOf_Process;
				}

		}
		else
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__," there isnot so many item ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}
	}
	else
	{
		sqlnum=0;
		sprintf( szSql, "delete from Bag where RoleID=%d and CellIndex=%d;", roleID,celIndex);
		iRet=dbo.ExceSQL(szSql);
		if( iRet!=0)
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}
		sprintf( szSql, "delete from Entity where EntityID=%d;",entityID);
		iRet=dbo.ExceSQL(szSql);
		if(iRet!=0 )
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}
	}

	sprintf( szSql, "select CostSell from Item where ItemID=%d;",itemID);
	iRet=dbo.QuerySQL(szSql);
	if(iRet!=0)
	{
		RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
	money=dbo.GetIntField(0);

	sprintf( szSql, "update RoleMoney set BindMoney=BindMoney+%d where RoleID=%d;", money*num,roleID);
	//增加游戏??
	iRet=dbo.ExceSQL(szSql);
	if( iRet!=0 )
	{
		RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}

				//浠诲姟璁℃暟
	_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(roleID,itemID);


	EndOf_Process:

	//组应答数??
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode ?? 才会返回包体剩下内容
		//s<<lic;
	}

	p.UpdatePacketLength();

	//发送应答数??
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
	if( 0 == RetCode )
	{
		//NotifyBag(roleID,lic,0);
		NotifyMoney(roleID);
 	}
	return;


}
void BagSvc::ProcessDropItem(Session& session,Packet& packet)
{//鐗╁搧涓㈠純
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	UInt32 roleID = packet.RoleID;
	Serializer s(packet.GetBuffer());
	UInt16 celIndex=0;
	Int16 iRet;
	UInt32 entityID=0;
	UInt32 itemID=0;
	//List<ItemCell> lic;
	//ItemCell lic1;//用来存取的对??
	s>>celIndex;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//鑾峰彇DB杩炴帴
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	sprintf( szSql, "select ItemID,EntityID from Bag where RoleID= %d and CellIndex=%d order by CellIndex;", roleID,celIndex);

		iRet=dbo.QuerySQL(szSql);
		if( 1 == iRet )
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}
 		if(0==iRet)
 		{
 			if(dbo.RowNum()==1)
 			{
 				while(dbo.HasRowData())
				{
					itemID=dbo.GetIntField(0);
					entityID=dbo.GetIntField(1);
					dbo.NextRow();
				}

			}
			else
			{
				RetCode = ERR_SYSTEM_DBERROR;
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				goto EndOf_Process;
			}
 		}
 		if(entityID==0)//??表示是不在实体表中有记录??只是需要删除Bag就可??
 		{

 			sprintf( szSql, "delete from Bag where RoleID=%d and CellIndex=%d;", roleID,celIndex);
 			iRet=dbo.ExceSQL(szSql);
 			if( 1 == iRet )
			{
				RetCode = ERR_SYSTEM_DBNORECORD;
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
				goto EndOf_Process;
			}
 		}
 		else
 		{//鍘讳簡瀹炰綋琛ㄣ�鍚屾椂鍘绘帀BAG
 			sprintf( szSql, "delete from Entity where EntityID=%d;",entityID);
 			iRet=dbo.ExceSQL(szSql);
 			if( 1 == iRet )
			{
				RetCode = ERR_SYSTEM_DBNORECORD;
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
				goto EndOf_Process;
			}
			sprintf( szSql, "delete from Bag where RoleID=%d and CellIndex=%d;", roleID,celIndex);
 			iRet=dbo.ExceSQL(szSql);
 			if( 1 == iRet )
			{
				RetCode = ERR_SYSTEM_DBNORECORD;
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
				goto EndOf_Process;
			}
 		}

				//浠诲姟璁℃暟
		_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(roleID,itemID);

	EndOf_Process:

	//组应答数??
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode ?? 才会返回包体剩下内容
		//s<<lic;
	}

	p.UpdatePacketLength();

	//发送应答数??
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
	if( 0 == RetCode )
	{
		//
 	}
	return;
}


void BagSvc::ProcessSortItem(Session& session,Packet& packet)
{//鐗╁搧鏁寸悊
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(2048);
	char szSql[1024];
	Connection con,conSub;
	DBOperate dbo,dboSub;
	Int16 iRet = 0;
	UInt32 roleID = packet.RoleID;
	UInt32 money=0;
	UInt16 celIndex,num,num1,num2=0;
	UInt32 iemID;
	List<ItemCell> lic;
	ItemCell lic1;//用来存取的对??
	UInt32 cellnum=0,sql_cell;

	//搴忓垪鍖栫被
	Serializer s(packet.GetBuffer());
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//鑾峰彇DB杩炴帴
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	conSub = _cp->GetConnection();
	dboSub.SetHandle(conSub.GetHandle());

	DEBUG_PRINTF( "C_S ProcessSortItem sucess!!!!!!!!\n" );
	//查询全部的堆叠物??
	//提取了没有满数量的单元，以及数量大于1的单??
	sprintf( szSql, "select CellIndex,ItemID,EntityID,Num \
						from Bag where RoleID= %d and \
							EntityID = 0 and  \
							(select IsStack from Item where ItemID=Bag.ItemID)=1 and \
								Num <> %d order \
								by ItemID desc,Num desc;", roleID,TOPSTACK_NUM);
	iRet=dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		//RetCode = ERR_SYSTEM_DBNORECORD;
		//LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
		//goto EndOf_Process;
	}
	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
	}

	while(dbo.HasRowData())
	{
		celIndex=dbo.GetIntField(0);
		iemID=dbo.GetIntField(1);
		num=dbo.GetIntField(3);		//鑾峰彇鏁版嵁
		dbo.NextRow();
		while(dbo.HasRowData()&&iemID==dbo.GetIntField(1))
		{
			num=num+dbo.GetIntField(3);
			if(num>TOPSTACK_NUM)
			{
				sprintf( szSql, "update Bag set Num=%d where RoleID= %d and CellIndex=%d;",TOPSTACK_NUM,roleID,celIndex);
				iRet=dboSub.ExceSQL(szSql);
				//大于最大堆叠，就在index上存放最大??
				num=num-TOPSTACK_NUM;
			}
			else
			{
				sprintf( szSql, "delete from Bag where  RoleID= %d and CellIndex=%d;",roleID,celIndex);
				iRet=dboSub.ExceSQL(szSql);
			}
			celIndex=dbo.GetIntField(0);
			dbo.NextRow();
		}
		sprintf( szSql, "update Bag set Num=%d where RoleID= %d and CellIndex=%d;",num,roleID,celIndex);
		iRet=dboSub.ExceSQL(szSql);
	}


	dbo.SetHandle(con.GetHandle());
	//鎶婃暟鎹簱鐨勫�鍏ㄩ儴鍒掑垎鍒�000鍗曞厓浠ュ悗
	sprintf( szSql, "update Bag set CellIndex=CellIndex+1000 where RoleID= %d;",roleID);
	iRet=dbo.ExceSQL(szSql);
	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQLSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
	}
	//查询全部??
	sprintf( szSql, "select CellIndex,ItemID,EntityID,Num from Bag where RoleID= %d  order by ItemType,CellIndex asc;", roleID);
	iRet=dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		//RetCode = ERR_SYSTEM_DBNORECORD;
		//LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
	}

	while(dbo.HasRowData())
	{
		cellnum++;
		sql_cell=dbo.GetIntField(0);
		if(sql_cell>cellnum)
		{
			sprintf( szSql, "update Bag set CellIndex=%d where RoleID= %d and CellIndex=%d;", cellnum,roleID,sql_cell);
			iRet=dboSub.ExceSQL(szSql);
		}
		dbo.NextRow();
	}


		//查询全部??
		sprintf( szSql, "select Bag.CellIndex,Bag.ItemID,Bag.EntityID,Bag.Num,Entity.Durability,Entity.BindStatus from Bag left join Entity on Bag.EntityID=Entity.EntityID where Bag.RoleID= %d order by Bag.CellIndex asc;", roleID);
		iRet=dbo.QuerySQL(szSql);
		if( 1 == iRet )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
		}
		if( iRet < 0 )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
 		}

		while(dbo.HasRowData())
		{
			lic1.celIndex=dbo.GetIntField(0);
			lic1.ItemID=dbo.GetIntField(1);
			lic1.EntityID=dbo.GetIntField(2);
			lic1.num=dbo.GetIntField(3);
			lic1.durability = dbo.GetIntField(4);
			lic1.bindStatus = dbo.GetIntField(5);

			if(lic1.EntityID!=0)
			{

					lic1.cdTime=0;

			}
			else
			{
				sprintf( szSql, "select CdTime from Item where ItemID= %d;", lic1.ItemID);
				iRet = dboSub.QuerySQL(szSql);
				if(iRet==0)
				{
					lic1.cdTime=dboSub.GetIntField(0);
				}
			}

			lic.push_back(lic1);
			//记录集下一条记??
			dbo.NextRow();
		}



	EndOf_Process:

	//组应答数??
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;

	if( 0 == RetCode )
	{//RetCode ?? 才会返回包体剩下内容
		//s<<money;
		s<<lic;
	}

	p.UpdatePacketLength();

	//发送应答数??
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
	if( 0 == RetCode )
	{
		//NotifyBag(roleID,lic,1);
	}

	return;
}




//鐗╁搧浣嶇疆浜ゆ崲
void BagSvc::ProcesschangeItem(Session & session,Packet & packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	UInt32 roleID = packet.RoleID;
	Serializer s(packet.GetBuffer());
	Int16 celIndex1=0;
	Int16 celIndex2=0;
	Int16 iRet,num=0;
	UInt32 entityID=0;
	UInt32 itemID=0;
	//List<ItemCell> lic;
	//ItemCell lic1;//用来存取的对??
	s>>celIndex1>>celIndex2;
//	LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%d,%d] " , celIndex1,celIndex2);
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	//灏哻ellindex2鍒ゆ柇涓�笅鏄笉鏄┖鐨�
	sprintf( szSql, "select ItemID from Bag where RoleID= %d and CellIndex=%d",roleID,celIndex1);
	iRet=dbo.QuerySQL(szSql);
	if(iRet!=0 )
	{
		RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
	sprintf( szSql, "select ItemID from Bag where RoleID= %d and CellIndex=%d",roleID,celIndex2);
	iRet=dbo.QuerySQL(szSql);
	if( 1 == iRet )
		{
			//鐩爣浣嶇疆鏄┖鐨勶紝灏嗘暟鎹瓨鍒扮┖浣嶇疆灏辫
			sprintf( szSql, "update Bag set CellIndex=%d where RoleID= %d and CellIndex=%d;",celIndex2,roleID,celIndex1);
			iRet=dbo.ExceSQL(szSql);
			if( 1 == iRet )
			{
				RetCode = ERR_SYSTEM_DBNORECORD;
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
				goto EndOf_Process;
			}
		}
	else
	{
		if( iRet < 0 )
		{
			RetCode = ERR_SYSTEM_DBERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			goto EndOf_Process;
		}
		//灏哻ellindx1鐨勫�鎹㈠埌鐗瑰埆浣嶇疆锛�000鍗曞厓浠ュ悗
		sprintf( szSql, "update Bag set CellIndex=CellIndex+1000 where RoleID= %d and CellIndex=%d;",roleID,celIndex1);
		iRet=dbo.ExceSQL(szSql);
		if( 1 == iRet )
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}
		//灏哻ellindex2鐨勫�鏀惧湪cellindex1涓紝
		sprintf( szSql, "update Bag set CellIndex=%d where RoleID= %d and CellIndex=%d;",celIndex1,roleID,celIndex2);
		iRet=dbo.ExceSQL(szSql);
		if( 1 == iRet )
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}
		//灏哻ellindex1涓殑鍊兼斁鍦╟ellindex2
		sprintf( szSql, "update Bag set CellIndex=%d where RoleID= %d and CellIndex=%d;",celIndex2,roleID,celIndex1+1000);
		iRet=dbo.ExceSQL(szSql);
		if( 1 == iRet )
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}
	}
	EndOf_Process:

	//组应答数??
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode ?? 才会返回包体剩下内容
		//s<<lic;
	}

	p.UpdatePacketLength();

	//发送应答数??
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
	if( 0 == RetCode )
	{
	//	NotifyBag(roleID,lic,0);
 	}
	return;


}


void BagSvc::ProcesschangeTopcell(Session & session,Packet & packet)
{

		UInt32	RetCode = 0;
		DataBuffer	serbuffer(1024);
		char szSql[1024];
		Connection con;
		DBOperate dbo;
		Byte type=0;
		Int16 iRet=0;
		UInt32 topcellnum=0;
		UInt32 roleID = packet.RoleID;
		//鑾峰彇DB杩炴帴
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
					//搴忓垪鍖栫被
		Serializer s(packet.GetBuffer());
		s>>type;

		RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

		if( s.GetErrorCode()!= 0 )
		{
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
			goto EndOf_Process;
		}
		topcellnum=pRole->TopCellNum();
		if(topcellnum==0)
		{
			topcellnum=25;//
		}
		if(type==0)
		{
			//短期的不知道怎么处理，暂时不??
		}
		if(type==1)
		{

			topcellnum=topcellnum+25;
			LOG(LOG_ERROR,__FILE__,__LINE__,"serial error%d",topcellnum);
		}
		if(topcellnum==100)
		{
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"The largest BagCell" );
			goto EndOf_Process;
		}
		if(DropGold(roleID,100)==-1)
		{
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"Gold is Not ....." );
			goto EndOf_Process;
		}
		pRole->TopCellNum(topcellnum);

		DEBUG_PRINTF( "C_S ProcessProcessChange topcell sucess!!!!!!!!\n" );


		EndOf_Process:
		//组应答数??
		Packet p(&serbuffer);
		s.SetDataBuffer(&serbuffer);
		serbuffer.Reset();

		p.CopyHeader(packet);
		p.Direction = DIRECT_C_S_RESP;
		p.PackHeader();

		s<<RetCode;

		if( 0 == RetCode )
		{//RetCode ?? 才会返回包体剩下内容
		s<<roleID;
		}

		p.UpdatePacketLength();

		//发送应答数??
		if( session.Send(&serbuffer) )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
		}

		DEBUG_PRINTF( "ack pkg=======, \n" );
		DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
		if(0==RetCode)
		{
		}
		return;
}



void BagSvc::ProcessputintoStoreBag(Session & session,Packet & packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	Int16 iRet = 0;

	UInt16 storeCellIndex,CellIndex=0;
	UInt32 ItemType1=0,ItemType2=0;
	UInt32 CellType1=0,CellType2=0;
	List<UInt16> cell;
	List<ItemCell> lic,lic22;
	ItemCell lic1,lic2;
	UInt32 roleID = packet.RoleID;
			con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
	//搴忓垪鍖栫被
	Serializer s(packet.GetBuffer());
	s>>CellIndex>>storeCellIndex;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//鍒ゆ鐩爣鏈夋病鏈夌墿鍝侊紝
		sprintf(szSql, "select Bag.CellIndex,Bag.CellType,Bag.ItemType,Bag.ItemID,Bag.EntityID,Bag.Num,Entity.Durability,Entity.BindStatus from Bag left join Entity on Bag.EntityID=Entity.EntityID where RoleID= %d  and Bag.CellIndex=%d;",roleID,CellIndex);
			iRet=dbo.QuerySQL(szSql);
			if(0==iRet)
			{
				lic1.celIndex=dbo.GetIntField(0);
				CellType1=dbo.GetIntField(1);
				ItemType1=dbo.GetIntField(2);
				lic1.ItemID=dbo.GetIntField(3);
				lic1.EntityID=dbo.GetIntField(4);
				lic1.num=dbo.GetIntField(5);
				lic1.durability=dbo.GetIntField(6);
				lic1.bindStatus=dbo.GetIntField(7);
			}
			if(iRet!=0)
			{

					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
			}
		//判断背包有没有物??
		sprintf(szSql, "select StoreBag.CellIndex,StoreBag.CellType, \
										StoreBag.ItemType, StoreBag.ItemID,StoreBag.EntityID, \
										StoreBag.Num,Entity.Durability,Entity.BindStatus \
										from StoreBag left join Entity on StoreBag.EntityID=Entity.EntityID \
										where RoleID= %d  and StoreBag.CellIndex=%d;",roleID,storeCellIndex);
			iRet=dbo.QuerySQL(szSql);
			if(0==iRet)
			{
				//鏈塽pdate
				lic2.celIndex=dbo.GetIntField(0);
				CellType2=dbo.GetIntField(1);
				ItemType2=dbo.GetIntField(2);
				lic2.ItemID=dbo.GetIntField(3);
				lic2.EntityID=dbo.GetIntField(4);
				lic2.num=dbo.GetIntField(5);
				lic2.durability=dbo.GetIntField(6);
				lic2.bindStatus=dbo.GetIntField(7);

				//鏇存柊涓ゅご
				sprintf(szSql, "update StoreBag set CellType=%d,ItemType=%d,ItemID=%d,EntityID=%d,Num=%d where RoleID=%d and CellIndex=%d;",CellType1,ItemType1,lic1.ItemID,lic1.EntityID,lic1.num,roleID,storeCellIndex );
				iRet=dbo.ExceSQL(szSql);
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				if(iRet!=0)
				{
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
					//
				}
				sprintf(szSql, "update Bag set CellType=%d,ItemType=%d,ItemID=%d,EntityID=%d,Num=%d where RoleID=%d and CellIndex=%d;",CellType2,ItemType2,lic2.ItemID,lic2.EntityID,lic2.num,roleID,CellIndex);
				iRet=dbo.ExceSQL(szSql);
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				if(iRet!=0)
				{
					//
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
				}


			}
			if(iRet==1)
			{
				//娌℃湁锛岀洿鎺nsert
				sprintf(szSql, "insert into StoreBag(RoleID,CellIndex,CellType,ItemType,ItemID,EntityID,Num) \
				                values(%d,%d,%d,%d,%d,%d,%d);",roleID,storeCellIndex,CellType1,ItemType1, lic1.ItemID,lic1.EntityID,lic1.num );

				iRet=dbo.ExceSQL(szSql);
//				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				if(iRet!=0)
				{
					//
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
				}

				//delete鍘熸潵鐨剆torebag
				sprintf(szSql, "delete from Bag where RoleID=%d and CellIndex=%d;",roleID,CellIndex );
				iRet=dbo.ExceSQL(szSql);
//				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				if(iRet!=0)
				{
					//
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
				}
				//OK
			}




//鏌ヨCD
					if(lic2.ItemID!=0)
					{
						lic2.celIndex=CellIndex;
					  lic1.celIndex=storeCellIndex;

							if(lic1.EntityID==0)
							{
								sprintf(szSql, "select CdTime from Item where ItemID=%d;",lic1.ItemID);
									iRet=dbo.QuerySQL(szSql);
									if(iRet==0)
									{
										lic1.cdTime=dbo.GetIntField(0);
									}
									if(iRet!=0)
									{
										RetCode = ERR_SYSTEM_DBERROR;
										LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
										goto EndOf_Process;
									}
							}
							if(lic2.EntityID==0)
							{
								sprintf(szSql, "select CdTime from Item where ItemID=%d;",lic2.ItemID);
									iRet=dbo.QuerySQL(szSql);
									if(iRet==0)
									{
										lic2.cdTime=dbo.GetIntField(0);
									}
									if(iRet!=0)
									{
										RetCode = ERR_SYSTEM_DBERROR;
										LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
										goto EndOf_Process;
									}
							}
							lic22.push_back(lic2);
							lic.push_back(lic1);
					}
				else
					{
							lic1.celIndex=storeCellIndex;
							if(lic1.EntityID==0)
							{
								sprintf(szSql, "select CdTime from Item where ItemID=%d;",lic1.ItemID);
									iRet=dbo.QuerySQL(szSql);
									if(iRet==0)
									{
										lic1.cdTime=dbo.GetIntField(0);
									}
									if(iRet!=0)
									{
										RetCode = ERR_SYSTEM_DBERROR;
										LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
										goto EndOf_Process;
									}
							}
							lic.push_back(lic1);
					}



	//浠诲姟璁℃暟
	_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(roleID,lic1.ItemID);
	if(lic2.ItemID!=0)
	{
		_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(roleID,lic2.ItemID);
	}



	EndOf_Process:

	//组应答数??
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode ?? 才会返回包体剩下内容
		//s<<lic;
	}

	p.UpdatePacketLength();

	//发送应答数??
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
	if( 0 == RetCode )
	{
		if(lic2.ItemID!=0)
		{
			NotifyBag(roleID,lic22);
		}
		_mainSvc->GetStoreSvc()->NotifyStoreBag(roleID,lic);
 	}
	return;
}



void BagSvc::ProcessMixEquip(Session & session,Packet & packet)//瑁呭淇悊809
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	char szcat[128];
	Connection con;
	DBOperate dbo;
	Int16 iRet = 0;
	Byte type=0;
	UInt16 index = 0;
	Byte mixOption = 0,isVip = 0;
	list<EquipItem> itms;
	EquipItem itm;
	UInt32 needMoney=0;
	UInt32 roleID = packet.RoleID;
	//int filedNum = 0;

	ItemCell item;
	List<ItemCell>licItem;
	List<ItemCell>::iterator iter;
	UInt32 isMixBagEquip = 0;

	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	//搴忓垪鍖栫被
	Serializer s(packet.GetBuffer());
	s>>type>>index>>mixOption;
	LOG(LOG_DEBUG,__FILE__,__LINE__,"type[%d]---index[%d]-mixOption[%d]",type,index,mixOption);
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//mixOptin 校验/////
	if(mixOption != 1 && mixOption != 2)
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"mixOption error ! mixOption[%d]",mixOption);
		goto EndOf_Process;
	}


	if(mixOption == 1)
	{
		//vip校验
		sprintf(szSql,"select IsVIP from Role where RoleID = %d;",roleID);
		iRet = dbo.QuerySQL(szSql);
		if(iRet != 0)
		{
			RetCode = ERR_APP_DATA;
		    LOG(LOG_ERROR,__FILE__,__LINE__,"Query SQL failed ! szSql[%s]",szSql);
		    goto EndOf_Process;
		}

		isVip = dbo.GetIntField(0);
		if(isVip < 1 || isVip > 6)
		{
			RetCode = ERR_APP_DATA;
		    LOG(LOG_ERROR,__FILE__,__LINE__,"role[%d] is not vip user !",roleID);
		    goto EndOf_Process;
		}
	}


	if(type==1||type==3)   //装备栏
	{
			sprintf(szSql, "select Equip.EquipIndex,Equip.ItemID,Equip.EntityID,Item.CdTime,Entity.BindStatus\
	                       from Equip,Item,Entity\
						   where RoleID=%d and Equip.ItemType = 2 and\
						   Equip.EntityID = Entity.EntityID and \
						   Entity.ItemID = Item.ItemID and Entity.Durability<>0",roleID);
			if(index!=0)
			{
				sprintf(szcat," and EquipIndex=%d",index);
				strcat(szSql,szcat);

			}

			strcat(szSql,";");
			isMixBagEquip = 0;

	}
	else if(type==2||type==4)  //背包栏
	{
			sprintf(szSql, "select Bag.CellIndex,Bag.ItemID,Bag.EntityID,Item.CdTime,Entity.BindStatus,Bag.Num\
				            from Bag,Item,Entity\
						    where RoleID=%d and Bag.ItemType = 2 and\
							Bag.EntityID = Entity.EntityID and \
							Entity.ItemID = Item.ItemID and Entity.Durability<>0",roleID);
			if(index!=0)
			{
				sprintf(szcat," and CellIndex=%d",index);
				strcat(szSql,szcat);

			}
			strcat(szSql,";");
			isMixBagEquip = 1;

	}

	iRet=dbo.QuerySQL(szSql);
	LOG(LOG_ERROR,__FILE__,__LINE__,"szSql[%s]",szSql);
	if(iRet!=0)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
	}

	//filedNum = dbo.FieldNum();
	if(isMixBagEquip == 1)
	{
		while(dbo.HasRowData())
	    {
			item.celIndex = dbo.GetIntField(0);
			item.ItemID= itm.ItemID=dbo.GetIntField(1);
			item.EntityID = itm.EntityID = dbo.GetIntField(2);
			item.cdTime = dbo.GetIntField(3);
			item.bindStatus = dbo.GetIntField(4);
			item.num = dbo.GetIntField(5);
			item.durability = 100;

			itms.push_back(itm);
		    licItem.push_back(item);
			dbo.NextRow();
	    }
	}
	if(isMixBagEquip == 0)
	{
		while(dbo.HasRowData())
	    {
			item.celIndex = dbo.GetIntField(0);
			item.ItemID= itm.ItemID=dbo.GetIntField(1);
			item.EntityID = itm.EntityID = dbo.GetIntField(2);
			item.cdTime = dbo.GetIntField(3);
			item.bindStatus = dbo.GetIntField(4);
			item.num = 1;
			item.durability = 100;

			itms.push_back(itm);
		    licItem.push_back(item);
			dbo.NextRow();
	    }
	}

	needMoney=SelectMixEquipMoney(itms);

	LOG(LOG_ERROR,__FILE__,__LINE__,"needmoney[%d]",needMoney);
	if(needMoney==0)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"there are some erro here! ");
		goto EndOf_Process;
	}
	else
	{
	  iRet = Dropmoney(roleID,needMoney);
		if(iRet == -1)
		{
			RetCode = NO_MUCH_MONEY;
			LOG(LOG_ERROR,__FILE__,__LINE__,"RoleID[%d] has no much money !",roleID);
			goto EndOf_Process;
		}

		if(iRet == 0)
		{
		  RetCode = ERR_SYSTEM_DBERROR;
		  LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error !");
		  goto EndOf_Process;
		}
	}

	iRet=UpdateMixEquip(itms);

	if(iRet==-1)
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"there are some erro here! ");
		goto EndOf_Process;
	}

	EndOf_Process:

	//组应答数??
	Packet p(&serbuffer);
	s.SetDataBuffer(&serbuffer);
	serbuffer.Reset();

	p.CopyHeader(packet);
	p.Direction = DIRECT_C_S_RESP;
	p.PackHeader();


	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode ?? 才会返回包体剩下内容
		//s<<lic;
	}

	p.UpdatePacketLength();

	//发送应答数??
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}


	if( 0 == RetCode )
	{
		if(type == 1 || type == 3)
		{//s-c 502
			LOG(LOG_DEBUG,__FILE__,__LINE__,"--------NotifyEquipUpdate--nsize[%d]",licItem.size());
			_mainSvc->GetAvatarSvc()->NotifyEquipUpdate(roleID,licItem);
		}
		if(type == 2 || type == 4)
		{//s -c 801
			NotifyBag(roleID,licItem);
		}
	  //?????????
	  //NotifyEquipDurabilityLoss(roleID, licMixEquip);
 	}
	return;
}


//S-C的血量，和蓝??
void BagSvc::NotifyHPandMP(UInt32 roleID)
{
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	UInt32 hp=0,mp=0;
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

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	hp=pRole->Hp();
	mp=pRole->Mp();
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

void BagSvc::NotifyMoney(UInt32 roleID )
{
		DataBuffer	serbuffer(1024);
		char szSql[1024];
		int iRet=0;
		UInt32 money=0;
		UInt32 Bindmoney=0;
		UInt32 Gold=0;
		Serializer s( &serbuffer );
		Packet p(&serbuffer);
		serbuffer.Reset();
		p.Direction = DIRECT_S_C_REQ;
		p.SvrType = 1;
		p.SvrSeq = 1;
		p.MsgType = 802;
		p.UniqID = 251;
		p.PackHeader();
		List<UInt32> lrid;
//		char szSql[1024];
		Connection con;
		DBOperate dbo;
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
		sprintf(szSql, "select Money,BindMoney,Gold from RoleMoney where RoleID=%d;",roleID);
		iRet=dbo.QuerySQL(szSql);
		if(0==iRet)
		{
			money=dbo.GetIntField(0);
			Bindmoney=dbo.GetIntField(1);
			Gold=dbo.GetIntField(2);
		}
		else
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found or erro!,szSql[%s] " , szSql);
		}


		s<<money<<Bindmoney<<Gold;
		p.UpdatePacketLength();
		lrid.push_back(roleID);
		if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
		}
		DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
		DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );

}

//S-C的一个广??
void BagSvc::NotifyBag(UInt32 roleID,List<ItemCell>& lic)
{
	DataBuffer	serbuffer(1024);
	Int16 iRet = 0;
	List<UInt32> lrid;
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 801;
	p.UniqID = 250;
	p.PackHeader();

	s<<lic;
	p.UpdatePacketLength();
	lrid.push_back(roleID);

	if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}

//S-C的Buff更新，比如使用经验符文??
void BagSvc::NotifyBuffUpdate(UInt32 roleID, UInt32 buffID, UInt32 buffTime)
{
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 2201;
	p.UniqID = 252;
	p.PackHeader();
	List<UInt32> lrid;
	s<<buffID<<buffTime;

	LOG(LOG_ERROR,__FILE__,__LINE__,"%d,%d !!",buffID,buffTime);
	p.UpdatePacketLength();
	lrid.push_back(roleID);
	if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );


}


//S-C的Buff更新，只针对队友，比如使用经验符文??
void BagSvc::NotifyTeamBuffUpdate(UInt32 teamRoleID, UInt32 roleID, UInt32 buffID, UInt32 buffTime)
{
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 2204;
	p.UniqID = 257;
	p.PackHeader();
	List<UInt32> lrid;
	s<<roleID<<buffID<<buffTime;

	LOG(LOG_ERROR,__FILE__,__LINE__,"%d,%d !!",buffID,buffTime);
	p.UpdatePacketLength();
	lrid.push_back(teamRoleID);
	if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}



//寰楀埌鐗╁搧锛屽崟涓猲um鏁伴噺蹇呴』灏忎簬TOPSTALL
UInt16 BagSvc::GetItem(UInt32 roleID,UInt32 ItemID,List<ItemCell>& lic,UInt16 num)
{

	char szSql[1024];
	Connection con;
	DBOperate dbo;
	Int16 iRet = 0;
	UInt16 cellType=0;
	UInt16 itemType=0;
	UInt16 IsStack=0;
	UInt16 Bind=0;
	UInt32 Dur=0;
	UInt32 EntityID=0;
	UInt32 CellIndex=0;
	ItemCell lic1;

	UInt16 count;
	//UInt32 numcell=0;

	CellIndex=IfhascellIndex(roleID);
	if(CellIndex==0)
	{//背包没有位置??
		return 0;
	}

	//鑾峰彇DB杩炴帴
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//缁勫悎SQL璇彞锛屾煡璇㈠嚭瀹屾暣鐨勭墿鍝佷俊鎭紝鏌ヨ鍑烘潵鐨勬槸鐗╁搧绫诲瀷鍜岀墿鍝佸崟鍏冩牸绫诲瀷
	sprintf( szSql, "select IsStack,Bind,CostBuy, \
			Durability,ItemType,CellType from Item where ItemID = %d;",ItemID);

	iRet = dbo.QuerySQL(szSql);
	if( 1 == iRet)
	{
		//澧炲姞鐨勭墿鍝佹槸涓嶅瓨鍦ㄧ殑
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data what to get not found,szSql[%s] " , szSql);
		return 0;
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,",szSql[%s] ", szSql);
		return 0;
 	}

 	//分离出数据做成变?? IsStack 是否堆叠   绑定属性Bind   Durability耐久??
	if(dbo.RowNum()==1)
	while(dbo.HasRowData())
	{

		IsStack=dbo.GetIntField(0);
		Bind=dbo.GetIntField(1);
//		price=dbo.GetIntField(2);
		Dur=dbo.GetIntField(3);
		itemType= dbo.GetIntField(4);
		cellType=dbo.GetIntField(5);

		//记录集下一条记??
		dbo.NextRow();
	}
	//鍙煡閬撶墿鍝佷腑涓�簺鏄笉鍏峰瀹炰綋ID鐨勶紝涓嬮潰鏄繘琛屽瀹炰綋ID琛ㄨ繘琛屾彃鍏ユ暟鎹殑浠ｇ爜
	//判断是否具有实体ID函数，是返回1，不是返??，int isEntity(int itemtype);该函数还没有开始编写代??
	if(itemType==2)//具备实体ID的话进行实体表插入， 更改的话对这里的值做个修??
	{
		//鍊间负2涓轰娇鐢ㄥ悗缁戝畾
		sprintf( szSql,"insert into Entity(ItemID,Durability,BindStatus) values(%d,%d,%d);",ItemID,Dur,Bind);
		iRet = dbo.ExceSQL(szSql);
		if( iRet < 0 )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);

 		}
		EntityID=dbo.LastInsertID();
		//瀵笲ag鍖呭仛涓�釜鎻掑叆鎿嶄綔锛�
		//鍖呯殑鎿嶄綔鏈夎繖涔堝嚑涓弬鏁癛oleID锛孋ellIndex顤旵ellType 锛孖temType  ItemID   EntityID Num
		//鍗曞厓鏍糏D鏄剧劧闇�鍋氫竴涓煡鎵剧┖鎿嶄綔

	}
	else
	{
			if(IsStack==0)
			{
				CellIndex=IfhascellIndex(roleID);
				if(num!=1||CellIndex==0)
				{
					return 0;
				}
			}
			sprintf( szSql, "select CdTime from Item where ItemID= %d;", ItemID);
			iRet = dbo.QuerySQL(szSql);
			if(iRet==0)
			{
				lic1.cdTime=dbo.GetIntField(0);
			}
	}
	lic1.bindStatus=Bind;
	lic1.celIndex=CellIndex;
	lic1.durability=Dur;
	lic1.EntityID=EntityID;
	lic1.num=num;
	lic1.ItemID=ItemID;
	lic.push_back(lic1);

	sprintf(szSql, "insert into Bag(RoleID,CellType,CellIndex,ItemType,ItemID,EntityID,Num)values(%d,%d,%d,%d,%d,%d,%d);",roleID,cellType,CellIndex,itemType,ItemID,EntityID,num);
	iRet=dbo.ExceSQL(szSql);

	if( iRet != 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return 0;
	}
				//浠诲姟璁℃暟
//	LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data what to get not found,szSql[%s] " , szSql);
	_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(roleID,lic1.ItemID);
	return CellIndex;
}

//得到,或者失去，数量为复数就是失去，金钱,返回1成功，返??失败
Int16 BagSvc::Getmoney(UInt32 RoleID,UInt32 num)
{
	Connection con;
	DBOperate dbo;
	Int32 iRet;
	char szSql[1024];
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	sprintf(szSql, "update RoleMoney set Money=Money+%d where RoleID=%d;",num,RoleID);

	iRet=dbo.ExceSQL(szSql);
		if( 1 == iRet )
		{
			return 0;
		}
		if( iRet < 0 )
		{
			return 0;
		}
		NotifyMoney(RoleID);
	return 1;

}

Int16 BagSvc::GetBindMoney(UInt32 RoleID,UInt32 num)//得到绑定钱，返回1成功，返??失败
{
		Connection con;
		DBOperate dbo;
		Int32 iRet;
		char szSql[1024];
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());

		sprintf(szSql, "update RoleMoney set BindMoney=BindMoney+%d where RoleID=%d;",num,RoleID);

			iRet=dbo.ExceSQL(szSql);
			if(  iRet!=0 )
			{
				return 0;
			}
		NotifyMoney(RoleID);
		return 1;
}


Int16 BagSvc::GetBindAndMoney(UInt32 RoleID,UInt32 num1,UInt32 num2)//寰楀埌缁戝畾閽卞拰涓嶇粦瀹氱殑
{
		Connection con;
		DBOperate dbo;
		Int32 iRet;
		char szSql[1024];
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());

		sprintf(szSql, "update RoleMoney set Money=Money+%d, BindMoney=BindMoney+%d where RoleID=%d;",num1,num2,RoleID);

		iRet=dbo.ExceSQL(szSql);
			if( 1 == iRet )
			{
				return 0;
			}
			if( iRet < 0 )
			{
				return 0;
			}
		NotifyMoney(RoleID);
		return 1;

}

//得到金币，金币充值，返回1成功，返??错误
Int16 BagSvc::GetGold(UInt32 RoleID,UInt32 num)
{
	Connection con;
	DBOperate dbo;
	Int32 iRet;
	char szSql[1024];
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	sprintf(szSql, "update RoleMoney set Gold=Gold+%d where RoleID=%d;",num,RoleID);

	iRet=dbo.ExceSQL(szSql);
		if( 1 == iRet )
		{
			return 0;
		}
		if( iRet < 0 )
		{
			return 0;
		}
	NotifyMoney(RoleID);
	return 1;

}

Int16 BagSvc::DigTheItem(UInt32 EntityID,UInt32 ItemID,UInt32 holdID)
{
	Connection con;
	DBOperate dbo;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	char szSql[1024];
	Int16 iRet=0;
	sprintf( szSql, "insert into EntityBonus(EntityID,HoleID,ItemID,HoleItemID) values(%d,%d,%d,0);",EntityID,ItemID,holdID);
	iRet=dbo.ExceSQL(szSql);
		if(iRet!=0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			return 0;
		}
		return 1;
}

Int16 BagSvc::AddstoneToItem(UInt32 EntityID,UInt32 holdID,UInt32 ItemID)
{
	Connection con;
	DBOperate dbo;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	char szSql[1024];
	Int16 iRet=0;
	sprintf( szSql, "update EntityBonus set HoleItemID=%d where EntityID=%d and HoldID=%d;",ItemID,EntityID,holdID);
	iRet=dbo.ExceSQL(szSql);
		if(iRet!=0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			return 0;
		}
		return 1;

}

UInt16 BagSvc::GetToBag(UInt32 RoleID,UInt16 cell,DBBagItem& item)
{
	Connection con;
	DBOperate dbo;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	char szSql[1024];
	Int16 iRet=0;
	ItemCell lic1;
	List<ItemCell> lic;

	item.CellIndex=cell;

	lic1.celIndex=item.CellIndex;
	lic1.ItemID=item.ItemID;
	lic1.EntityID=item.EntityID;
	lic1.num=item.NUM;
	if(lic1.EntityID!=0)
	{
		sprintf( szSql, "select Durability,BindStatus from Entity where EntityID=%d;",lic1.EntityID);
		iRet=dbo.QuerySQL(szSql);
		if(iRet==0)
		{
			lic1.durability=dbo.GetIntField(0);
			lic1.bindStatus=dbo.GetIntField(1);
		}
		sprintf( szSql, "select CdTime from Item where ItemID=%d;",lic1.ItemID);
		iRet=dbo.QuerySQL(szSql);
		if(iRet==0)
		{
			lic1.cdTime=dbo.GetIntField(0);
		}
	}
	LOG(LOG_ERROR,__FILE__,__LINE__,"CellIndex[%d]",item.CellIndex);
	sprintf( szSql, "insert into Bag (RoleID,CellType,CellIndex ,ItemType,ItemID,EntityID ,Num)\
		             values(%d,%d,%d,%d,%d,%d,%d);",\
		             RoleID,item.CellType,item.CellIndex,item.ItemType,item.ItemID,item.EntityID,item.NUM);
		iRet=dbo.ExceSQL(szSql);
		if(iRet!=0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			return 0;
		}
		_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(RoleID,lic1.ItemID);
		lic.push_back(lic1);
		NotifyBag(RoleID,lic);
		return 1;
}


//返回-1的错??0成功

Int16 BagSvc::DleteItemForEmail(UInt32 RoleID,UInt16 CellIndex,ItemList& lis,UInt32& EntityID)
{
	DBBagItem item;
	if(DeletefromBagitem(RoleID,CellIndex,item) !=1)
	{
		return -1;
	}
	lis.ItemID=item.ItemID;
	lis.num=item.NUM;
	EntityID=item.EntityID;
	return 0;

}
//返回0背包满了，返??1失败，返??成功
Int16 BagSvc::GetItemFromEmail(UInt32 RoleID,ItemList& lis,UInt32 EntityID)
{
		Connection con;
		DBOperate dbo;
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
		char szSql[1024];
		Int16 iRet=0;
		List<ItemCell> lic;
		ItemCell lic1;
		UInt32 ItemType=0;
		lic1.ItemID=lis.ItemID;
		lic1.num=lis.num;
		lic1.EntityID=EntityID;
		lic1.celIndex=IfhascellIndex(RoleID);
		if(lic1.celIndex==0)
		{
			return 0;
		}

			sprintf( szSql,"select ItemType,CdTime from Item where ItemID=%d;",lis.ItemID);;
			iRet=dbo.QuerySQL(szSql);
			if(iRet==0)
			{
				ItemType=dbo.GetIntField(0);
				lic1.cdTime=dbo.GetIntField(1);

			}
		sprintf( szSql, "insert into Bag(RoleID,CellIndex ,CellType,ItemType,ItemID,EntityID ,Num) value(%d,%d,1,%d,%d,%d,%d);",RoleID,lic1.celIndex,ItemType,lis.ItemID,EntityID,lis.num);
			iRet=dbo.ExceSQL(szSql);
			if(iRet!=0)
			{
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
				return 0;
			}
			if(EntityID!=0)
			{
				sprintf( szSql,"select Durability,BindStatus from Entity where EntityID=%d;",EntityID);;
				iRet=dbo.QuerySQL(szSql);
				if(iRet==0)
				{
					lic1.durability=dbo.GetIntField(0);
					lic1.bindStatus=dbo.GetIntField(1);
				}
			}
			_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(RoleID,lis.ItemID);
			lic.push_back(lic1);
			NotifyBag(RoleID,lic);
			return 1;
}


UInt16 BagSvc::DeletefromBag(UInt32 RoleID,List<UInt16>& cell,List<DBBagItem>& bagitem)
{
	List<UInt16>::iterator itor;
	DBBagItem item;
	Connection con;
	DBOperate dbo;
	ItemCell lic1;
	List<ItemCell> lic;

	List<ItemCell>::iterator it;

	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	char szTmp[128];
	char szCat[256];
	char szSql[1024];

	Int16 iRet=0;
	UInt32 count=cell.size();
	sprintf(szCat," ");
	for(itor=cell.begin();itor!=cell.end();itor++)
	{
		if(count==1)
		{
			sprintf( szTmp, "%d); ",*(itor));
		}
		else
		{
			sprintf( szTmp, "%d, ",*(itor));
		}
		strcat(szCat,szTmp);
		sprintf(szTmp," ");
		count--;
	}
	sprintf( szSql, "select RoleID,CellIndex ,CellType , ItemType ,ItemID,EntityID ,Num from Bag where RoleID=%d and CellIndex in(",RoleID );
	strcat(szSql,szCat);
		iRet=dbo.QuerySQL(szSql);
		if(iRet==0)
		{
			item.RoleID=dbo.GetIntField(0);
			lic1.celIndex=item.CellIndex=dbo.GetIntField(1);
			item.CellType=dbo.GetIntField(2);
			item.ItemType=dbo.GetIntField(3);
			lic1.ItemID=item.ItemID=dbo.GetIntField(4);
			item.EntityID=dbo.GetIntField(5);
			item.NUM=dbo.GetIntField(6);
			lic1.num=0;
			lic.push_back(lic1);
			bagitem.push_back(item);
		}
		else
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			return 0;
		}
	sprintf( szSql, "delete from Bag where RoleID=%d and CellIndex in(",RoleID );
	strcat(szSql,szCat);
		iRet=dbo.ExceSQL(szSql);
		if(iRet!=0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			return 0;
		}
		for(it=lic.begin();it!=lic.end();it++)
		{
			_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(RoleID,it->ItemID);
		}
		NotifyBag(RoleID,lic);
		return 1;
}

//?????: 0 ?????????????????1 ??? ??1 ???
Int16 BagSvc::TradeItem(UInt32 RoleID,List<UInt16>& cell,UInt32 ToRoleID,List<UInt16>& cell1 )
{
		List<DBBagItem> bagitem,bagitem2;
		DBBagItem item;
		List<DBBagItem>::iterator itor;
		List<UInt16>::iterator it;
		List<UInt16> lic,lic2;
		UInt16 size1=0,size2=0,size3=0,size4=0;
		size1=cell.size();
		size2=cell1.size();

		//??鱳??????п???
		//LOG(LOG_ERROR,__FILE__,__LINE__,"cell.size[%d]",size1);
		//LOG(LOG_ERROR,__FILE__,__LINE__,"cell1.size[%d]",size2);
		if(0==Ifhassomany(RoleID,lic,size2)||0==Ifhassomany(ToRoleID,lic2,size1))
		{
			return 0;
		}

		if(size1!=0)
		{
			if(DeletefromBag(RoleID, cell,bagitem)==0)
			{
				return -1;
			}
		}
		if(size2!=0)
		{
			if(DeletefromBag(ToRoleID,cell1,bagitem2)==0)
			{
				return -1;
			}
		}
		size3=bagitem.size();
		size4=bagitem2.size();
		if(size1!=size3||size2!=size4)
		{
			return -1;

		}
		else
		{
		   if(0 != lic2.size())
		   {
		       it=lic2.begin();
				for(itor=bagitem.begin();itor!=bagitem.end();itor++)
				{
				    //LOG(LOG_ERROR,__FILE__,__LINE__,"cellindex[%d]",*it);
				    //LOG(LOG_ERROR,__FILE__,__LINE__,"roleid[%d]-itemid[%d]",ToRoleID,itor->ItemID);
					GetToBag(ToRoleID,*it,*itor);

					it++;
				}
		   }

		   if(0 != lic.size())
		   {
		       it=lic.begin();
				for(itor=bagitem2.begin();itor!=bagitem2.end();itor++)
				{
					LOG(LOG_ERROR,__FILE__,__LINE__,"cellindex[%d]",*it);
					LOG(LOG_ERROR,__FILE__,__LINE__,"itemid[%d]--itemid[%d]",RoleID,itor->ItemID);
					GetToBag(RoleID,*it,*itor);

					it++;
				}
		   }
		}
		return 1;

}


UInt16 BagSvc::DeletefromBagitem(UInt32 RoleID,UInt16 cell,DBBagItem& item )
{
	Connection con;
	DBOperate dbo;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	char szSql[1024];
	List<ItemCell> lic;
	ItemCell lic1;
	Int16 iRet=0;
	sprintf( szSql, "select RoleID,CellIndex ,CellType , ItemType ,ItemID,EntityID ,Num from Bag where RoleID=%d and CellIndex=%d",RoleID,cell);
		iRet=dbo.QuerySQL(szSql);
		if(iRet==0)
		{
			item.RoleID=dbo.GetIntField(0);
			lic1.celIndex=item.CellIndex=dbo.GetIntField(1);
			item.CellType=dbo.GetIntField(2);
			item.ItemType=dbo.GetIntField(3);
			lic1.ItemID=item.ItemID=dbo.GetIntField(4);
			lic1.EntityID=item.EntityID=dbo.GetIntField(5);
			item.NUM=dbo.GetIntField(6);
		}
		else
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			return 0;
		}
	sprintf( szSql, "delete from Bag where RoleID=%d and CellIndex =%d;",RoleID ,cell);
	iRet=dbo.ExceSQL(szSql);
		if(iRet!=0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			return 0;
		}
		lic1.num=0;
		lic.push_back(lic1);
		_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(RoleID,lic1.ItemID);
		NotifyBag(RoleID,lic);
		return 1;
}

//丢弃物品 返回-1失败，返??成功 Items为物品清单，后面的参数为返回的物品信息，用于广播??
Int16 BagSvc::DropItems(UInt32 RoleID,List<ItemList>& items,List<ItemCell>& lic)
{
	ItemCell lic1;//用来存取的对??
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	Int16 iRet;
	int listSize = 0;
	List<ItemCell> licc;
	UInt32 entityID=0;
	UInt16 Num=0;
	UInt32 ItemID=0;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	char szTmp[128];
	char szEntity[300];
	char szCat[256];
	List<ItemCell>::iterator itor;

//	LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found " );

	if(SelectIfhasItems(RoleID,items,licc)==-1)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found " );
		return -1;
	}

	sprintf(szEntity," ");
	sprintf( szCat, "delete from Entity where EntityID in( ");
	sprintf( szSql, "delete from Bag where RoleID=%d and CellIndex in(",RoleID );
	listSize = licc.size();

	 if(listSize == 0)
	 {
	 	return 0;
	 }

	for( itor = licc.begin(); itor != licc.end(); itor++ )
	{

		lic1.bindStatus=itor->bindStatus;
		lic1.num=itor->num;
		lic1.celIndex=itor->celIndex;
		lic1.EntityID=itor->EntityID;
		lic1.ItemID=itor->ItemID;
		lic1.durability=itor->durability;

		if(lic1.EntityID!=0)
		{
			if(listSize == 1)
			{
				sprintf( szEntity, "%d)",lic1.EntityID);
			 	strcat( szCat, szEntity );

				sprintf( szTmp, "%d)",lic1.celIndex);
				strcat( szSql, szTmp );
			}
			else
			{
				sprintf( szEntity, "%d,",lic1.EntityID);
				strcat( szCat, szEntity);

				sprintf( szTmp, "%d,",lic1.celIndex);
				strcat( szSql, szTmp );
			}

			lic1.num=0;
			lic.push_back(lic1);

			}
		else
		{
				if(lic1.ItemID==itor->ItemID)
				{
					if(listSize == 1)
					{

						sprintf( szTmp, "%d)",lic1.celIndex);
						strcat( szSql, szTmp );
					}
					else
					{

						sprintf( szTmp, "%d,",lic1.celIndex);
						strcat( szSql, szTmp );
					}

						lic1.num=0;

						lic.push_back(lic1);
				}
				else
				{
					iRet=dropItemIfCell(RoleID,lic1);
					if(iRet==-1)
					{
						LOG(LOG_ERROR,__FILE__,__LINE__,"erro!,szSql[%s] " , szSql);
					}
					lic.push_back(lic1);
				}
		}

		listSize--;

	 }

	iRet=dbo.ExceSQL(szSql);
	if(iRet!=0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
		return -1;
	}

	if(0 != strcmp(szEntity," "))
	{
		iRet=dbo.ExceSQL(szCat);
		if(iRet!=0)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szCat);
		}
	}


	return 0;

}
//瀵逛簬鍙互鍫嗗彔鐨勭墿鍝佽繘琛屼涪寮冿紝鏈夐儴鍒嗕涪寮冪殑鍙兘
Int32 BagSvc::dropItemIfCell(UInt32 RoleID,ItemCell & items )
{
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	Int16 iRet;
	UInt32 Num=0;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

		sprintf( szSql, "select Num from Bag where RoleID =%d and CellIndex=%d;",RoleID ,items.celIndex);
				iRet=dbo.QuerySQL(szSql);
				if(iRet==0)
				{
					Num=dbo.GetIntField(0);
				}
				else
				{
					return -1;
				}
				if(items.num==Num)
				{
					sprintf( szSql, "delete from Bag where RoleID =%d and CellIndex=%d;",RoleID ,items.celIndex);
				}
				else if(items.num<Num)
				{
					sprintf( szSql, "update Bag set Num=Num-%d where RoleID =%d and CellIndex=%d;",Num-items.num,RoleID ,items.celIndex);
				}
				iRet=dbo.ExceSQL(szSql);
				if(iRet!=0)
				{
					LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
					return -1;
				}
				items.num=Num-items.num;
				return 0;
}

//澶卞幓鐗╁搧,鍗曚釜
UInt16 BagSvc::DropItem(UInt32 RoleID,UInt32 ItemID,List<ItemCell>& lic)
{
	ItemCell lic1;//用来存取的对??
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	Int16 iRet;
	UInt32 entityID=0;
	UInt16 CellIndex=0;
	UInt16 Num;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	sprintf( szSql, "select CellIndex,EntityID,Num from Bag where RoleID= %d and ItemID=%d order by CellIndex;", RoleID,ItemID);

		iRet=dbo.QuerySQL(szSql);
		if( 1 == iRet )
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
			return 0;
		}
 		if(0==iRet)
 		{

 				while(dbo.HasRowData())
				{
					CellIndex=dbo.GetIntField(0);
					entityID=dbo.GetIntField(1);
					Num=dbo.GetIntField(2);
					break;
				}

 		}
 		if(entityID==0)//??表示是不在实体表中有记录??只是需要删除Bag就可??
 		{
 			if(Num==1)
 			{
 				sprintf( szSql, "delete from Bag where RoleID=%d and CellIndex=%d;", RoleID,CellIndex);
			}
			else
			{
				sprintf( szSql, "update Bag set Num=Num-1 where RoleID =%d and CellIndex=%d;",RoleID ,CellIndex);
			}
 			iRet=dbo.ExceSQL(szSql);
 			if( 1 == iRet )
			{
				return 0;
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			}
 		}
 		else
 		{//鍘讳簡瀹炰綋琛ㄣ�鍚屾椂鍘绘帀BAG
 			sprintf( szSql, "select Durability,BindStatus from Entity where EntityID= %d;", entityID);
					iRet=dbo.QuerySQL(szSql);
					if( 1 == iRet )
					{
						LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
						return 0;

					}
					if(dbo.RowNum()==1)
					{
						lic1.durability=dbo.GetIntField(0);
						lic1.bindStatus=dbo.GetIntField(1);
					}
 			sprintf( szSql, "delete from Entity where EntityID=%d;",entityID);
 			iRet=dbo.ExceSQL(szSql);
 			if( 1 == iRet )
			{
				return 0;
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);

			}
			sprintf( szSql, "delete from Bag where RoleID=%d and CellIndex=%d;", RoleID,CellIndex);
 			iRet=dbo.ExceSQL(szSql);
 			if( 1 == iRet )
			{
				return 0;

			}
 		}

		lic1.EntityID=entityID;
 		lic1.celIndex=CellIndex;
 		lic1.num=0;
 		lic1.ItemID=ItemID;
 		lic.push_back(lic1);
 		_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(RoleID,lic1.ItemID);
 		return 1;



}

Int16 BagSvc::DropMoneyOnly(UInt32 RoleID,UInt32 num)//娌℃湁缁戝畾鐨勯噾閽辩殑涓㈠け
{
			  Connection con;
				DBOperate dbo;
				Int32 iRet;
				UInt32 BindMoney=0;
				char szSql[1024];
				con = _cp->GetConnection();
				dbo.SetHandle(con.GetHandle());
				UInt32 money;
				money=selectmoney(RoleID,BindMoney);
				if(money <num)
				{
				  return -1;
				}
				else
				{
					money=money-num;
				}
				sprintf(szSql, "update RoleMoney set Money=%d where RoleID=%d;",money,RoleID);
				iRet=dbo.ExceSQL(szSql);
				if( iRet !=0 )
				{
					return 0;
				}
			NotifyMoney(RoleID);
			return 1;
}

//失去金钱,返回-1金钱不足，返??失败，返??成功，绑定钱的交易，不够去非绑定??
Int16 BagSvc::Dropmoney(UInt32 RoleID,UInt32 num)
{
		Connection con;
		DBOperate dbo;
		Int32 iRet;
		UInt32 BindMoney=0;
		char szSql[1024];
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
		UInt32 money;
		money=selectmoney(RoleID,BindMoney);
		if(money+BindMoney<num)
		{

			LOG(LOG_ERROR,__FILE__,__LINE__,"has not so much money! ");
			return -1;
		}
		if(BindMoney>=num)
		{
			BindMoney=BindMoney-num;
		}
		else
		{
			money= money - (num - BindMoney);
			BindMoney=0;

		}
		sprintf(szSql, "update RoleMoney set Money=%d,BindMoney=%d where RoleID=%d;",money,BindMoney,RoleID);
		LOG(LOG_ERROR,__FILE__,__LINE__,"szSql[%s]",szSql);
		iRet=dbo.ExceSQL(szSql);
		if(  iRet != 0 )
		{

			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			return 0;
		}
		NotifyMoney(RoleID);

	return 1;

}
//失去金币，返??1不足，返??错误，返??成功
Int16 BagSvc::DropGold(UInt32 RoleID,UInt32 num)
{
		Connection con;
		DBOperate dbo;
		Int32 iRet;
		UInt32 Gold=0;
		char szSql[1024];
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
		Gold=selectGold(RoleID);
		if(Gold<num)
		{
			return -1;
		}

		sprintf(szSql, "update RoleMoney set Gold=Gold-%d where RoleID=%d;",num,RoleID);
		iRet=dbo.ExceSQL(szSql);
		if( iRet !=0 )
		{
			return 0;
		}

		NotifyMoney(RoleID);
	return 1;

}


UInt32 BagSvc::SelectMixEquipMoney(list<EquipItem>& Entity)
{
	Connection con;
	DBOperate dbo;
	Int32 iRet=0;
	UInt32 totalMoney=0;
	char szSql[1024];
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());



	char sztmp[128];

	List<EquipItem>::iterator itor;
	UInt32 Count=Entity.size();

	sprintf(szSql, "select Entity.Durability,Entity.ItemID ,Item.Durability,Item.MinLevel,Item.EquipPos from Entity left join Item \
									on Entity.ItemID=Item.ItemID where \
								EntityID in(");
	for(itor=Entity.begin();itor!=Entity.end();itor++)
	{
			if(Count!=1)
				sprintf(sztmp, "%d,",itor->EntityID);
			else
				sprintf(sztmp, "%d);",itor->EntityID);
			strcat(szSql,sztmp);
				Count--;
	}

	iRet=dbo.QuerySQL(szSql);
	if(iRet!=0)
	{

		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or erro,szSql[%s] " , szSql);
		return 0;
	}
	while(dbo.HasRowData())
	{


		totalMoney=totalMoney+(UInt32)((dbo.GetIntField(2)-dbo.GetIntField(0))*(50+8*dbo.GetIntField(3))*GetTheNum(dbo.GetIntField(4)));

		dbo.NextRow();
	}

	return totalMoney;
}

Int32 BagSvc::UpdateMixEquip(list<EquipItem>& Entity)
{
	Connection con;
	DBOperate dbo;
	UInt32 iRet;
	UInt32 Gold=0;
	char szSql[1024];
	char sztmp[128];
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	UInt32 Count=0;
	list<EquipItem>::iterator itor;

	sprintf(szSql, "update  Entity set Durability=(select Durability from Item where ItemID=Entity.ItemID) where EntityID in(");
	Count=Entity.size();

		for(itor=Entity.begin();itor!=Entity.end();itor++)
		{
			if(Count!=1)
				sprintf(sztmp, "%d,",itor->EntityID);
			else
				sprintf(sztmp, "%d);",itor->EntityID);
			strcat(szSql,sztmp);
				Count--;
		}

	iRet=dbo.ExceSQL(szSql);
	if(iRet!=0)
	{

		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or erro,szSql[%s] " , szSql);
		return -1;
	}
	return 0;

}

//鏌ヨ鑳屽寘鏄惁鍏锋湁杩欎簺鐗╁搧锛屽皯涓�欢鍒欒繑鍥�1锛屽畬鍏ㄥ垯杩斿洖0,ItemList涓殑鐗╁搧ItemID涓嶈兘閲嶅,骞朵笖杩斿洖杩欎簺鎵�湁鐗╁搧鐨勪俊鎭疘temCell
Int32 BagSvc::SelectIfhasItems(UInt32 RoleID,List<ItemList>& items,List<ItemCell>& lic)
{
	ItemCell lic1;
	List<ItemList>::iterator itor;
	Connection con;
	DBOperate dbo;
	UInt32 iRet;
	char szSql[1024];
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	Int32 i=0;
	UInt32 EntityID=0;
	UInt32 num=0,count=0;
	//		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found " );
	for(itor=items.begin();itor!=items.end();itor++)
	{
			num=0;
			sprintf(szSql, "select CellIndex, EntityID,Num from Bag where RoleID =%d and ItemID = %d;",RoleID,itor->ItemID);
			iRet=dbo.QuerySQL(szSql);
			if( 1 == iRet )
			{
				i=-1;
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
				break;
			}
			if( iRet == 0 )
			{
					if(dbo.HasRowData())
					{
						EntityID=dbo.GetIntField(1);
					}
					if(EntityID!=0)
					{
						num=dbo.RowNum();
						if(num>=(itor->num))
						{
							for(int j=0;j<(itor->num);j++)
							{
								if(dbo.HasRowData())
								{
									lic1.ItemID=itor->ItemID;
									lic1.EntityID=dbo.GetIntField(1);
									lic1.num=1;
									lic1.celIndex=dbo.GetIntField(0);

									lic.push_back(lic1);

									dbo.NextRow();
								}
							}
						}
						else
						{
						LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found " );
							i=-1;
							break;
						}
					}
					else
					{
						while(dbo.HasRowData())
						{
							if(num<itor->num)
							{
								count=dbo.GetIntField(2);
								lic1.ItemID=itor->ItemID;
								lic1.celIndex=dbo.GetIntField(0);
								lic1.num=count;
								num=num+count;
								if(num>(itor->num))
								{
									lic1.num=count-(num-(itor->num));
								}
								lic.push_back(lic1);
								dbo.NextRow();
							}
							else
							{
								break;
							}

							//鍙互鍫嗗彔鐨勫叿鏈夊崟鍏冧笉瀹屽叏鏄殑闂
						}
						if((itor->num)>num)
						{
							i=-1;
							break;
						}
						else
						{
							i = num;
						}
				  }

			}
			if(iRet<0)
			{
				i=-1;
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
				break;
			}


	}
//		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found " );
	return i;

}

//鏌ヨ瑙掕壊鐨勯噾閽憋紝杩斿洖-1澶辫触锛屽叾浠栦负杩斿洖閲戦挶鏁伴噺
UInt32 BagSvc::selectmoney(UInt32 RoleID,UInt32& BindMoney)
{
	Connection con;
	DBOperate dbo;
	UInt32 iRet;
	char szSql[1024];
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	UInt32 money;
	sprintf(szSql, "select Money,BindMoney from RoleMoney where RoleID=%d;",RoleID);
	iRet=dbo.QuerySQL(szSql);
	if(0==iRet)
	{
		money=dbo.GetIntField(0);
		BindMoney=dbo.GetIntField(1);
	}
	if(iRet==1)
	{
		return 0;
	}
	if(iRet<0)
	{
		return 0;
	}
	return money;
}


UInt32 BagSvc::selectGold(UInt32 RoleID)//鏌ヨ閲戝竵
{
		Connection con;
		DBOperate dbo;
		UInt32 iRet;
		char szSql[1024];
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
		UInt32 Gold;
		sprintf(szSql, "select Gold from RoleMoney where RoleID=%d;",RoleID);
		iRet=dbo.QuerySQL(szSql);
		if(0==iRet)
		{
			Gold=dbo.GetIntField(0);
		}
		if(iRet==1)
		{
			return 0;
		}
		if(iRet<0)
		{
			return 0;
		}
		return Gold;
}

//返回0成功，返??失败,返回-1背包空间不足
Int32 BagSvc::RoleGetItem(UInt32 roleID,List<ItemList>& items)
{

		UInt16 count;
		UInt32 iRet;
		UInt32 num=0;
		List<ItemCell> lic;
		UInt16 f;
		List<ItemList>::iterator itor;
		num=GetEmtycellNum(roleID);
		if(items.size()>num)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__," not so many cell found,%d ",items.size() );
			return -1;
		}
		for(itor=items.begin();itor!=items.end();itor++)
		{

//				LOG(LOG_ERROR,__FILE__,__LINE__,"any worry with it	%d ", itor->ItemID);
				f=GetItem(roleID,itor->ItemID,lic,itor->num);
				if(f==0)
				{
					LOG(LOG_ERROR,__FILE__,__LINE__,"any worry with it  %d ", itor->ItemID);
				//	return 1;
				}
				LOG(LOG_ERROR,__FILE__,__LINE__,"GetItem[%d] ", itor->ItemID);
		}
		LOG(LOG_ERROR,__FILE__,__LINE__,"NotifyBag---");
		NotifyBag(roleID,lic);
		return 0;
}
Int32 BagSvc::RoleGetItem(UInt32 roleID,List<UInt32>& items)//返回0成功，返??失败
{
	UInt16 count;
		UInt32 iRet;
		UInt32 num=0;
		List<ItemCell> lic;
		UInt16 f;
		List<UInt32>::iterator itor;
		num=GetEmtycellNum(roleID);
		if(items.size()>num)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__," not so many cell found,%d ",items.size() );
			return -1;
		}
		for(itor=items.begin();itor!=items.end();itor++)
		{
				f=GetItem(roleID,*itor,lic,1);
				if(f==0)
				{
					LOG(LOG_ERROR,__FILE__,__LINE__,"any worry with it   " );
					return 1;
				}
		}
		NotifyBag(roleID,lic);
		return 0;

}
//返回0成功，返??失败
Int32 BagSvc::RoleDropItem(UInt32 roleID,List<ItemList>& items)
{
	List<ItemCell> lic;
	List<ItemList>::iterator itor;
	Int16 f;
	f=DropItems(roleID,items,lic);
	if(f==-1)
	{
		return 1;
	}
	else
	{
		for(itor=items.begin();itor!=items.end();itor++)
		{
			_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(roleID,itor->ItemID);
		}
		NotifyBag(roleID,lic);
		return 0;
	}
}

//判断是否有一个单??有的话，返回cellIndex,没有的话，返??
UInt16 BagSvc::IfhascellIndex(UInt32 RoleID)
{
	UInt16 CellIndex=0;
	Connection con;
	DBOperate dbo;
	UInt16 count;
	Int32 iRet;
	UInt16 count_index=0;
	UInt16 count_int=1;
	char szSql[1024];
	UInt32 TOPCELL_NUM=GetRoleCellNum(RoleID);
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	sprintf( szSql, " select CellType,CellIndex,ItemID,Num from Bag where RoleID=%d;",RoleID);
	iRet=dbo.QuerySQL(szSql);
	if(iRet==1)
	{
		CellIndex=1;

	}
	count=dbo.RowNum();
	if(count>=TOPCELL_NUM)
	{

		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
		return 0;
	}
		while(dbo.HasRowData())
		{
			count_index=dbo.GetIntField(1);

			if(count_index>count_int)
				{
					CellIndex=count_int;
					break;
				}
			//记录集下一条记??
				dbo.NextRow();
				count_int=count_index+1;
		}
		if(count_index<TOPCELL_NUM)
		{
			if(CellIndex==0)
			CellIndex=count_index+1;
		}
		return CellIndex;

}
//判断是否含有多个cellIndex，多个参数，一个List用作存储单元格数??返回的值为0，就是不具有
//要求的数量，返回值为1就是具有要求的数量，参数数量就是要求的数??
UInt16 BagSvc::Ifhassomany(UInt32 RoleID,List<UInt16>& Cell,UInt16 num)
{
	UInt16 CellIndex=0;
	Connection con;
	DBOperate dbo;
	UInt16 count;
	UInt32 iRet;
	UInt16 count_index=0;
	UInt16 count_int=0;
	char szSql[1024];
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	UInt32 TOPCELL_NUM=GetRoleCellNum(RoleID);

	if(num == 0)
	{
	  return 1;
	}
	sprintf( szSql, " select CellType,CellIndex,ItemID,Num from Bag where RoleID=%d order by CellIndex asc;",RoleID);
	iRet=dbo.QuerySQL(szSql);
	count=dbo.RowNum();
	if(num+count>TOPCELL_NUM)
	{//不满足num数量的空??
		return 0;
	}
		while(dbo.HasRowData())
		{
			count_index=dbo.GetIntField(1);

			while(count_index>count_int+1)
			{
				count_int=count_int+1;
				Cell.push_back(count_int);
				num--;
				//count_int=count_int+1;
				if(num==0)
				break;
				}
			//记录集下一条记??
			count_int=count_index;
				if(num==0)
					break;
				dbo.NextRow();


		}
		while(count_int<TOPCELL_NUM&&num!=0)
		{
				count_int=count_int+1;
				Cell.push_back(count_int);
				num--;
			//	count_int=count_int+1;

		}
		if(num==0)
		return 1;
		else
		return 0;
}


UInt32 BagSvc::GetRoleCellNum(UInt32 roleID)
{
	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);
	UInt32 num=pRole->TopCellNum();
	if(num==0)
	num=25;
//	LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data what to get not found,szSql[%d] " , num);
	return num;
}

UInt32 BagSvc::CompiteUse(UInt32 RoleID,UInt32 itemID,List<ItemCell>& lic)
{

		Connection con;
		DBOperate dbo;
		UInt32 iRet;
		char szSql[1024];
		List<ItemList>::iterator itor;
		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
		Int32 i=0;
		UInt32 itemID1=0;
    List<ItemList> items;
    ItemList item;
	LOG(LOG_ERROR,__FILE__,__LINE__,"itemid[%d]",itemID);
	sprintf( szSql, "select ItemID,PartItemID,Num from ItemComposite where ItemID<>%d and ItemID=(select ItemID from ItemComposite where PartItemID=%d);",itemID,itemID);
	iRet=dbo.QuerySQL(szSql);
	if(iRet < 0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"DB op error ! szSql[%s]",szSql);
		return 0;
	}
	if(iRet == 1)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Query SQL failed ! szSql[%s]",szSql);
		return 0;
	}
	if(iRet==0)
	{
			itemID1=dbo.GetIntField(0);
			while(dbo.HasRowData())
			{
				item.ItemID=dbo.GetIntField(1);
				item.num=dbo.GetIntField(2);
				items.push_back(item);
				dbo.NextRow();
			}
	}
	 i=DropItems(RoleID,items,lic);
	if(i==-1)
	{
	return 0;
	}
	else
	{
				NotifyBag(RoleID,lic);
				for(itor=items.begin();itor!=items.end();itor++)
				{
						_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(RoleID,itor->ItemID);
				}
	}
	return itemID1;
}





Int16 BagSvc::UseGiftBag(UInt32 RoleID, UInt32 itemID,List<ItemList>& lis)//浣跨敤绀煎寘
{
		Connection con;
		DBOperate dbo;
		UInt32 iRet;
		char szSql[1024];

		con = _cp->GetConnection();
		dbo.SetHandle(con.GetHandle());
    ItemList item;

		sprintf( szSql, "select PartItemID,Num from GiftItemBag where ItemID=%d;",itemID);
		iRet=dbo.QuerySQL(szSql);
		if(iRet==0)
		{

				while(dbo.HasRowData())
				{
					item.ItemID=dbo.GetIntField(0);
					item.num=dbo.GetIntField(1);
					lis.push_back(item);
					dbo.NextRow();
				}
		}
		else
		{
			return -1;
		}

	return 0;
}

Int16 BagSvc::UseVIPItem(UInt32 RoleID,UInt32 itemID,Byte vip)//VIP鍗＄殑浣跨敤
{
	Connection con;
	DBOperate dbo;
	Int32 iRet;
	char szSql[1024];
	ItemMapTravelPoint point;
	UInt32 NumDate=0;
	Byte type=0;
	UInt32 viplev=0;
	DateTime datetime,endtime;
	string maxendTime;

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(RoleID);

	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	sprintf( szSql, "select BonusAttr,BonusValue from ItemBonus \
										where ItemID=%d;", itemID);
		//绫诲瀷30琛ㄧずVIP
		iRet=dbo.QuerySQL(szSql);
		if(iRet!=0)
		{
				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL same erro or not found[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				return -1;
		}
		else
		{
			 if(dbo.GetIntField(0)==30)
			 {
			 		NumDate=dbo.GetIntField(1);
			 }
		}

		if(vip==0)
		{	//棣栨

			pRole->IsVIP(1);
			sprintf( szSql, "insert into VIP(RoleID,VIPLev,VIPExp) values(%d,1,0);",RoleID);
			viplev = 1;
		}
		else if(vip>10)
		{
			//已经是vip,或者vip过期??
			viplev = vip-10;
			pRole->IsVIP(vip-10);
			sprintf( szSql, "update VIP set VIPLev=%d where RoleID=%d;",vip-10,RoleID);
		}
		else if(vip<10)
		{
		//褰撳墠灏辨槸vip
			viplev = vip;
		}
		if(vip>10||vip==0)
		{
			iRet=dbo.ExceSQL(szSql);
			if(iRet!=0)
			{

				LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL same erro or not found[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
				return -1;
			}
		}

		if(NumDate==90) {type=2;}
		else if(NumDate==180){type=3;}
				 else{type=1;}
		if(vip<10&&vip!=0)
		{
			//鍘熸湰灏辨槸vip
				sprintf( szSql, "select ifnull(Max(EndTime),now()) from VIPTime where RoleID=%d;",RoleID);
				iRet=dbo.QuerySQL(szSql);
				if(iRet<0)
				{
					LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL same erro or not found[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					return -1;
				}
				else if(iRet==0)
				{
					maxendTime=dbo.GetStringField(0);
					datetime.SetDate(maxendTime);
				}
				else if(iRet==1)
				{
					datetime=datetime.Now();
				}

				endtime=datetime;
				endtime=endtime.AddDays((double)(NumDate+1));
			sprintf( szSql, "insert into VIPTime(BeginTime,EndTime,RoleID,Type) \
												values ('%d-%d-%d','%d-%d-%d',%d,%d);",datetime.year,datetime.month,datetime.day
																															,endtime.year,endtime.month,endtime.day,RoleID,type);
		}
		else
		{
				endtime=endtime.Now();

				LOG(LOG_ERROR,__FILE__,__LINE__,"%d,%d,%d",endtime.day,endtime.year,endtime.month);
				endtime=endtime.AddDays((double)(NumDate+1));

				LOG(LOG_ERROR,__FILE__,__LINE__,"%d,%d,%d",endtime.day,endtime.year,endtime.second);
			sprintf( szSql, "insert into VIPTime(BeginTime,EndTime,RoleID,Type)	values (now(),'%d-%d-%d',%d,%d);",
																						endtime.year,endtime.month,endtime.day,RoleID,type);
		}

			iRet=dbo.ExceSQL(szSql);
		if(iRet!=0)
		{

			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL same erro or not found[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			return -1;
		}
	DataBuffer	serbuffer(1024);
	Packet p(&serbuffer);
	Serializer s( &serbuffer );
	serbuffer.Reset();

	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 309;
	p.UniqID = 251;
	p.Direction = DIRECT_S_C_REQ;
	p.PackHeader();
	s << viplev;
	p.UpdatePacketLength();

	List<UInt32> lrid;
	lrid.push_back(RoleID);
	if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}

	return 0;
}

//获得空的单元的数量，返回0没有了，其他返回空的单元的数??
Int32 BagSvc::GetEmtycellNum(UInt32 roleID)
{
	UInt16 CellIndex=0;
	Connection con;
	DBOperate dbo;
	UInt16 count=0;
	UInt32 iRet;
	char szSql[1024];
	UInt32 TOPCELL_NUM=GetRoleCellNum(roleID);
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	sprintf( szSql, " select CellType,CellIndex,ItemID,Num from Bag where RoleID=%d order by CellIndex asc;",roleID);
	iRet=dbo.QuerySQL(szSql);
	if(iRet==0)
	{
			count=dbo.RowNum();
			if(count>=TOPCELL_NUM)
			{
				return 0;
			}
		return TOPCELL_NUM-count;
	}
	else
	{
		if(iRet==1)
		{
		return TOPCELL_NUM;
		}
	}

	return 0;
}
//返回信息0成功??1不存??其他失败

Int32 BagSvc::RoleSelectCell(UInt32 roleID,UInt16 cellIndex,ItemCell& lic)
{

	char szSql[1024];
	Connection con;
	DBOperate dbo;
	Int16 iRet = 0;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	sprintf( szSql, "select Bag.CellIndex,Bag.ItemID,Bag.EntityID, \
									Bag.Num,Entity.Durability,Entity.BindStatus from Bag left \
									join Entity on Bag.EntityID=Entity.EntityID where \
									RoleID= %d and CellIndex=%d;", roleID,cellIndex);
	iRet=dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
		return -1;
		//璇ヤ綅缃病鏈夌墿鍝侊紱
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		return 1;
 	}

 	if(0==iRet)
	{
		while(dbo.HasRowData())
		{
			lic.celIndex=dbo.GetIntField(0);
			lic.ItemID=dbo.GetIntField(1);
			lic.EntityID=dbo.GetIntField(2);
			lic.num=dbo.GetIntField(3);
			lic.durability = dbo.GetIntField(4);
			lic.bindStatus = dbo.GetIntField(5);
		//记录集下一条记??
			dbo.NextRow();
		}
	}
	if(lic.EntityID!=0)
	{
		lic.cdTime=0;

	}

	else
	{
		sprintf( szSql, "select CdTime from Item where ItemID= %d;", lic.ItemID);
		iRet = dbo.QuerySQL(szSql);
		if(iRet==0)
		{
			lic.cdTime=dbo.GetIntField(0);
		}
	}
	return 0;

}
Int32 BagSvc::RoleSelectCellList(UInt32 roleID,List<UInt16>& cellIndex,List<ItemCell>& lic)
{
	char szSql[1024];
	char szCat[256];
	Connection con,conSub;
	DBOperate dbo,dboSub;
	Int16 iRet = 0;
	ItemCell lic1;
	UInt32 count=cellIndex.size();
	List<UInt16>::iterator itor;
	conSub = _cp->GetConnection();
	dboSub.SetHandle(conSub.GetHandle());

	con = _cp->GetConnection();
	dbo.SetHandle(conSub.GetHandle());
	sprintf( szSql, "select Bag.CellIndex,Bag.ItemID,Bag.EntityID,Bag.Num, \
											Entity.Durability,Entity.BindStatus from Bag left join \
											Entity on Bag.EntityID=Entity.EntityID where \
											RoleID= %d and CellIndex in(", roleID);


	for(itor=cellIndex.begin();itor!=cellIndex.end();itor++)
	{
		if(count!=1)
		{
			sprintf( szCat, "%d,");
		}
		else
		{
			sprintf( szCat, "%d);");
		}
		strcat(szSql,szCat);
		count--;
	}
	iRet=dbo.QuerySQL(szSql);


	if( 1 == iRet )
	{
		return -1;
		//璇ヤ綅缃病鏈夌墿鍝侊紱
	}
	if( iRet < 0 )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
 	}

 	if(0==iRet)
	{
		while(dbo.HasRowData())
		{
					lic1.celIndex=dbo.GetIntField(0);
					lic1.ItemID=dbo.GetIntField(1);
					lic1.EntityID=dbo.GetIntField(2);
					lic1.num=dbo.GetIntField(3);
					lic1.durability = dbo.GetIntField(4);
					lic1.bindStatus = dbo.GetIntField(5);
					if(lic1.EntityID!=0)
					{
							lic1.cdTime=0;

					}

					else
					{
							sprintf( szSql, "select CdTime from Item where ItemID= %d;", lic1.ItemID);
							iRet = dboSub.QuerySQL(szSql);
							if(iRet==0)
							{
								lic1.cdTime=dboSub.GetIntField(0);
							}
					}
					lic.push_back(lic1);
		//记录集下一条记??
			dbo.NextRow();
		}
	}

	return 0;
}

//閾跺竵鍗＄殑浣跨敤
//返回 0 成功 ??0 失败
Int32 BagSvc::UseSilverCoinCard(UInt32 roleID)
{
  UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	Int16 iRet = 0;

	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	sprintf(szSql,"update RoleMoney set Money= Money + 1000\
									where RoleID = %d;",roleID);
	iRet = dbo.ExceSQL(szSql);
	LOG(LOG_DEBUG,__FILE__,__LINE__,"iRet[%d]",iRet);
	LOG(LOG_DEBUG,__FILE__,__LINE__,"szSql[%s]",szSql);

	if(0 != iRet)
	{
	  return -1;
	}

	return 0;

}

Int32 BagSvc::UseItemToHpMp(UInt32 RoleID,UInt32 itemID,UInt32 CdTime,RolePtr& pRole)
{
    assert(pRole);

	UInt32 t=time(NULL);
	UInt32 lev=0;
	lev=2;
	Connection con;
	UInt32 HpAdd=0,MpAdd=0;
	DBOperate dbo;
	Int16 iRet = 0;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	char szSql[1024];

	sprintf( szSql, "select BonusAttr,BonusValue from ItemBonus where ItemID=%d;", itemID);
	iRet=dbo.QuerySQL(szSql);
	if(iRet!=0)
	{
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL same erro or not found[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
	}
	else
	{
		while(dbo.HasRowData())
		{
			if(dbo.GetIntField(0)==20)
			{
				HpAdd=dbo.GetIntField(1);
			}
			else if(dbo.GetIntField(0)==21)
			{
				MpAdd=dbo.GetIntField(1);
			}
			dbo.NextRow();
		}
	}

	if(HpAdd!=0&&MpAdd==0)
	{
		if(t>=pRole->HpCDflag)
		{
			pRole->AddHp(HpAdd);
			pRole->CdTimetoTheFalg(1,CdTime);
//			_mainSvc->GetCoreData()->AddHp(RoleID,HpAdd);
//			_mainSvc->GetCoreData()->CdTimetoTheFalg(RoleID,1,CdTime);
		}
		else
		{
			return -1;
		}
	}
	else if(HpAdd==0&&MpAdd!=0)
	{
		if(t>=pRole->MpCDflag)
		{
			pRole->AddMp(MpAdd);
			pRole->CdTimetoTheFalg(2,CdTime);
//			_mainSvc->GetCoreData()->AddMp(RoleID,MpAdd);
//			_mainSvc->GetCoreData()->CdTimetoTheFalg(RoleID,2,CdTime);
		}
		else
		{
			return -1;
		}
	}
	else if(HpAdd!=0&&MpAdd!=0)
	{
		if(t>=pRole->HpMpflag)
		{
			pRole->AddHp(HpAdd);
			pRole->AddMp(MpAdd);
			pRole->CdTimetoTheFalg(3,CdTime);
//			_mainSvc->GetCoreData()->AddHp(RoleID,HpAdd);
//			_mainSvc->GetCoreData()->AddMp(RoleID,MpAdd);
//			_mainSvc->GetCoreData()->CdTimetoTheFalg(RoleID,3,CdTime);
		}
		else
		{
			return -1;
		}
	}
	return 0;
}
//鑰愪箙绯绘暟
float BagSvc::GetTheNum(UInt32 euippos)
{
	float n;
	switch(euippos%100)
	{
		case 1: n=0.5;break;
		case 3:	n=0.6;break;
		case 4: n=0.3;break;
		case 7: n=0.3;break;
		case 10: n=0.4;break;
		case 5: n=1.0;break;
		case 8: n=1.2;break;
		default: n=1.0;break;
	}
	return n;
}

void BagSvc::RoleUseToughRune(int roleID, int itemId)
{
	NewTimer **pRoleTimers = _roleRuneTimerMap[roleID];
	NewTimer* pTimer = pRoleTimers[ToughRuneTimer];

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	if (pTimer != 0)
	{
		pTimer->Type(NewTimer::OnceTimer);
		pTimer->Interval(2*5); //2灏忔椂
		int Change = 0;
		if (pTimer->IsRunning())
		{
			RuneTimerCallBackParam param = *(RuneTimerCallBackParam*)(pTimer->GetCallbackFunParam());
			Change = ToughRuneHpChange[itemId-6003]-param.valueChange;
			param.roleID = roleID;
			param.valueChange= ToughRuneHpChange[itemId-6003];
			pTimer->SetCallbackFun(TimerDecToughness, _mainSvc,
						&param, sizeof(param));
			pTimer->reStart();
		}
		else
		{
			Change = ToughRuneHpChange[itemId-6003];
			RuneTimerCallBackParam param;
			param.roleID = roleID;
			param.valueChange= ToughRuneHpChange[itemId-6003];
			pTimer->SetCallbackFun(TimerDecToughness, _mainSvc,
						&param, sizeof(param));
			pTimer->start(&(_mainSvc->_tm));
		}
		pRole->AddMaxHpBonus(Change);
		_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				1,
				pRole->MaxHpBonus());

	}
}

void BagSvc::RoleUseStoneRune(int roleID, int itemId)
{
	NewTimer **pRoleTimers = _roleRuneTimerMap[roleID];
	NewTimer* pTimer = pRoleTimers[StoneRuneTimer];

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	if (pTimer != 0)
	{
		pTimer->Type(NewTimer::OnceTimer);
		pTimer->Interval(2*5); //2灏忔椂
		int Change = 0;
		if (pTimer->IsRunning())
		{
			RuneTimerCallBackParam param = *(RuneTimerCallBackParam*)(pTimer->GetCallbackFunParam());
			Change = StoneRuneChange[itemId-6007]-param.valueChange;
			param.roleID = roleID;
			param.valueChange= StoneRuneChange[itemId-6007];
			pTimer->SetCallbackFun(TimerDecStoneness, _mainSvc,
						&param, sizeof(param));
			pTimer->reStart();
		}
		else
		{
			Change = StoneRuneChange[itemId-6007];
			RuneTimerCallBackParam param;
			param.roleID = roleID;
			param.valueChange= StoneRuneChange[itemId-6007];
			pTimer->SetCallbackFun(TimerDecStoneness, _mainSvc,
						&param, sizeof(param));
			pTimer->start(&(_mainSvc->_tm));
		}
		pRole->AddDefenceBonus(Change);
		_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				5,
				pRole->DefenceBonus());

	}

}

void BagSvc::RoleUseSaintRune(int roleID, int itemId)
{
	NewTimer **pRoleTimers = _roleRuneTimerMap[roleID];
	NewTimer* pTimer = pRoleTimers[SaintRuneTimer];

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	if (pTimer != 0)
	{
		pTimer->Type(NewTimer::OnceTimer);
		pTimer->Interval(2*5); //2灏忔椂
		int Change = 0;
		if (pTimer->IsRunning())
		{
			RuneTimerCallBackParam param = *(RuneTimerCallBackParam*)(pTimer->GetCallbackFunParam());
			Change = SaintRuneChange[itemId-6011]-param.valueChange;
			param.roleID = roleID;
			param.valueChange= SaintRuneChange[itemId-6011];
			pTimer->SetCallbackFun(TimerDecSaintness, _mainSvc,
						&param, sizeof(param));
			pTimer->reStart();
		}
		else
		{
			Change = SaintRuneChange[itemId-6011];
			RuneTimerCallBackParam param;
			param.roleID = roleID;
			param.valueChange= SaintRuneChange[itemId-6011];
			pTimer->SetCallbackFun(TimerDecSaintness, _mainSvc,
						&param, sizeof(param));
			pTimer->start(&(_mainSvc->_tm));
		}
		pRole->AddMDefenceBonus(Change);
		_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				6,
				pRole->MDefenceBonus());

	}

}

void BagSvc::RoleUseIntelliRune(int roleID, int itemId)
{
	NewTimer **pRoleTimers = _roleRuneTimerMap[roleID];
	NewTimer* pTimer = pRoleTimers[IntelliRuneTimer];

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	if (pTimer != 0)
	{
		pTimer->Type(NewTimer::OnceTimer);
		pTimer->Interval(2*5); //2灏忔椂
		int Change = 0;
		if (pTimer->IsRunning())
		{
			RuneTimerCallBackParam param = *(RuneTimerCallBackParam*)(pTimer->GetCallbackFunParam());
			Change = IntelliRuneChange[itemId-6015]-param.valueChange;
			param.roleID = roleID;
			param.valueChange= IntelliRuneChange[itemId-6015];
			pTimer->SetCallbackFun(TimerDecIntelliness, _mainSvc,
						&param, sizeof(param));
			pTimer->reStart();
		}
		else
		{
			Change = IntelliRuneChange[itemId-6015];
			RuneTimerCallBackParam param;
			param.roleID = roleID;
			param.valueChange= IntelliRuneChange[itemId-6015];
			pTimer->SetCallbackFun(TimerDecIntelliness, _mainSvc,
						&param, sizeof(param));
			pTimer->start(&(_mainSvc->_tm));
		}
		pRole->AddIntelligenceBonus(Change);
		_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				9,
				pRole->IntelligenceBonus());

	}

}

void BagSvc::RoleUseCorrectRune(int roleID, int itemId)
{
	NewTimer **pRoleTimers = _roleRuneTimerMap[roleID];
	NewTimer* pTimer = pRoleTimers[CorrectRuneTimer];

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	if (pTimer != 0)
	{
		pTimer->Type(NewTimer::OnceTimer);
		pTimer->Interval(2*5); //2灏忔椂
		int Change = 0;
		if (pTimer->IsRunning())
		{
			RuneTimerCallBackParam param = *(RuneTimerCallBackParam*)(pTimer->GetCallbackFunParam());
			Change = CorrectRuneChange[itemId-6019]-param.valueChange;
			param.roleID = roleID;
			param.valueChange= CorrectRuneChange[itemId-6019];
			pTimer->SetCallbackFun(TimerDecCorrectness, _mainSvc,
						&param, sizeof(param));
			pTimer->reStart();
		}
		else
		{
			Change = CorrectRuneChange[itemId-6019];
			RuneTimerCallBackParam param;
			param.roleID = roleID;
			param.valueChange= CorrectRuneChange[itemId-6019];
			pTimer->SetCallbackFun(TimerDecCorrectness, _mainSvc,
						&param, sizeof(param));
			pTimer->start(&(_mainSvc->_tm));
		}
		pRole->AddHitRateBonus(Change);
		_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				12,
				pRole->HitRateBonus());

	}

}

void BagSvc::RoleUseSpeedRuneRune(int roleID, int itemId)
{
	NewTimer **pRoleTimers = _roleRuneTimerMap[roleID];
	NewTimer* pTimer = pRoleTimers[SpeedRuneTimer];

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	if (pTimer != 0)
	{
		pTimer->Type(NewTimer::OnceTimer);
		pTimer->Interval(2*5); //2灏忔椂
		int Change = 0;
		if (pTimer->IsRunning())
		{
			RuneTimerCallBackParam param = *(RuneTimerCallBackParam*)(pTimer->GetCallbackFunParam());
			Change = SpeedRuneChange[itemId-6023]-param.valueChange;
			param.roleID = roleID;
			param.valueChange= SpeedRuneChange[itemId-6023];
			pTimer->SetCallbackFun(TimerDecSpeedness, _mainSvc,
						&param, sizeof(param));
			pTimer->reStart();
		}
		else
		{
			Change = SpeedRuneChange[itemId-6023];
			RuneTimerCallBackParam param;
			param.roleID = roleID;
			param.valueChange= SpeedRuneChange[itemId-6023];
			pTimer->SetCallbackFun(TimerDecSpeedness, _mainSvc,
						&param, sizeof(param));
			pTimer->start(&(_mainSvc->_tm));
		}
		pRole->AddAttackSpeedBonus(Change);
		_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				14,
				pRole->AttackSpeedBonus());

	}

}

void BagSvc::RoleUseFocusRune(int roleID, int itemId)
{
	NewTimer **pRoleTimers = _roleRuneTimerMap[roleID];
	NewTimer* pTimer = pRoleTimers[FocusRuneTimer];

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	if (pTimer != 0)
	{
		pTimer->Type(NewTimer::OnceTimer);
		pTimer->Interval(2*5); //2灏忔椂
		int Change = 0;
		if (pTimer->IsRunning())
		{
			RuneTimerCallBackParam param = *(RuneTimerCallBackParam*)(pTimer->GetCallbackFunParam());
			Change = FocusRuneChange[itemId-6027]-param.valueChange;
			param.roleID = roleID;
			param.valueChange= FocusRuneChange[itemId-6027];
			pTimer->SetCallbackFun(TimerDecFocusness, _mainSvc,
						&param, sizeof(param));
			pTimer->reStart();
		}
		else
		{
			Change = FocusRuneChange[itemId-6027];
			RuneTimerCallBackParam param;
			param.roleID = roleID;
			param.valueChange= FocusRuneChange[itemId-6027];
			pTimer->SetCallbackFun(TimerDecFocusness, _mainSvc,
						&param, sizeof(param));
			pTimer->start(&(_mainSvc->_tm));
		}
		pRole->AddCritRateBonus(Change);
		_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				7,
				pRole->CritRateBonus());

	}

}

void BagSvc::RoleUseFlyRune(int roleID, int itemId)
{
	NewTimer **pRoleTimers = _roleRuneTimerMap[roleID];
	NewTimer* pTimer = pRoleTimers[FlyRuneTimer];

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	if (pTimer != 0)
	{
		pTimer->Type(NewTimer::OnceTimer);
		pTimer->Interval(2*5); //2灏忔椂
		int Change = 0;
		if (pTimer->IsRunning())
		{
			RuneTimerCallBackParam param = *(RuneTimerCallBackParam*)(pTimer->GetCallbackFunParam());
			Change = FlyRuneChange[itemId-6031]-param.valueChange;
			param.roleID = roleID;
			param.valueChange= FlyRuneChange[itemId-6031];
			pTimer->SetCallbackFun(TimerDecFlyness, _mainSvc,
						&param, sizeof(param));
			pTimer->reStart();
		}
		else
		{
			Change = FlyRuneChange[itemId-6031];
			RuneTimerCallBackParam param;
			param.roleID = roleID;
			param.valueChange= FlyRuneChange[itemId-6031];
			pTimer->SetCallbackFun(TimerDecFlyness, _mainSvc,
						&param, sizeof(param));
			pTimer->start(&(_mainSvc->_tm));
		}
		pRole->AddAgilityBonus(Change);
		_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				10,
				pRole->AgilityBonus());

	}

}

void BagSvc::RoleUseAngryRune(int roleID, int itemId)
{
	NewTimer **pRoleTimers = _roleRuneTimerMap[roleID];
	NewTimer* pTimer = pRoleTimers[AngryRuneTimer];

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	if (pTimer != 0)
	{
		pTimer->Type(NewTimer::OnceTimer);
		pTimer->Interval(2*5); //2灏忔椂
		int Change = 0;
		if (pTimer->IsRunning())
		{
			RuneTimerCallBackParam param = *(RuneTimerCallBackParam*)(pTimer->GetCallbackFunParam());
			Change = AngryRuneChange[itemId-6035]-param.valueChange;
			param.roleID = roleID;
			param.valueChange= AngryRuneChange[itemId-6035];
			pTimer->SetCallbackFun(TimerDecAngryness, _mainSvc,
						&param, sizeof(param));
			pTimer->reStart();
		}
		else
		{
			Change = AngryRuneChange[itemId-6035];
			RuneTimerCallBackParam param;
			param.roleID = roleID;
			param.valueChange= AngryRuneChange[itemId-6035];
			pTimer->SetCallbackFun(TimerDecAngryness, _mainSvc,
						&param, sizeof(param));
			pTimer->start(&(_mainSvc->_tm));
		}
		pRole->AddStrengthBonus(Change);
		_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				8,
				pRole->StrengthBonus());

	}

}

void BagSvc::RoleUseExtremeRune(int roleID, int itemId)
{
	NewTimer **pRoleTimers = _roleRuneTimerMap[roleID];
	NewTimer* pTimer = pRoleTimers[ExtremeRuneTimer];

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

	if (pTimer != 0)
	{
		pTimer->Type(NewTimer::OnceTimer);
		pTimer->Interval(2*5); //2灏忔椂
		int Change = 0;
		if (pTimer->IsRunning())
		{
			RuneTimerCallBackParam param = *(RuneTimerCallBackParam*)(pTimer->GetCallbackFunParam());
			Change = ExtremeRuneChange[itemId-6039]-param.valueChange;
			param.roleID = roleID;
			param.valueChange= ExtremeRuneChange[itemId-6039];
			pTimer->SetCallbackFun(TimerDecExtremeness, _mainSvc,
						&param, sizeof(param));
			pTimer->reStart();
		}
		else
		{
			Change = ExtremeRuneChange[itemId-6039];
			RuneTimerCallBackParam param;
			param.roleID = roleID;
			param.valueChange= ExtremeRuneChange[itemId-6039];
			pTimer->SetCallbackFun(TimerDecExtremeness, _mainSvc,
						&param, sizeof(param));
			pTimer->start(&(_mainSvc->_tm));
		}
		pRole->AddAttackPowerHighBonus(Change);
		_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				3,
				pRole->AttackPowerHighBonus());

	}

}

void BagSvc::TimerDecToughness( void * obj, void * arg, int argLen )
{
// 	UInt32 roleID = *((UInt32*)arg);
	RuneTimerCallBackParam param = *((RuneTimerCallBackParam*)arg);
	int roleID = param.roleID;
	int hpChange = param.valueChange;
 	MainSvc * _mainSvc = static_cast<MainSvc *>(obj);
 	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

 	pRole->AddMaxHpBonus(-hpChange);

	_mainSvc->GetAvatarSvc()->NotifySingleBonus(roleID, 1,pRole->MaxHpBonus() );

 	UInt32 hp = pRole->Hp();
 	UInt32 maxHp = pRole->MaxHp();

 	if(hp>maxHp){
 		pRole->AddHp(hp-maxHp);
 		_mainSvc->GetBagSvc()->NotifyHPandMP(roleID);
 	}
}


void BagSvc::TimerDecStoneness( void * obj, void * arg, int argLen )
{
	RuneTimerCallBackParam param = *((RuneTimerCallBackParam*)arg);
	int roleID = param.roleID;
	int change = param.valueChange;
 	MainSvc * _mainSvc = static_cast<MainSvc *>(obj);

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

 	pRole->AddDefenceBonus(-change);

	_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				5,
				pRole->DefenceBonus());
}

void BagSvc::TimerDecSaintness( void * obj, void * arg, int argLen )
{
	RuneTimerCallBackParam param = *((RuneTimerCallBackParam*)arg);
	int roleID = param.roleID;
	int change = param.valueChange;
 	MainSvc * _mainSvc = static_cast<MainSvc *>(obj);

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

 	pRole->AddMDefenceBonus(-change);

	_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				6,
				pRole->MDefenceBonus());

}

void BagSvc::TimerDecIntelliness( void * obj, void * arg, int argLen )
{
	RuneTimerCallBackParam param = *((RuneTimerCallBackParam*)arg);
	int roleID = param.roleID;
	int change = param.valueChange;
 	MainSvc * _mainSvc = static_cast<MainSvc *>(obj);

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

 	pRole->AddIntelligenceBonus(-change);

	_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				9,
				pRole->IntelligenceBonus());

}

void BagSvc::TimerDecCorrectness( void * obj, void * arg, int argLen )
{
	RuneTimerCallBackParam param = *((RuneTimerCallBackParam*)arg);
	int roleID = param.roleID;
	int change = param.valueChange;
 	MainSvc * _mainSvc = static_cast<MainSvc *>(obj);

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

 	pRole->AddHitRateBonus(-change);

	_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				12,
				pRole->HitRateBonus());

}

void BagSvc::TimerDecSpeedness( void * obj, void * arg, int argLen )
{
	RuneTimerCallBackParam param = *((RuneTimerCallBackParam*)arg);
	int roleID = param.roleID;
	int change = param.valueChange;
 	MainSvc * _mainSvc = static_cast<MainSvc *>(obj);

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

 	pRole->AddAttackSpeedBonus(-change);

	_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				14,
				pRole->AttackSpeedBonus());
}


void BagSvc::TimerDecFocusness( void * obj, void * arg, int argLen )
{
	RuneTimerCallBackParam param = *((RuneTimerCallBackParam*)arg);
	int roleID = param.roleID;
	int change = param.valueChange;
 	MainSvc * _mainSvc = static_cast<MainSvc *>(obj);

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

 	pRole->AddCritRateBonus(-change);

	_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				7,
				pRole->CritRateBonus());
}

void BagSvc::TimerDecFlyness( void * obj, void * arg, int argLen )
{
	RuneTimerCallBackParam param = *((RuneTimerCallBackParam*)arg);
	int roleID = param.roleID;
	int change = param.valueChange;
 	MainSvc * _mainSvc = static_cast<MainSvc *>(obj);

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

 	pRole->AddAgilityBonus(-change);

	_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				10,
				pRole->AgilityBonus());

}

void BagSvc::TimerDecAngryness( void * obj, void * arg, int argLen )
{
	RuneTimerCallBackParam param = *((RuneTimerCallBackParam*)arg);
	int roleID = param.roleID;
	int change = param.valueChange;
 	MainSvc * _mainSvc = static_cast<MainSvc *>(obj);

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

 	pRole->AddStrengthBonus(-change);

	_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				8,
				pRole->StrengthBonus());

}

void BagSvc::TimerDecExtremeness( void * obj, void * arg, int argLen )
{
	RuneTimerCallBackParam param = *((RuneTimerCallBackParam*)arg);
	int roleID = param.roleID;
	int change = param.valueChange;
 	MainSvc * _mainSvc = static_cast<MainSvc *>(obj);

	RolePtr pRole = _mainSvc->GetCoreData()->ProcessGetRolePtr(roleID);

 	pRole->AddAttackPowerHighBonus(-change);

	_mainSvc->GetAvatarSvc()->NotifySingleBonus(
				roleID,
				3,
				pRole->AttackPowerHighBonus());

}

