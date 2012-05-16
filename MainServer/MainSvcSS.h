// S_S 数据方向的消息处理 server
//

#ifndef MAINSVCSS_H
#define MAINSVCSS_H

#include "SSServer.h"
#include "ConnectionPool.h"
#include <list>


class MainSvc;
class PKSvcSS;
class AccountSvcSS;

class MainSvcSS
{
public:
	friend class Map;

	MainSvcSS();
	~MainSvcSS();

	void SetService( MainSvc * svc );
	
protected:
	int OnInit(void* service);

	void OnStop();

	void OnConnected(Session& session);

	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);


	//------------S_S 交易-----------------------------------------


	//------------子业务处理 -----------------------------------------
	
	

private:
	SSServer<MainSvcSS>* _service;
	IniFile _file;
	ConnectionPool *_cp;

	MainSvc * _mainSvc;

	PKSvcSS * _pkSvcSS;

	AccountSvcSS * _accountSvcSS;
	
};


#endif

