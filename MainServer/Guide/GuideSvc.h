//业务处理 server   背包相关

#ifndef GUIDESVC_H
#define GUIDESVC_H

#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"
#include "ArchvBagItemCell.h"


class MainSvc;
 
class GuideSvc
{
public:
	GuideSvc(void* service, ConnectionPool * cp);
	~GuideSvc();
	void SetService( MainSvc * svc );

 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);



	//===================主业务====================================
	
	//[MsgType:1801]设置新手引导的进度
	void ProcessSetGuideStep(Session& session,Packet& packet);

	//[MsgType:1802]结束新手引导
	void ProcessEndGuide(Session& session,Packet& packet);
	
	//===================子业务=======================================

	
	//---------------------s-c的广播----------------------------------

	
	void NotifyGuideStep(UInt32 roleID);


private:
	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;

};


#endif

