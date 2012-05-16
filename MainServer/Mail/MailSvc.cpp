#include "MainSvc.h"
#include "DBOperate.h"
#include "MailSvc.h"
#include "Role.h"
#include "ArchvMail.h"
#include "CoreData.h"
#include "../Bag/BagSvc.h"


MailSvc::MailSvc(void *service, ConnectionPool * cp)
{
	_mainSvc = (MainSvc*)(service);
	_cp = cp;

}

MailSvc::~MailSvc()
{
	//
}

//数据包信息
void MailSvc::OnProcessPacket(Session& session,Packet& packet)
{
	DEBUG_PRINTF1( "C_S req pkg-------MsgType[%d] \n", packet.MsgType );
	DEBUG_SHOWHEX( packet.GetBuffer()->GetReadPtr()-PACKET_HEADER_LENGTH, packet.GetBuffer()->GetDataSize()+PACKET_HEADER_LENGTH, 0, __FILE__, __LINE__ );

	switch(packet.MsgType)
	{
		case 1501: //[MsgType:1501]邮件查询
			ProcessGetRoleMails(session,packet);
		break;

    case 1502: //[MsgType:1502]发送邮件
			ProcessSendRoleMails(session, packet);
			break;

		case 1503: //	[MsgType:1503]删除邮件
			ProcessDeleteRoleMails(session, packet);
			break;

		case 1504: //[MsgType:1504]附件移到背包
	    ProcessAttachToBag(session, packet);
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
void MailSvc::ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode)
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

//[MsgType:1501]邮件查询
void MailSvc::ProcessGetRoleMails(Session& session, Packet& packet)
{
	char szSql[2048];
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(65530);
	UInt32 roleID= packet.RoleID;
	int iRet;
	UInt32 itemid = 0,entityid = 0,num = 0;
	Connection con;
	DBOperate dbo;

	Connection conNew;
	DBOperate dboNew;

	ArchvMailQueryItem queryItem;
	List<ArchvMailQueryItem>lqueryItem;
	List<ArchvMailQueryItem>::iterator plist;

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

	//获取DB连接
	conNew = _cp->GetConnection();
	dboNew.SetHandle(conNew.GetHandle());

	//删除超过30天的邮件
	sprintf(szSql,"delete from MailBox \
	          where (now()- SendTime)>= %d and RoleID = %d;",MAIL_SAVE_TIME,roleID);
	iRet = dbo.ExceSQL(szSql);
	if(iRet != 0)
	{
	  RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_DEBUG,__FILE__,__LINE__,"Excute SQL not found sql[%s]" , szSql);
		goto EndOf_Process;
	}

  sprintf(szSql,"select MailBox.MailID,\
  											Role.RoleName,\
  											    MailBox.SendTime,\
                                                MailBox.Money,\
 								 				MailBox.ItemID,\
 								 				MailBox.Num,\
 								 				MailBox.EntityID,\
 								 				MailBox.Content,\
 								 				MailBox.IsRead,\
 								 				MailBox.MailType,\
 								 				MailBox.MoneyType\
 								 from MailBox,Role where MailBox.SendRoleID = Role.RoleID\
 								 and MailBox.RoleID = %d;",roleID);
 	iRet = dbo.QuerySQL(szSql);
//    LOG(LOG_ERROR,__FILE__,__LINE__,"sql[%s]",szSql);
 	if(iRet < 0)
 	{
 	  RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_DEBUG,__FILE__,__LINE__,"Excute SQL not found sql[%s]" , szSql);
		goto EndOf_Process;
 	}
 	if(iRet == 1)
 	{
 	    RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_DEBUG,__FILE__,__LINE__,"role[%d] not exist or has no mail !" , roleID);
		goto EndOf_Process;
 	}

 	while(dbo.HasRowData())
 	{
 	 	 queryItem.MailID = dbo.GetIntField(0);
 	 	 queryItem.SendRoleName = dbo.GetStringField(1);
 	 	 queryItem.SendTime = dbo.GetStringField(2);
 	 	 queryItem.Money = dbo.GetIntField(3);
 	 	 queryItem.ItemID = dbo.GetIntField(4);
 	 	 queryItem.Num = dbo.GetIntField(5);
 	 	 entityid = dbo.GetIntField(6);
 	 	 queryItem.Content =  dbo.GetStringField(7);
 	 	 queryItem.IsRead = dbo.GetIntField(8);
 	 	 queryItem.MailType = dbo.GetIntField(9);
 	 	 queryItem.MoneyType = dbo.GetIntField(10);
 	 	 queryItem.Durability = 0;

 	 	 if(queryItem.ItemID != 0)
 	 	 {
				sprintf(szSql,"select Durability from Entity \
							where ItemID = %d and EntityID = %d;",queryItem.ItemID,entityid);
				iRet = dboNew.QuerySQL(szSql);
				if(iRet < 0)
				{
				RetCode = ERR_SYSTEM_DBERROR;
				LOG(LOG_DEBUG,__FILE__,__LINE__,"Excute SQL not found sql[%s]" , szSql);
				goto EndOf_Process;
				}
				if(iRet == 0)
				{
				  queryItem.Durability = dboNew.GetIntField(0);
				}
 	 	 }

 	   lqueryItem.push_back(queryItem);

 	   dbo.NextRow();
 	}

 	//未读邮件全部置为已读
 	sprintf(szSql,"update MailBox set IsRead = 1\
 									where RoleID = %d and IsRead = 0;",roleID);
 	iRet = dbo.ExceSQL(szSql);
 	if(iRet != 0)
 	{
	   RetCode = ERR_SYSTEM_DBERROR;
	   LOG(LOG_DEBUG,__FILE__,__LINE__,"Excute SQL not found sql[%d]" , szSql);
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

	LOG(LOG_ERROR,__FILE__,__LINE__,"RetCode[%d]",RetCode);
	s<<RetCode;
	if( 0 == RetCode )
	{
		//RetCode 为0 才会返回包体剩下内容
		s<<lqueryItem;
	}
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
}

//[MsgType:1502]发送邮件
void MailSvc::ProcessSendRoleMails(Session& session, Packet& packet)
{
  char szSql[1024];
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	UInt32 roleID = packet.RoleID;
	int iRet;
	Connection con;
	DBOperate dbo;

	string RecvRoleName,sndRoleName,mailContent;
	UInt32 attachMoney = 0,itemNum = 0,tmpMoney = 0,roleTotalCost = 0;
	UInt16 cellIndex = 0;
	UInt32 recvRoleID = 0;
	DateTime sndTime;
	string strMailSendTime;

	ItemList ilist;
	ArchvMailQueryItem newMail;
	UInt32 entityID = 0,mailCount = 0;
	UInt32 tmpItemNum = 0,tmpItemID = 0,tmpEntityID = 0;
	Byte mailType = 0;
	UInt32 roleOwnMoney = 0;


	//序列化类
	Serializer s(packet.GetBuffer());
 	s>>RecvRoleName>>mailType>>mailContent>>attachMoney>>cellIndex>>itemNum;

	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	// 检查收件人是否存在
	sprintf(szSql,"select RoleID from Role \
									where RoleName = '%s';",RecvRoleName.c_str());
	iRet = dbo.QuerySQL(szSql);
	if(iRet < 0)
	{
	   RetCode = ERR_SYSTEM_DBERROR;
		 LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		 goto EndOf_Process;
	}
	if(iRet == 1)
	{
	   RetCode = ERR_SYSTEM_DBNORECORD;
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"RecvRoleName[%s] is not exist !" ,RecvRoleName.c_str());
		 goto EndOf_Process;
	}

	recvRoleID = dbo.GetIntField(0);

	sprintf(szSql,"select RoleName from Role where RoleID = %d;",roleID);
	iRet = dbo.QuerySQL(szSql);
	if(iRet < 0)
	{
	   RetCode = ERR_SYSTEM_DBERROR;
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s]!" ,szSql);
		 goto EndOf_Process;
	}
	if(iRet == 1)
	{
	   RetCode = ERR_SYSTEM_DBNORECORD;
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"roleName[%d] is not exist !" ,roleID);
		 goto EndOf_Process;
	}
	sndRoleName = dbo.GetStringField(0);


	//邮件类型校验 1 系统邮件，2 用户邮件
 	if(mailType != 1 && mailType != 2)
 	{
 	   RetCode = ERR_APP_DATA;
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"MailType[%d] not exist !" ,mailType);
		 goto EndOf_Process;
 	}

	//发件人(角色)不允许发送系统邮件
  if(mailType == 1)
  {
     RetCode = ERR_APP_DATA;
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"role[%d] not allowed to send system mail !" ,roleID);
		 goto EndOf_Process;
  }


	//检查邮件内容是否为空
	if(mailContent.empty())
	{
	   RetCode = ERR_APP_DATA;
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"mail content is empty !" );
		 goto EndOf_Process;
	}

	//邮件内容大小
	if(mailContent.size() > MAX_MAIL_CONTENT)
	{
	   RetCode = ERR_APP_DATA;
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"role[%d] mail content size more than [%d]",roleID,MAX_MAIL_CONTENT);
		 goto EndOf_Process;
	}

	//发件人是否有这么多金钱
	sprintf(szSql,"select BindMoney,Money from RoleMoney where roleID =%d;",roleID);
	iRet = dbo.QuerySQL(szSql);
	if(iRet < 0)
	{
	   RetCode = ERR_APP_DATA;
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"role[%d] has no so much money[%d]",roleID,attachMoney);
		 goto EndOf_Process;
	}
	if(iRet == 1)
	{
	   RetCode = ERR_APP_DATA;
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"Excute SQL not found szSql[%s]",szSql);
		 goto EndOf_Process;
	}

	tmpMoney = dbo.GetIntField(0);
	roleOwnMoney = dbo.GetIntField(1);

	if(attachMoney > roleOwnMoney)
	{
	   RetCode = ERROR_LACK_MONTY;
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"role[%d] has no so much money !",roleID);
		 goto EndOf_Process;
	}

	if(PER_MAIL_COST > tmpMoney)
	{
	   RetCode = ERROR_LACK_MONTY;
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"role[%d] has no so much bindmoney !",roleID);
		 goto EndOf_Process;
	}

	//收件人邮箱是否满
	sprintf( szSql, "select count(*)\
									from MailBox \
									where RoleID = %d;", recvRoleID);

	iRet = dbo.QuerySQL(szSql);
	if(iRet < 0)
	{
		 RetCode = ERR_SYSTEM_DBNORECORD;
		 LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		 goto EndOf_Process;
	}

	mailCount = dbo.GetIntField(0);

	if(mailCount >= MAIL_MAX_NUM)
	{
		RetCode = ERROR_MAIL_FULL;
		LOG(LOG_ERROR,__FILE__,__LINE__,"recvRole[%d] mailCount is more than 100 !",recvRoleID);
		goto EndOf_Process;
	}


  //从背包中删除物品附件
  if(cellIndex > 0)
  {
    iRet = _mainSvc->GetBagSvc()->DleteItemForEmail(roleID,cellIndex,ilist,entityID);
    if(-1 == iRet )
    {
       RetCode = ERROR_ITEM_NOT_FIND;
	   LOG(LOG_ERROR,__FILE__,__LINE__,"DleteItemForEmail failed !");
	   goto EndOf_Process;
    }
		LOG(LOG_DEBUG,__FILE__,__LINE__,"iRet[%d]",iRet);
		LOG(LOG_DEBUG,__FILE__,__LINE__,"ItemID[%d]",ilist.ItemID);
		LOG(LOG_DEBUG,__FILE__,__LINE__,"num[%d]",ilist.num);
		LOG(LOG_DEBUG,__FILE__,__LINE__,"entityID[%d]",entityID);

    tmpItemID = ilist.ItemID;
    tmpItemNum = ilist.num;
    tmpEntityID = entityID;
  }
  else    //没有物品附件
  {
    tmpItemID = 0;
    tmpItemNum = 0;
    tmpEntityID = 0;
  }

	//MailBox中插入记录
	sprintf(szSql,"insert into MailBox(\
																			RoleID,\
																			SendRoleID,\
																			SendTime,\
																			Money,\
																			MoneyType,\
																			ItemID,\
																			Num,\
																			EntityID,\
																			Content,\
																			IsRead,\
																			MailType\
																			)\
														    value(%d,%d,now(),%d,1,%d,%d,%d,'%s',0,%d);",\
														    recvRoleID,roleID,\
														    attachMoney,tmpItemID,tmpItemNum,tmpEntityID,\
														    mailContent.c_str(),mailType);
	iRet = dbo.ExceSQL(szSql);
	if(iRet != 0)
	{
	     RetCode = ERR_SYSTEM_DBERROR;
			 LOG(LOG_ERROR,__FILE__,__LINE__,"Excute SQL not found szSql[%s]",szSql);
			 goto EndOf_Process;
	}

	newMail.MailID = dbo.LastInsertID();
	newMail.MailType = mailType;
	newMail.IsRead = 0;
	newMail.SendRoleName = sndRoleName;
	newMail.SendTime = strMailSendTime;
	newMail.Content = mailContent;
	newMail.Money = attachMoney;
	newMail.MoneyType = 1;       //类型为:银币
	newMail.ItemID = tmpItemID;
	newMail.Num = tmpItemNum;
	newMail.Durability = 0;

  if(tmpItemID != 0)
  {
     //查询Item的耐久度
			sprintf(szSql,"select Durability from Entity \
								where ItemID = %d and EntityID = %d;",tmpItemID,tmpEntityID);
		  iRet = dbo.QuerySQL(szSql);
	      if(iRet == 0)
	      {
	        newMail.Durability = dbo.GetIntField(0);
	      }
		  if(iRet < 0)
		  {
		   	 RetCode = ERR_SYSTEM_DBERROR;
			 	 LOG(LOG_ERROR,__FILE__,__LINE__,"Excute SQL not found szSql[%d]",szSql);
			 	 goto EndOf_Process;
		  }
		  if(iRet == 1)
		  {
		    newMail.Durability = 0;
		  }
  }


  //扣除每封邮件的收费和金钱附件
  sprintf(szSql,"update RoleMoney set Money = Money - %d,\
  							BindMoney = BindMoney - %d\
  							where RoleID = %d;",attachMoney,PER_MAIL_COST,roleID);
  iRet = dbo.ExceSQL(szSql);
  if(iRet != 0)
  {
        RetCode = ERR_APP_OP;
		LOG(LOG_ERROR,__FILE__,__LINE__,"Excute SQL not found ! szSql[%s]",szSql);
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

	LOG(LOG_ERROR,__FILE__,__LINE__,"RetCode[%d]",RetCode);
	s<<RetCode;
	if( 0 == RetCode )
	{
		//RetCode 为0 才会返回包体剩下内容
	}
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

	if(0 == RetCode)
	{
	  //发送有新邮件通知
	  NotifyNewMail(recvRoleID,newMail);
	}
	if(0 == RetCode)
	{
	  //金钱通知
	  _mainSvc->GetBagSvc()->NotifyMoney(roleID);
	}

}

//[MsgType:1503]删除邮件
void MailSvc::ProcessDeleteRoleMails(Session& session, Packet& packet)
{
	char szSql[1024];
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	UInt32 roleID= packet.RoleID;

	int iRet;
	Connection con;
	DBOperate dbo;

	List<UInt32>lmailID;
	List<UInt32>::iterator pMailID;

	ArchvMailItem mi;

	//序列化类
	Serializer s(packet.GetBuffer());
 	s>>lmailID;
	if( s.GetErrorCode()!= 0 )
	{
		RetCode = ERR_SYSTEM_SERERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"serial error" );
		goto EndOf_Process;
	}

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	//判断角色是否有邮件
	sprintf(szSql,"select MailID from MailBox where RoleID = %d;",roleID);
	iRet = dbo.QuerySQL(szSql);
	if(iRet < 0)
	{
	  RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		goto EndOf_Process;
	}
	if(iRet == 1)
	{
	  RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_DEBUG,__FILE__,__LINE__,"role[%d] has no mail ! " ,roleID);
		goto EndOf_Process;
	}

	for(pMailID = lmailID.begin(); pMailID != lmailID.end(); pMailID++)
	{
	   sprintf(szSql,"delete from MailBox \
	   								where MailID = %d and RoleID = %d;",*pMailID,roleID);
	   iRet = dbo.ExceSQL(szSql);
	   if(iRet != 0)
	   {
	      RetCode = ERR_SYSTEM_DBERROR;
				LOG(LOG_ERROR,__FILE__,__LINE__,"Excute SQL not found[] " ,roleID);
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
	{
		//RetCode 为0 才会返回包体剩下内容
		//s<<lmi;
	}
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		//LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );

}

//[MsgType:1504]附件移到背包
void MailSvc::ProcessAttachToBag(Session& session, Packet& packet)
{
  char szSql[1024];
	UInt32	RetCode = 0;
	DataBuffer	serbuffer(1024);
	UInt32 roleID = packet.RoleID;
	int iRet;
	Connection con;
	DBOperate dbo;

	UInt32 mailID = 0, moneyNum = 0, entityid = 0;
	Byte moveType = 0,moneyType = 0;

	ItemList ilist;

	//序列化类
	Serializer s(packet.GetBuffer());
 	s>>mailID>>moveType;

 	//LOG(LOG_DEBUG,__FILE__,__LINE__,"mailID[%d]==movyType[%d]",mailID,moveType);

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

  sprintf(szSql,"select ItemID,Num,EntityID,Money,MoneyType\
           from MailBox \
           where MailID = %d and RoleID = %d;",mailID, roleID);

  iRet = dbo.QuerySQL(szSql);
  if(iRet < 0)
  {
    RetCode = ERR_SYSTEM_DBERROR;
		LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		goto EndOf_Process;
  }
  if(iRet == 1)
  {
    RetCode = ERR_SYSTEM_DBNORECORD;
		LOG(LOG_ERROR,__FILE__,__LINE__,"role[%d] mail[%d] not exist ! ",roleID,mailID);
		goto EndOf_Process;
  }

  ilist.ItemID = dbo.GetIntField(0);
  ilist.num = dbo.GetIntField(1);
  entityid = dbo.GetIntField(2);
  moneyNum = dbo.GetIntField(3);
  moneyType = dbo.GetIntField(4);

  if(1 == moveType && moneyNum != 0)
  {
    //银币移到背包
    if(moneyType == 1 )
    {
       iRet = _mainSvc->GetBagSvc()->Getmoney(roleID,moneyNum);
	  		if(0 == iRet)
	  		{
				  RetCode = ERR_APP_OP;
				  LOG(LOG_ERROR,__FILE__,__LINE__,"GetBagSvc()->Getmoney  errro!");
				  goto EndOf_Process;
    		}
    }
    //金币移到背包
    if(moneyType == 2)
    {
       iRet = _mainSvc->GetBagSvc()->GetGold(roleID, moneyNum);
       if(iRet == 0)
       {
          RetCode = ERR_APP_OP;
				  LOG(LOG_ERROR,__FILE__,__LINE__,"GetBagSvc()->GetGold  errro!");
				  goto EndOf_Process;
       }
    }

	  iRet = InitMoneyAttach(mailID, roleID);
	  if(iRet)
	  {
	     RetCode = ERR_APP_OP;
		   LOG(LOG_ERROR,__FILE__,__LINE__,"_InitMoneyAttach failed !");
		   goto EndOf_Process;
	  }
  }
  else if(2 == moveType)
  {
  		//附件是否存在
		  if(ilist.ItemID == 0)
		  {
		    RetCode = ERR_APP_OP;
				LOG(LOG_DEBUG,__FILE__,__LINE__," mail[%d] Item Attach not exist ! ",mailID);
				goto EndOf_Process;
		  }

     iRet = _mainSvc->GetBagSvc()->GetItemFromEmail(roleID, ilist, entityid);
     if(-1 == iRet)
     {
       RetCode = ERR_APP_OP;
		   LOG(LOG_ERROR,__FILE__,__LINE__,"GetBagSvc()->GetItemFromEmail error !");
		   goto EndOf_Process;
     }

		 iRet = InitItemAttach(mailID, roleID);
		 if(iRet)
		 {
		   RetCode = ERR_APP_OP;
		   LOG(LOG_ERROR,__FILE__,__LINE__,"InitItemAttach failed !");
		   goto EndOf_Process;
		 }
  }
  else
  {
     RetCode = ERR_APP_DATA;
		 LOG(LOG_ERROR,__FILE__,__LINE__,"Attach MoveType[%d] error ! " ,moveType);
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
	{
		//RetCode 为0 才会返回包体剩下内容
		//s<<lmi;
	}
	p.UpdatePacketLength();

	//发送应答数据
	if( session.Send(&serbuffer) )
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"session.Send error ");
	}

	DEBUG_PRINTF( "ack pkg=======, \n" );
	DEBUG_SHOWHEX( serbuffer.GetReadPtr(), serbuffer.GetDataSize(), 0, __FILE__, __LINE__ );
}

//[MsgType:2001]新邮件通知
void MailSvc::NotifyNewMail(UInt32 roleID,ArchvMailQueryItem &newMail)
{
  List<UInt32>lrid;
	DataBuffer	serbuffer(8196);
	Serializer s( &serbuffer );
	Packet p(&serbuffer);
	serbuffer.Reset();
	p.Direction = DIRECT_S_C_REQ;
	p.SvrType = 1;
	p.SvrSeq = 1;
	p.MsgType = 2001;
	p.UniqID = 214;
	p.PackHeader();

	s<<newMail;
	p.UpdatePacketLength();

	lrid.push_back(roleID);

	if( _mainSvc->Service()->Broadcast(lrid ,&serbuffer))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Broadcast error!!" );
	}

	DEBUG_PRINTF2( "S_C req pkg ----- MsgType[%d],lrid.size[%d]  \n", p.MsgType, lrid.size() );
	DEBUG_SHOWHEX( p.GetBuffer()->GetReadPtr(), p.GetBuffer()->GetDataSize(), 0, __FILE__, __LINE__ );
}


//用户领取了金钱附件后用于初始化MailBox中的Money
//返回 0 成功， 非 0 失败
int MailSvc::InitMoneyAttach(UInt32 &mailid, UInt32 &roleid)
{
	char szSql[1024];
	UInt32	RetCode = 0;
	int iRet = 0;
	Connection con;
	DBOperate dbo;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	sprintf(szSql,"update MailBox set Money = 0 \
		             where MailID = %d and RoleID = %d;", mailid, roleid);

	iRet = dbo.ExceSQL(szSql);
	if(iRet != 0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Failed to Inition Money Attach !" );
		return -1;
	}

	return 0;
}

//用户领取了物品附件之后用于初始化MailBox中的ItemID、Num和EntityID
//返回 0 成功，非 0 失败
int MailSvc::InitItemAttach(UInt32 &mailid, UInt32 &roleid)
{
	char szSql[1024];
	UInt32	RetCode = 0;
	int iRet = 0;
	Connection con;
	DBOperate dbo;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	sprintf(szSql,"update MailBox \
		            set ItemID = 0,Num = 0,EntityID = 0\
		            where MailID = %d and RoleID = %d;", mailid, roleid);

	iRet = dbo.ExceSQL(szSql);
	if(iRet != 0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"Failed to Init Item Attach !" );
		return -1;
	}

	return 0;
}


//系统邮件，金钱类型 1 银币(非绑定铜币)，2 金币(元宝)
//返回 0 成功，非 0 失败
int MailSvc::OnSendSystemMail(ArchvSystemMailItem sysMail)
{
    char szSql[1024];
	UInt32	RetCode = 0;
	int iRet = 0;

	string recvRoleName,mailContent;
	UInt32 recvRoleID = 0,mailCount = 0;
	Byte isStack = 0,itemType = 0,isBind = 0;
	UInt32 durability = 0;
	DateTime nowTime;
	ArchvMailQueryItem newMail;
	Connection con;
	DBOperate dbo;

	//获取DB连接
	con = _cp->GetConnection();
	dbo.SetHandle(con.GetHandle());

	recvRoleName = sysMail.RecvRoleName;
	mailContent = sysMail.Content;

	LOG(LOG_ERROR,__FILE__,__LINE__,"Content[%s]",mailContent.c_str());

	//判断收件人是否存在
	sprintf(szSql,"select RoleID from Role\
									where RoleName = '%s';",recvRoleName.c_str());
	iRet = dbo.QuerySQL(szSql);
	if(iRet < 0)
	{
	  LOG(LOG_ERROR,__FILE__,__LINE__,"Excute SQL not found szSql[%s]!",szSql);
		return -1;
	}
	if(iRet == 1)
	{
	  LOG(LOG_ERROR,__FILE__,__LINE__,"recvRoleName[%s] is not exist !",recvRoleName.c_str());
	  return -1;
	}

	recvRoleID = dbo.GetIntField(0);


	//检查邮件内容是否为空
	if(mailContent.empty())
	{
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"mail content is empty !" );
		 return -1;
	}

	//邮件内容大小
	if(mailContent.size() > MAX_MAIL_CONTENT)
	{
		 LOG(LOG_DEBUG,__FILE__,__LINE__,"role[%d] mail content size more than [%d]",recvRoleID,MAX_MAIL_CONTENT);
		 return -1;
	}

	//收件人邮箱是否满
	sprintf( szSql, "select count(*)\
									from MailBox \
									where RoleID = %d;", recvRoleID);

	iRet = dbo.QuerySQL(szSql);
	if(iRet == 1)
	{
	  mailCount = 0;
	}
	else if(iRet == 0)
    {
       mailCount = dbo.GetIntField(0);
    }
	else
	{
	   LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found or  errro! ,szSql[%s] " , szSql);
		 return -1;
	}

	if(mailCount >= MAIL_MAX_NUM)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"recvRole[%d] mailCount is more than 100 !",recvRoleID);
		return -1;
	}

	//查询是否需要插入实体表
	if(sysMail.ItemID == 0)
	{
	    LOG(LOG_ERROR,__FILE__,__LINE__,"error : itemid[%d] is 0 !");
		return -1;
	}

	sprintf(szSql,"select Bind,Durability,IsStack,ItemType from Item where ItemID = %d",sysMail.ItemID);
	iRet = dbo.QuerySQL(szSql);
	if(iRet < 0)
	{
	    LOG(LOG_ERROR,__FILE__,__LINE__,"Query SQL not found ! szSql[%s]",szSql);
		return -1;
	}
	if(iRet == 1)
	{
	    LOG(LOG_ERROR,__FILE__,__LINE__,"itemid[%d] not exist !",sysMail.ItemID);
		return -1;
	}
	if(iRet == 0)
	{
	  isBind  = dbo.GetIntField(0);
	  durability = dbo.GetIntField(1);
	  isStack = dbo.GetIntField(2);
	  itemType = dbo.GetIntField(3);

	  if(isStack == 0 && sysMail.Num > 1)  //物品不能堆叠
	  {
	  	 LOG(LOG_ERROR,__FILE__,__LINE__,"itemid[%d] is not stack  !",sysMail.ItemID);
		 return -1;
	  }

	  if(itemType == 2)
	  {
	    sprintf(szSql,"insert into Entity(ItemID,Durability,BindStatus)\
			          values(%d,%d,%d);",sysMail.ItemID,durability,isBind);
	    iRet = dbo.ExceSQL(szSql);
		if(iRet != 0)
		{
		  LOG(LOG_ERROR,__FILE__,__LINE__,"itemid[%d] is not stack  !",sysMail.ItemID);
		  return -1;
		}

		sysMail.EntityID = dbo.LastInsertID();
	  }
	  else
	  {
	    sysMail.EntityID = 0;
	  }

	}

	//MailBox中插入记录
	sprintf(szSql,"insert into MailBox(\
																			RoleID,\
																			SendRoleID,\
																			SendTime,\
																			Money,\
																			MoneyType,\
																			ItemID,\
																			Num,\
																			EntityID,\
																			Content,\
																			IsRead,\
																			MailType\
																			)\
														    value(%d,0,now(),%d,%d,%d,%d,%d,'%s',0,1);",\
														    recvRoleID,sysMail.Money,sysMail.MoneyType,\
														    sysMail.ItemID,sysMail.Num,sysMail.EntityID,\
														    sysMail.Content.c_str());
	iRet = dbo.ExceSQL(szSql);
	if(iRet != 0)
	{
			 LOG(LOG_ERROR,__FILE__,__LINE__,"Excute SQL not found szSql[%s]",szSql);
			 return -1;
	}

	newMail.MailID = dbo.LastInsertID();
	newMail.MailType = 2;
	newMail.IsRead = 0;
	newMail.SendRoleName = "系统邮件";
	newMail.SendTime = nowTime.Now().StringDateTime();
	newMail.Content = sysMail.Content;
	newMail.Money = sysMail.Money;
	newMail.MoneyType = sysMail.MoneyType;
	newMail.ItemID = sysMail.ItemID;
	newMail.Num = sysMail.Num;
	newMail.Durability = durability;

	//新邮件通知
   NotifyNewMail(recvRoleID,newMail);

   return 0;

}


