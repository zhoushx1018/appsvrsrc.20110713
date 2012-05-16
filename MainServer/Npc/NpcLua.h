// NpcÄ£¿é    C API for lua

#ifndef NPCLUA_H
#define NPCLUA_H

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}


#include "NpcSvc.h"

namespace NpcLua
{

#define GLOBAL_NPCSVC_VAR    "__gNpcSvc__"
#define GLOBAL_ROLEID_VAR  "__gRoleID__"
#define GLOBAL_TASKSVC_VAR    "__gTaskSvc__"


void RegisterNpcCAPI(lua_State* L);

#define SET_NPCSVC(l,p)\
	lua_pushlightuserdata(l,p);\
	lua_setfield(l,LUA_GLOBALSINDEX,GLOBAL_NPCSVC_VAR);

#define GET_NPCSVC(l,p)\
	lua_getfield(l,LUA_GLOBALSINDEX,GLOBAL_NPCSVC_VAR);\
	p = (NpcSvc*)lua_touserdata(l,-1);\
	lua_pop(L,1);

#define SET_TASKSVC(l,p)\
	lua_pushlightuserdata(l,p);\
	lua_setfield(l,LUA_GLOBALSINDEX,GLOBAL_TASKSVC_VAR);

#define GET_TASKSVC(l,p)\
	lua_getfield(l,LUA_GLOBALSINDEX,GLOBAL_TASKSVC_VAR);\
	p = (TaskSvc*)lua_touserdata(l,-1);\

#define SET_ROLEID(l,id)\
	lua_pushinteger(l,id);\
	lua_setfield(l,LUA_GLOBALSINDEX,GLOBAL_ROLEID_VAR);

#define GET_ROLEID(l,id)\
	lua_getfield(l,LUA_GLOBALSINDEX,GLOBAL_ROLEID_VAR);\
	id = lua_tointeger(l,-1);\
	lua_pop(L,1);


int GetRoleTaskStatus(lua_State* L);

int GetLuaRetCode(lua_State* L);

}

#endif

