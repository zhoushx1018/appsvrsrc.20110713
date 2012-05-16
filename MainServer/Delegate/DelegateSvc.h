//业务处理 server   

#ifndef DELEGATESVC_H
#define DELEGATESVC_H

#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"
#include "ArchvBagItemCell.h"


class MainSvc;
class NewTimer;

class DelegateSvc
{
public:
	DelegateSvc(void* service, ConnectionPool * cp);
	~DelegateSvc();
	void SetService( MainSvc * svc );

 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);



	//===================主业务====================================
	
	//[MsgType:1901]查询进度
	void ProcessGetRoleDelegate(Session& session,Packet& packet);

	//[MsgType:1902]请求开始
	void ProcessBegRoleDelegate(Session& session,Packet& packet);
	
	//===================子业务=======================================
	void OnDelegateTimeout(UInt32 delegateID );

	static void TimerCBDelegate( void * obj, void * arg, int argLen );
	

	
	//---------------------s-c的广播----------------------------------
	void NotifyEndOfRoleDelegate(UInt32 roleID,UInt32 taskID);
	

private:
	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;
	std::list<NewTimer*> _timerList;
};


#endif

