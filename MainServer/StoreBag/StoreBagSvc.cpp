
#include "StoreBagSvc.h"
#include "MainSvc.h"
#include "DBOperate.h"
#include "ArchvBagItemCell.h"
#include "./Task/TaskSvc.h"
#include "CoreData.h"
#include "Role.h"
#include "./Bag/BagSvc.h"

StoreBagSvc::StoreBagSvc(void* service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;

}

StoreBagSvc::~StoreBagSvc()
{

}
//数据包信息
void StoreBagSvc::OnProcessPacket(Session& session,Packet& packet)
{
DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 1101: //查背包仓库列表
		ProcessGetItem(session,packet);
		break;			
		case 1102://从仓库取出物品
		ProcessMoveoutItem(session,packet);
		break;
		//case 1103://将物品放入仓库
//		ProcessMoveinItem(session,packet);
	//	break;
		case 1104://仓库内部两个物品进行交换
		ProcesschangeItem(session,packet);
		break;
		case 1105://仓库整理
		ProcessSortItem(session,packet);
		break;
		case 1106://仓库扩充
		ProcessAddTopCell(session,packet);
		break;

		default:
			ClientErrorAck(session,packet,ERR_SYSTEM_PARAM);
			LOG(LOG_ERROR,__FILE__,__LINE__,"MsgType[%d] not found",packet.MsgType);
		}
}




//客户端错误应答
//@param  session 连接对象
//@param	packet 请求包
//@param	RetCode 返回errorCode 值
void StoreBagSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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


//MSG{1101}
void StoreBagSvc::ProcessGetItem(Session& session,Packet& packet)
{
//查询背包物品函数
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(2048);
	char szSql[1024];
	Connection con,conSub;
	DBOperate dbo,dboSub;
	Int16 iRet = 0;
	UInt32 roleID = packet.RoleID;
	List<ItemCell> lic;
	Byte PackNum=25;
	ItemCell lic1;//用来存取的对象
	UInt32 money=0;
	UInt32 bindmoney=0;
	UInt32 Gold=0;
	
	
	//序列化类
	Serializer s(packet.GetBuffer());
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	conSub = _cp->GetConnection();
	dboSub.SetHandle(conSub.GetHandle());
	
	DEBUG_PRINTF( "C_S ProcessGetItem sucess!!!!!!!!\n" );	
	
	//查询全部的
	sprintf( szSql, "select StoreBag.CellIndex,StoreBag.ItemID,StoreBag.EntityID,StoreBag.Num,Entity.Durability,Entity.BindStatus from StoreBag left join Entity on StoreBag.EntityID=Entity.EntityID where RoleID= %d  order by StoreBag.CellIndex asc;", roleID);
	iRet=dbo.QuerySQL(szSql);
	if( 1 == iRet )
	{
//		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
	}
	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
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
				sprintf( szSql, "select CdTime from Item where ItemID=%d;", lic1.ItemID);
				iRet = dboSub.QuerySQL(szSql);
				if(iRet==0)
				{
					lic1.cdTime=dboSub.GetIntField(0);
				}
			}
			lic.push_back(lic1);
		//记录集下一条记录
			dbo.NextRow();
		}
	
	}

		

		sprintf(szSql, "select TopStoreCellNum from Role where RoleID=%d;",roleID);
		iRet=dbo.QuerySQL(szSql);
		if(0==iRet)
		{
			PackNum=(Byte)((dbo.GetIntField(0)+36)/36);

		}
		if(iRet!=0)
		{
			//return;
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
	{//RetCode 为0 才会返回包体剩下内
		s<<money;
		s<<bindmoney;
		s<<Gold;
		s<<PackNum;
		s<<lic;
	}	

	p.UpdatePacketLength();
	
	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}
	
	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	return;
	
}

//1102从仓库中取出东西
void StoreBagSvc::ProcessMoveoutItem(Session& session,Packet& packet)
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	Int16 iRet = 0;
	UInt32 CellchangNum=0;
	UInt16 storeCellIndex,CellIndex=0;
	UInt32 ItemType1=0,ItemType2=0;
	UInt32 CellType1=0,CellType2=0;
	List<ItemCell> lic,lic22;
	ItemCell lic1,lic2;
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	UInt32 roleID = packet.RoleID;

	//序列化类
	Serializer s(packet.GetBuffer());
	s>>storeCellIndex>>CellIndex;
	//得到物品ID和数量
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	LOG(LOG_ERROR,__FILE__,__LINE__,"serial error %d %d",storeCellIndex,CellIndex );
	//判段目标有没有物品，
		sprintf(szSql, "select StoreBag.CellIndex,StoreBag.CellType,StoreBag.ItemType,StoreBag.ItemID,StoreBag.EntityID,StoreBag.Num,Entity.Durability,Entity.BindStatus from StoreBag left join Entity on StoreBag.EntityID=Entity.EntityID where RoleID= %d  and StoreBag.CellIndex=%d;",roleID,storeCellIndex);
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
		//判断背包有没有物品
		sprintf(szSql, "select Bag.CellIndex,Bag.CellType,Bag.ItemType, Bag.ItemID,Bag.EntityID,Bag.Num,Entity.Durability,Entity.BindStatus from Bag left join Entity on Bag.EntityID=Entity.EntityID where RoleID= %d  and Bag.CellIndex=%d;",roleID,CellIndex);
			iRet=dbo.QuerySQL(szSql);
			if(0==iRet)
			{
				//有update
				lic2.celIndex=dbo.GetIntField(0);
				CellType2=dbo.GetIntField(1);
				ItemType2=dbo.GetIntField(2);
				lic2.ItemID=dbo.GetIntField(3);
				lic2.EntityID=dbo.GetIntField(4);
				lic2.num=dbo.GetIntField(5);
				lic2.durability=dbo.GetIntField(6);
				lic2.bindStatus=dbo.GetIntField(7);

				//更新两头
				sprintf(szSql, "update Bag set CellType=%d,ItemType=%d,ItemID=%d,EntityID=%d,Num=%d where RoleID=%d and CellIndex=%d;",CellType1,ItemType1,lic1.ItemID,lic1.EntityID,lic1.num,roleID,CellIndex );
				iRet=dbo.ExceSQL(szSql);
				if(iRet!=0)
				{
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
				}
				sprintf(szSql, "update StoreBag set CellType=%d,ItemType=%d,ItemID=%d,EntityID=%d,Num=%d where RoleID=%d and CellIndex=%d;",CellType2,ItemType2,lic2.ItemID,lic2.EntityID,lic2.num,roleID,storeCellIndex);
				iRet=dbo.ExceSQL(szSql);
				if(iRet!=0)
				{
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
				}
				
				
			}
			if(iRet==1)
			{
				//没有，直接insert 
				sprintf(szSql, "insert into Bag(RoleID,CellIndex,CellType,ItemType,ItemID,EntityID,Num) \
				                values(%d,%d,%d,%d,%d,%d,%d);",roleID,CellIndex,CellType1,ItemType1, lic1.ItemID,lic1.EntityID,lic1.num );

				iRet=dbo.ExceSQL(szSql);
				if(iRet!=0)
				{
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
				}

				//delete原来的storebag
				sprintf(szSql, "delete from StoreBag where RoleID=%d and CellIndex=%d;",roleID,storeCellIndex );
				iRet=dbo.ExceSQL(szSql);
				if(iRet!=0)
				{
					RetCode = ERR_SYSTEM_DBERROR;
					LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
					goto EndOf_Process;
				}
				//OK
			}

			
			//整理出S-C的数据
			if(lic2.ItemID!=0)
			{
				lic2.celIndex=storeCellIndex;
				lic1.celIndex=CellIndex;

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
				lic22.push_back(lic2);
				lic.push_back(lic1);
			}
			else
			{
				//目标没有的，
			//lic1.celIndex=;
				lic1.celIndex=CellIndex;
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
	
	
	//任务计数
	_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(roleID,lic1.ItemID);
	if(lic2.ItemID!=0)
	{
		_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(roleID,lic2.ItemID);
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
		//s<<lic;
	}	

	p.UpdatePacketLength();
	
	//发送应答数据
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
			NotifyStoreBag(roleID,lic22);
		}
		_mainSvc->GetBagSvc()->NotifyBag(roleID,lic);
 	}
	return;
}




void StoreBagSvc::ProcessDropItem(Session& session,Packet& packet)
{//物品丢弃
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
	//ItemCell lic1;//用来存取的对象
	s>>celIndex;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
		sprintf( szSql, "delete from StoreBag where RoleID=%d and CellIndex=%d;", roleID,celIndex);
 			iRet=dbo.ExceSQL(szSql);
 			if(iRet!=0 )
			{
				RetCode = ERR_SYSTEM_DBNORECORD;
				LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
				goto EndOf_Process;
			}

				//任务计数
		_mainSvc->GetTaskSvc()->OnBagItemAddOrDelete(roleID,itemID);

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
		//s<<lic;
	}	

	p.UpdatePacketLength();
	
	//发送应答数据
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


void StoreBagSvc::ProcessSortItem(Session& session,Packet& packet)
{//物品整理
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con,conSub;
	DBOperate dbo,dboSub;
	Int16 iRet = 0;
	UInt32 roleID = packet.RoleID;
	UInt32 money=0;
	UInt16 celIndex,num,num1,num2=0;
	UInt32 iemID;
	List<ItemCell> lic;
	ItemCell lic1;//用来存取的对象
	UInt32 cellnum=0,sql_cell;
	
	//序列化类
	Serializer s(packet.GetBuffer());
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}
		
	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	conSub = _cp->GetConnection();
	dboSub.SetHandle(conSub.GetHandle());
		
	DEBUG_PRINTF( "C_S ProcessSortItem sucess!!!!!!!!\n" );	
	//查询全部的堆叠物品
	//提取了没有满数量的单元，以及数量大于1的单元
	sprintf( szSql, "select CellIndex,ItemID,EntityID,Num from StoreBag where RoleID= %d and EntityID = 0 and ItemType<>30 and Num <> 99 order by ItemID desc,Num desc;", roleID);
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
		num=dbo.GetIntField(3);		//获取数据
		dbo.NextRow();
		while(dbo.HasRowData()&&iemID==dbo.GetIntField(1))
		{
			num=num+dbo.GetIntField(3);
			if(num>99)
			{
				sprintf( szSql, "update StoreBag set Num=99 where RoleID= %d and CellIndex=%d;",roleID,celIndex);
				iRet=dboSub.ExceSQL(szSql);
				//大于最大堆叠，就在index上存放最大值
				num=num-99;
			}
			else
			{	
				sprintf( szSql, "delete from StoreBag where  RoleID= %d and CellIndex=%d;",roleID,celIndex);
				iRet=dboSub.ExceSQL(szSql);
			}
			celIndex=dbo.GetIntField(0);
			dbo.NextRow();
		}
		sprintf( szSql, "update StoreBag set Num=%d where RoleID= %d and CellIndex=%d;",num,roleID,celIndex);
		iRet=dboSub.ExceSQL(szSql);
	}

		
	dbo.SetHandle(con.GetHandle());
	//把数据库的值全部划分到1000单元以后
	sprintf( szSql, "update StoreBag set CellIndex=CellIndex+1000 where RoleID= %d;",roleID);
	iRet=dbo.ExceSQL(szSql);
	if( iRet < 0 )
	{
		RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQLSQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
		goto EndOf_Process;
	}
	//查询全部的
	sprintf( szSql, "select CellIndex,ItemID,EntityID,Num from StoreBag where RoleID= %d  order by ItemType,CellIndex asc;", roleID);
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
			sprintf( szSql, "update StoreBag set CellIndex=%d where RoleID= %d and CellIndex=%d;", cellnum,roleID,sql_cell);
			iRet=dboSub.ExceSQL(szSql);
		}
		dbo.NextRow();
	}
		



	
		//查询全部的
		sprintf( szSql, "select StoreBag.CellIndex,StoreBag.ItemID,StoreBag.EntityID,StoreBag.Num,Entity.Durability,Entity.BindStatus from StoreBag left join Entity on StoreBag.EntityID=Entity.EntityID where RoleID= %d	order by StoreBag.CellIndex asc;", roleID);
		iRet=dbo.QuerySQL(szSql);
		if( 1 == iRet )
		{
	//		LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL data not found ,szSql[%s] " , szSql);
		}
		if( iRet < 0 )
		{
			RetCode = ERR_SYSTEM_DBERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"QuerySQL error[%s],szSql[%s] ", mysql_error(con.GetHandle()), szSql);
			goto EndOf_Process;
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
					sprintf( szSql, "select CdTime from Item where ItemID=%d;", lic1.ItemID);
					iRet = dboSub.QuerySQL(szSql);
					if(iRet==0)
					{
						lic1.cdTime=dboSub.GetIntField(0);
					}
				}
				lic.push_back(lic1);
			//记录集下一条记录
				dbo.NextRow();
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

		
	s<<RetCode;

	if( 0 == RetCode )
	{//RetCode 为0 才会返回包体剩下内容
		//s<<money;
		s<<lic;
	}	

	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
	if( 0 == RetCode )
	{
	}
	
	return;
}


void StoreBagSvc::ProcesschangeItem(Session & session,Packet & packet)//物品位置交换
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
	//ItemCell lic1;//用来存取的对象
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
	//将cellindex2判断一下是不是空的
	sprintf( szSql, "select ItemID from StoreBag where RoleID= %d and CellIndex=%d",roleID,celIndex1);
	iRet=dbo.QuerySQL(szSql);
	if(iRet!=0 )
	{
		RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
	sprintf( szSql, "select ItemID from StoreBag where RoleID= %d and CellIndex=%d",roleID,celIndex2);
	iRet=dbo.QuerySQL(szSql);
	if( 1 == iRet )
		{
			//目标位置是空的，将数据存到空位置就行
			sprintf( szSql, "update StoreBag set CellIndex=%d where RoleID= %d and CellIndex=%d;",celIndex2,roleID,celIndex1);
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
		//将cellindx1的值换到特别位置，1000单元以后
		sprintf( szSql, "update StoreBag set CellIndex=CellIndex+1000 where RoleID= %d and CellIndex=%d;",roleID,celIndex1);
		iRet=dbo.ExceSQL(szSql);
		if( 1 == iRet )
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}
		//将cellindex2的值放在cellindex1中，
		sprintf( szSql, "update StoreBag set CellIndex=%d where RoleID= %d and CellIndex=%d;",celIndex1,roleID,celIndex2);
		iRet=dbo.ExceSQL(szSql);
		if( 1 == iRet )
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			goto EndOf_Process;
		}
		//将cellindex1中的值放在cellindex2
		sprintf( szSql, "update StoreBag set CellIndex=%d where RoleID= %d and CellIndex=%d;",celIndex2,roleID,celIndex1+1000);
		iRet=dbo.ExceSQL(szSql);
		if( 1 == iRet )
		{
			RetCode = ERR_SYSTEM_DBNORECORD;
			LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%s] " , szSql);
			goto EndOf_Process;
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

		
	s<<RetCode;
	if( 0 == RetCode )
	{//RetCode 为0 才会返回包体剩下内容
		//s<<lic;
	}	

	p.UpdatePacketLength();
	
	//发送应答数据
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


void StoreBagSvc::ProcessAddTopCell(Session & session,Packet & packet)//物品位置交换
{
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	char szSql[1024];
	Connection con;
	DBOperate dbo;
	UInt32 roleID = packet.RoleID;
	Serializer s(packet.GetBuffer());
	Int16 iRet=0;
	Byte type=0;

	s>>type;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());
	if(_mainSvc->GetBagSvc()->DropGold(roleID,100)==-1)
	{
			RetCode = ERR_SYSTEM_SERERROR;
			LOG(LOG_ERROR,__FILE__,__LINE__,"gold is not ....");
			goto EndOf_Process;
	}
	sprintf( szSql, "update Role set TopStoreCellNum =TopStoreCellNum+36 where RoleID=%d;",roleID);
	iRet=dbo.ExceSQL(szSql);
	if(iRet!=0)
	{	
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"sql erro!%s",szSql);
		goto EndOf_Process;
		
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
		//s<<lic;
	}	

	p.UpdatePacketLength();
	
	//发送应答数据
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


void StoreBagSvc::NotifyStoreBag(UInt32 roleID,List<ItemCell>& lic)
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
	p.MsgType = 1101;
	p.UniqID = 250;
	p.PackHeader();
	List<ItemCell>::iterator itor;
	itor=lic.begin();
	
	//获取DB连接
	s<<lic;
	p.UpdatePacketLength();
	lrid.push_back(roleID);
	LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!,%d,%d",itor->ItemID,itor->cdTime);
	if(_mainSvc->Service()->Broadcast(lrid, &serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!");
	}
	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}


