#include "NpcLua.h"
#include "NpcSvc.h"
#include "Log.h"
#include "../Task/TaskSvc.h"

namespace NpcLua
{

//@brief 注册C API for lua
void RegisterNpcCAPI(lua_State* L)
{
	lua_register(L,"GetRoleTaskStatus",GetRoleTaskStatus);
	
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








}


