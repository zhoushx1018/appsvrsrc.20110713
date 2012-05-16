#include "LuaState.h"
#include <errno.h>

LuaState::LuaState()
{
	if(pthread_key_create(&_key,OnDelete))
	{
		LOG(LOG_ERROR,__FILE__,__LINE__,"%s",strerror(errno));
	}
}

LuaState::~LuaState()
{
	pthread_key_delete(_key);
}

lua_State* LuaState::GetState(REGISTERCAPI rCAPI )
{
	lua_State* state = (lua_State*)pthread_getspecific(_key);
	if(state==NULL)
	{
		state = luaL_newstate();    //初始化lua
		luaL_openlibs(state);
		pthread_setspecific(_key,state);

 		(*rCAPI)(state);
	}

	return state;
}

int LuaState::Do(lua_State* state,const string& script)
{
	if(script.size()==0) return 0;

	bool err = luaL_dostring(state,script.c_str());
	if(err)
	{
		//LOG(LOG_ERROR,__FILE__,__LINE__,"%s",lua_tostring(state, -1));
		//弹出错误信息所在的最上层栈
		//lua_pop(state, 1);

		return err;
	}

	lua_getglobal(state,"fun");

	return lua_pcall(state,0,LUA_MULTRET,0);
}

const char* LuaState::GetErrorMsg(lua_State* state) 
{
	return lua_tostring(state, -1);
}

void LuaState::OnDelete(void* data)
{
	lua_State* state = (lua_State*)data;
	lua_close(state);
}


