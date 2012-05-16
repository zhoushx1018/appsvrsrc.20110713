
#ifndef TEAMSVC_H
#define TEAMSVC_H

#include "GWProxy.h"
#include "IniFile.h"
#include "ConnectionPool.h"


class MainSvc;
 
class TeamSvc
{
public:
	TeamSvc(void* service, ConnectionPool * cp);
	~TeamSvc();
	void SetService( MainSvc * svc );

 	void OnProcessPacket(Session& session,Packet& packet);

	void ClientErrorAck(Session& session, Packet& packet, UInt32	RetCode);



	//===================主业务====================================
	
	
	 
	//请求加入队伍
	void ProcessRequestjoin(Session& session,Packet& packet);
	//S-C到对方，某个人给你发起邀请交易

	//回答加入队伍信息
	void ProcessAnswerRequest(Session& session,Packet& packet);
	//回答后，s-c答案，某个人接受了你的邀请, 向两个人都发送，表示建立起来交易
	void ProcessChangeCaptain(Session& session,Packet& packet);//提升为队长

	void ProcessCapt_Member(Session& session,Packet& packet);//踢出队伍

	void ProcessCallMember(Session& session,Packet& packet);//召唤队员

	void ProcessMember_OUt(Session& session,Packet& packet);//退出队伍

	void ProcessBackOrleave(Session& session,Packet& packet);//归队或者离队


	void NotifyReceive(UInt32 RoleID,string Name,List<UInt32>& it);

	void NotifyTeamChang(UInt32 RoleID,Byte changeType,List<UInt32>& it);

	void NotifyCaptionCall(List<UInt32>& it);
	
	void NotifyRefuse(UInt32 RoleID,UInt32 ID);//谁拒绝谁，RoleID是指谁ID是收到的人

	void NotifyTeamInfo(UInt32 RoleID,list<TeamRole>& l);
private:
	MainSvc * _mainSvc;
	IniFile _file;
	ConnectionPool *_cp;

};


#endif

