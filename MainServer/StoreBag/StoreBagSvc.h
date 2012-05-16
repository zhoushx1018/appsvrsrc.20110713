//业务处理 server   背包相关

#ifndef STOREBAGSVC_H
#define STOREBAGSVC_H
//#define TOPCELL_NUM 25
#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"
#include "ArchvBagItemCell.h"


class MainSvc;
 
class StoreBagSvc
{
public:
	StoreBagSvc(void* service, ConnectionPool * cp);
	~StoreBagSvc();
	void SetService( MainSvc * svc );

 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);



	//===================主业务====================================
	
	
	void ProcessGetItem(Session& session,Packet& packet);//msgtype 1101 查询仓库物品列表
	
	void ProcessMoveoutItem(Session& session,Packet& packet);//取出仓库东西  1102
	
	void ProcessDropItem(Session & session,Packet & packet);//物品丢弃。
	
	void ProcessSortItem(Session & session,Packet & packet);//物品整理
	
	void ProcesschangeItem(Session & session,Packet & packet);//物品位置交换

    void ProcessAddTopCell(Session & session,Packet & packet);//改变仓库上限

	UInt32 GetRoleCellNum(UInt32 roleID);//获得单元的数量

	//===================子业务=======================================

	
	//---------------------s-c的广播----------------------------------

	
	void NotifyStoreBag(UInt32 roleID,List<ItemCell>& lic);


private:
	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;

};


#endif

