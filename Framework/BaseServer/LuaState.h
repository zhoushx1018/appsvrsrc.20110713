#ifndef LUASTATE_H
#define LUASTATE_H

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <pthread.h>
#include "Log.h"
#include <string>

using namespace std;


typedef void( * REGISTERCAPI)(lua_State* L);

class LuaState
{
public:
	LuaState();

	~LuaState();

	lua_State* GetState(REGISTERCAPI rCAPI);

	static int Do(lua_State* state,const string& script);
	
	static const char* GetErrorMsg(lua_State* state);
private:
	static void OnDelete(void* data);
	
private:
	pthread_key_t _key;
};

#endif

