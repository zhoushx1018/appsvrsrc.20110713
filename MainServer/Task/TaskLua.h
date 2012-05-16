// taskÄ£¿é    C API for lua

#ifndef TASKLUA_H
#define TASKLUA_H

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}


#include "TaskSvc.h"

namespace TaskLua
{
#define GLOBAL_TASKSVC_VAR    "__gTaskSvc__"
#define GLOBAL_ROLEID_VAR  "__gRoleID__"
#define GLOBAL_TASKID_VAR  "__gTaskID__"

void RegisterTaskCAPI(lua_State* L);

#define SET_TASKSVC(l,p)\
	lua_pushlightuserdata(l,p);\
	lua_setfield(l,LUA_GLOBALSINDEX,GLOBAL_TASKSVC_VAR);

#define GET_TASKSVC(l,p)\
	lua_getfield(l,LUA_GLOBALSINDEX,GLOBAL_TASKSVC_VAR);\
	p = (TaskSvc*)lua_touserdata(l,-1);\
	lua_pop(L,1);

#define SET_ROLEID(l,id)\
	lua_pushinteger(l,id);\
	lua_setfield(l,LUA_GLOBALSINDEX,GLOBAL_ROLEID_VAR);

#define GET_ROLEID(l,id)\
	lua_getfield(l,LUA_GLOBALSINDEX,GLOBAL_ROLEID_VAR);\
	id = lua_tointeger(l,-1);\
	lua_pop(L,1);


#define SET_TASKID(l,id)\
	lua_pushinteger(l,id);\
	lua_setfield(l,LUA_GLOBALSINDEX,GLOBAL_TASKID_VAR);

#define GET_TASKID(l,id)\
	lua_getfield(l,LUA_GLOBALSINDEX,GLOBAL_TASKID_VAR);\
	id = lua_tointeger(l,-1);\
	lua_pop(L,1);


int GetLuaRetCode(lua_State* L);

int GetRoleTaskStatus(lua_State* L);

int GetRoleProID(lua_State* L);

int AddRoleMoney(lua_State* L);

int AddRoleBindMoney(lua_State* L);

int AddRoleExp(lua_State* L);

int AddRoleItem(lua_State* L);

int DeleteItem(lua_State* L);


int AddRoleTaskKillMonster(lua_State* L);

int AddRoleTaskHoldItem(lua_State* L);

int AddRoleTaskUseItem(lua_State* L);

int AddRoleTaskDress(lua_State* L);

int AddRoleTaskCallPet(lua_State* L);

int AddRoleTaskLearnSkill(lua_State* L);

int AddRoleTaskComposeEquip(lua_State* L);

int AddRoleTaskAddFriend(lua_State* L);

int AddRoleTaskAddTeam(lua_State* L);


}


#endif

