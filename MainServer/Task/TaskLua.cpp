#include "TaskLua.h"
#include "TaskSvc.h"
#include "Log.h"
#include "CoreData.h"

namespace TaskLua
{

//@brief 注册C API for lua
void RegisterTaskCAPI(lua_State* L)
{
	lua_register(L,"GetRoleTaskStatus",GetRoleTaskStatus);
	lua_register(L,"GetRoleProID",GetRoleProID);
	
	lua_register(L,"AddRoleItem",AddRoleItem);
	lua_register(L,"AddRoleMoney",AddRoleMoney);
	lua_register(L,"AddRoleBindMoney",AddRoleBindMoney);
	
	lua_register(L,"AddRoleExp",AddRoleExp);
	lua_register(L,"DeleteItem",DeleteItem);
	
	lua_register(L,"AddRoleTaskKillMonster",AddRoleTaskKillMonster);
	lua_register(L,"AddRoleTaskHoldItem",AddRoleTaskHoldItem);
	lua_register(L,"AddRoleTaskUseItem", AddRoleTaskUseItem );
	lua_register(L,"AddRoleTaskDress", AddRoleTaskDress);
	lua_register(L,"AddRoleTaskCallPet", AddRoleTaskCallPet);
	lua_register(L,"AddRoleTaskLearnSkill", AddRoleTaskLearnSkill);
	lua_register(L,"AddRoleTaskComposeEquip", AddRoleTaskComposeEquip);
	lua_register(L,"AddRoleTaskAddFriend", AddRoleTaskAddFriend);
	lua_register(L,"AddRoleTaskAddTeam", AddRoleTaskAddTeam);


	
	
	
 	lua_settop(L,0);
}



//@brief 获取lua 脚本执行的返回值 
//		仅限于有一个整数返回值的脚本
//@return lua 的返回值
int GetLuaRetCode(lua_State* L)
{
	return lua_tointeger(L,-1);
}



//@brief 查询指定角色指定任务的状态信息
//@lua 调用格式: GetRoleTaskStatus( taskID ) 
//@param-Lua	taskID	任务ID
//@return-Lua	status 1 未接 2未完成 3 已完成 4 已交付
int GetRoleTaskStatus(lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID;
	UInt32 taskID;
	Byte status = 0;
 	int iRet = 0;

	//入参
	GET_TASKSVC(L,taskSvc);
	GET_ROLEID(L, roleID);	
	taskID = lua_tointeger(L,1);

 	iRet = taskSvc->GetRoleTaskStatus( roleID, taskID, status );
 	if(iRet)
 	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"GetRoleTaskStatus error!!!!");
		return 0;
	}
	
	lua_pushinteger(L,status);
 
	return 1;
}

//@brief 查询角色的职业
//@lua 调用格式: GetRoleProID() 
//@return-Lua	proID	职业ID
int GetRoleProID(lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID;
	UInt32 taskID;
	UInt32 proID = 0;
 	int iRet = 0;

	//入参
	GET_TASKSVC(L,taskSvc);
	GET_ROLEID(L, roleID);	



 	iRet = taskSvc->GetRoleProID( roleID, proID);
 	if(iRet)
 	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"GetRoleProID error!!!!");
		return 0;
	}

	
	lua_pushinteger(L,proID);
 
	return 1;
}




int AddRoleMoney(lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	UInt32 num=0;
	int iRet=0;
	List<ItemCell> lic;
	
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	num=lua_tointeger(L,1);

	iRet=taskSvc->AddRoleMoney(roleID,num);
	if(iRet==0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"AddRoleMoney error!!!!");
		return 0;
	}
	return 0;
}





int AddRoleBindMoney(lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	UInt32 num=0;
	int iRet=0;
	List<ItemCell> lic;
	
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	num=lua_tointeger(L,1);

	iRet=taskSvc->AddRoleBindMoney(roleID,num);
	if(iRet==0)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"AddRoleBindMoney error!!!!");
		return 0;
	}
	return 0;
}


int AddRoleExp(lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	UInt32 exp=0;
	int iRet=0;
	List<ItemCell> lic;
	
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	exp=lua_tointeger(L,1);

	iRet=taskSvc->AddRoleExp(roleID,exp);
	if(iRet==-1)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"AddRoleExp error!!!!");
		return 0;
	}
	//s-c的获得经验的通知，暂时没做
	return 0;
}
int AddRoleItem(lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	int iRet=0;
	UInt32 ItemID=0;
	UInt32 num=0;
	
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	ItemID=lua_tointeger(L,1);
	num=lua_tointeger(L,2);

	iRet=taskSvc->AddRoleItem(roleID,ItemID,num);
	if(iRet==-1)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"there is no room to get item error!!!!");
		return 0;
	}
	if(iRet==1)
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"get item error!!!!");
		return 0;
		
	}
	
	return 0;
}

int DeleteItem(lua_State* L)
{
		TaskSvc * taskSvc;
		UInt32 roleID = 0;
		int iRet=0;
		UInt32 ItemID=0;
		UInt32 num=0;
		
		GET_TASKSVC( L,taskSvc );
		GET_ROLEID( L, roleID );
		ItemID=lua_tointeger(L,1);
		num=lua_tointeger(L,2);

		iRet=taskSvc->DeleteItem(roleID,ItemID,num);
		if(iRet==-1)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"there is not so many  item error!!!!");
			return 0;
		}
		if(iRet==1)
		{
			LOG(LOG_ERROR,__FILE__,__LINE__,"get item error!!!!");
			return 0;
			
		}
		
		return 0;
}


//@brief 增加任务细项--杀怪
//@lua 调用格式: AddRoleTaskKillMonster( goalID, goalNum ) 
//@param-Lua	goalID	目标ID
//@param-Lua	goalNum		目标数量
//@return-Lua	空
int AddRoleTaskKillMonster(lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	UInt32 taskID = 0;

	//入参
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	GET_TASKID( L, taskID );
	UInt32 goalID = lua_tointeger(L,1);
	UInt32 goalNum = lua_tointeger(L,2);
	Byte	 goalType = 1;										//杀怪

 	int iRet = taskSvc->AddRoleTaskDetail( roleID, taskID, goalType, goalID, goalNum);
 	if(iRet)
 	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"AddRoleTaskDetail error!roleID[%d], taskID[%d], goalType[%d], goalID[%d], goalNum[%d]", roleID, taskID, goalType, goalID, goalNum	);
		return 0;
	}
	
	return 0;
}


//@brief 增加任务细项--物品获取  , 即 物品持有
//@lua 调用格式: AddRoleTaskHoldItem( goalID, goalNum ) 
//@param-Lua	goalID	目标ID
//@param-Lua	goalNum		目标数量
//@return-Lua	空
int AddRoleTaskHoldItem(lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	UInt32 taskID = 0;

	//入参
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	GET_TASKID( L, taskID );
	UInt32 goalID = lua_tointeger(L,1);
	UInt32 goalNum = lua_tointeger(L,2);
	Byte	 goalType = 2;										//物品获取,物品持有

 	int iRet = taskSvc->AddRoleTaskDetail( roleID, taskID, goalType, goalID, goalNum);
 	if(iRet)
 	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"AddRoleTaskDetail error!roleID[%d], taskID[%d], goalType[%d], goalID[%d], goalNum[%d]", roleID, taskID, goalType, goalID, goalNum	);
		return 0;
	}
	
	return 0;
}


//@brief 增加任务细项--使用物品
//@lua 调用格式: AddRoleTaskUseItem( goalID ) 
//@param-Lua	goalID	目标ID
//@return-Lua	空
int AddRoleTaskUseItem(lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	UInt32 taskID = 0;

	//入参
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	GET_TASKID( L, taskID );
	UInt32 goalID = lua_tointeger(L,1);
	UInt32 goalNum = 1;
	Byte	 goalType = 3;										//使用物品

 	int iRet = taskSvc->AddRoleTaskDetail( roleID, taskID, goalType, goalID, goalNum);
 	if(iRet)
 	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"AddRoleTaskDetail error!roleID[%d], taskID[%d], goalType[%d], goalID[%d], goalNum[%d]", roleID, taskID, goalType, goalID, goalNum	);
		return 0;
	}
	
	return 0;
}




//@brief 增加任务细项--穿装备
//@lua 调用格式: AddRoleTaskDress( goalID ) 
//@param-Lua	goalID	目标ID
//@return-Lua	空
int AddRoleTaskDress(lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	UInt32 taskID = 0;

	//入参
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	GET_TASKID( L, taskID );
	UInt32 goalID = lua_tointeger(L,1);
	UInt32 goalNum = 1;
	Byte	 goalType = 4;										//穿装备

 	int iRet = taskSvc->AddRoleTaskDetail( roleID, taskID, goalType, goalID, goalNum);
 	if(iRet)
 	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"AddRoleTaskDetail error!roleID[%d], taskID[%d], goalType[%d], goalID[%d], goalNum[%d]", roleID, taskID, goalType, goalID, goalNum	);
		return 0;
	}
	
	return 0;
}







//@brief 增加任务细项--穿装备
//@lua 调用格式: AddRoleTaskCallPet( goalID ) 
//@param-Lua	goalID	目标ID
//@return-Lua	空
int AddRoleTaskCallPet(lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	UInt32 taskID = 0;

	//入参
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	GET_TASKID( L, taskID );
	UInt32 goalID = lua_tointeger(L,1);
	UInt32 goalNum = 1;
	Byte	 goalType = 5;										//召唤宠物

 	int iRet = taskSvc->AddRoleTaskDetail( roleID, taskID, goalType, goalID, goalNum);
 	if(iRet)
 	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"AddRoleTaskDetail error!roleID[%d], taskID[%d], goalType[%d], goalID[%d], goalNum[%d]", roleID, taskID, goalType, goalID, goalNum	);
		return 0;
	}
	
	return 0;
}



//@brief 增加任务细项--学习技能
//@lua 调用格式: AddRoleTaskLearnSkill () 
//@return-Lua	空
int AddRoleTaskLearnSkill (lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	UInt32 taskID = 0;

	//入参
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	GET_TASKID( L, taskID );
	UInt32 goalID = 0;
	UInt32 goalNum = 1;
	Byte	 goalType = 6;										//学习技能

 	int iRet = taskSvc->AddRoleTaskDetail( roleID, taskID, goalType, goalID, goalNum);
 	if(iRet)
 	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"AddRoleTaskDetail error!roleID[%d], taskID[%d], goalType[%d], goalID[%d], goalNum[%d]", roleID, taskID, goalType, goalID, goalNum	);
		return 0;
	}
	
	return 0;
}





//@brief 增加任务细项--装备合成
//@lua 调用格式: AddRoleTaskComposeEquip () 
//@return-Lua	空
int AddRoleTaskComposeEquip (lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	UInt32 taskID = 0;

	//入参
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	GET_TASKID( L, taskID );
	UInt32 goalID = 0;
	UInt32 goalNum = 1;
	Byte	 goalType = 7;										//装备合成

 	int iRet = taskSvc->AddRoleTaskDetail( roleID, taskID, goalType, goalID, goalNum);
 	if(iRet)
 	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"AddRoleTaskDetail error!roleID[%d], taskID[%d], goalType[%d], goalID[%d], goalNum[%d]", roleID, taskID, goalType, goalID, goalNum	);
		return 0;
	}
	
	return 0;
}


//@brief 增加任务细项--添加好友
//@lua 调用格式: AddRoleTaskAddFriend () 
//@return-Lua	空
int AddRoleTaskAddFriend (lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	UInt32 taskID = 0;

	//入参
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	GET_TASKID( L, taskID );
	UInt32 goalID = 0;
	UInt32 goalNum = 1;
	Byte	 goalType = 8;										//添加好友

 	int iRet = taskSvc->AddRoleTaskDetail( roleID, taskID, goalType, goalID, goalNum);
 	if(iRet)
 	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"AddRoleTaskDetail error!roleID[%d], taskID[%d], goalType[%d], goalID[%d], goalNum[%d]", roleID, taskID, goalType, goalID, goalNum	);
		return 0;
	}
	
	return 0;
}



//@brief 增加任务细项--组队参与
//@lua 调用格式: AddRoleTaskAddTeam () 
//@return-Lua	空
int AddRoleTaskAddTeam (lua_State* L)
{
	TaskSvc * taskSvc;
	UInt32 roleID = 0;
	UInt32 taskID = 0;

	//入参
	GET_TASKSVC( L,taskSvc );
	GET_ROLEID( L, roleID );
	GET_TASKID( L, taskID );
	UInt32 goalID = 0;
	UInt32 goalNum = 1;
	Byte	 goalType = 9;										//组队参与

 	int iRet = taskSvc->AddRoleTaskDetail( roleID, taskID, goalType, goalID, goalNum);
 	if(iRet)
 	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"AddRoleTaskDetail error!roleID[%d], taskID[%d], goalType[%d], goalID[%d], goalNum[%d]", roleID, taskID, goalType, goalID, goalNum	);
		return 0;
	}
	
	return 0;
}



}



