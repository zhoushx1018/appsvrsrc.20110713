	#include "Team.h"
	#include "ArchvRole.h"

		Team::Team():_leaderRoleID(0),_teamID(0)
		{
			
		}

		Team::~Team()
		{
		}
		

		list<TeamRole> Team::GetMemberRoleID()
		{
			return _listMember;
		}
		UInt32 Team::GetTeamID()
		{
			return _teamID;
		}
		UInt32 Team::GetLeaderRoleID()
		{
			return _leaderRoleID;
		}
		
		void Team::AddToTeam(UInt32 roleID)//加入队员
		{
			TeamRole role;
			role.roleId=roleID;
			role.status=1;
			_listMember.push_back(role);
		}
		void Team::ChangToBeLeader(UInt32 roleID)//提升队长
		{
			if(roleID!=0)
			{
				//自动提升队长
				changeRoleStues(_leaderRoleID,1);
				LeaderRoleID(roleID);
				_leaderRoleID=roleID;
			}
			else
			{
				//自己选择
				
			}
		}

		void Team::changeRoleStues(UInt32 roleID,Byte type)
		{
				list<TeamRole>::iterator it;
				for( it=_listMember.begin();it!=_listMember.end();it++)
				{
					if(it->roleId==roleID)
					{
						it->status=type;
						break;
					}
				}
		}
		void Team::OutTheTeam(UInt32 roleID)//离开队伍
		{	
				list<TeamRole>::iterator it;
				
				for( it=_listMember.begin();it!=_listMember.end();it++)
				{
//					LOG(LOG_ERROR,__FILE__,__LINE__,"ExceSQL data not found ,szSql[%d] ",it->roleId);
					if(it->roleId==roleID)
					{
						
						_listMember.erase(it);
						break;
					}
				}
				
		}
			
		void Team::TeamID(UInt32 input)
		{
			_teamID=input;
		}
		void Team::LeaderRoleID(UInt32 input)
		{
			_leaderRoleID=input;
			changeRoleStues(input,2);
		}

