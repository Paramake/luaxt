#include "ApiBridge.h"
#include <functional>

extern Renderer* renderer;
extern RenderCache* renderCache;
extern SDL_Window* window;

extern int InitializeLuaRenderer(lua_State* L);
extern int InitializeSystem(lua_State* L);

void ApiBridge::InitializeLibs(RenderCache* renderCacheInst, Renderer* rendererInst, SDL_Window* windowInst, lua_State* L)
{
	renderCache = renderCacheInst;
	renderer = rendererInst;
	window = windowInst;

	const luaL_Reg libs[] = {
		{ "renderer", InitializeLuaRenderer },
		{ "system", InitializeSystem },
		{ NULL, NULL }
	};

	lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    for (auto lib = libs; lib->func; lib++) {
		lua_pushcfunction(L, lib->func);
		lua_pushstring(L, lib->name);
		lua_call(L, 1, 1);
		lua_pushvalue(L, -1);
		lua_setfield(L, -3, lib->name);
    }
}
