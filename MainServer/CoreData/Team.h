// 角色类
#ifndef TEAM_H
#define TEAM_H

#include "OurDef.h"
#include "Creature.h"
#include  "ArchvRole.h"
//角色升级可加属性点

class Team
{
public:
		Team();
		~Team();

public:
		list<TeamRole> GetMemberRoleID();
		UInt32 GetTeamID();
		UInt32 GetLeaderRoleID();

		
		void AddToTeam(UInt32 roleID);//加入队员
		void ChangToBeLeader(UInt32 roleID);//提升队长
		void OutTheTeam(UInt32);//离开队伍
		void changeRoleStues(UInt32 roleID,Byte type);
		
			
		void TeamID(UInt32 input);
		void LeaderRoleID(UInt32 input);
		
		
	//-------------------------
private:
		UInt32		_teamID;
		UInt32		_leaderRoleID;
		list<TeamRole>	_listMember;

};


#endif

