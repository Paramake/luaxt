#include "ApiBridge.h"

extern Renderer* renderer;
extern RenderCache* renderCache;

static int f_load(lua_State* L) 
{
	auto fileName = luaL_checkstring(L, 1);
	auto size = luaL_checknumber(L, 2);
	auto self = reinterpret_cast<RenFont**>(lua_newuserdata(L, sizeof(RenFont*)));
	*self = renderer->loadFont(fileName, size);
	if (!(*self)) return luaL_error(L, "failed to load font");
	return 1;
}

static int f_set_tab_width(lua_State* L) 
{
	auto self = reinterpret_cast<RenFont**>(luaL_checkudata(L, 1, "Font"));
	auto width = luaL_checknumber(L, 2);
	renderer->setFontTabWidth(*self, width);
	return 0;
}

static int f_gc(lua_State* L)
{
	auto self = reinterpret_cast<RenFont**>(luaL_checkudata(L, 1, "Font"));
	if (*self) renderCache->freeFont(*self);
	return 0;

}

static int f_get_width(lua_State* L)
{
	auto self = reinterpret_cast<RenFont**>(luaL_checkudata(L, 1, "Font"));
	auto text = luaL_checkstring(L, 2);
	lua_pushnumber(L, renderer->getFontWidth(*self, text));
	return 1;
}

static int f_get_height(lua_State* L)
{
	auto self = reinterpret_cast<RenFont**>(luaL_checkudata(L, 1, "Font"));
	lua_pushnumber(L, renderer->getFontHeight(*self));
	return 1;
}


int InitializeFontRenderer(lua_State* L)
{
	const luaL_Reg lib[] =
	{
		{ "__gc",			f_gc            },
		{ "load",			f_load          },
		{ "set_tab_width",	f_set_tab_width },
		{ "get_width",		f_get_width     },
		{ "get_height",		f_get_height    },
		{ NULL,				NULL			}
	};

	luaL_newmetatable(L, "Font");
	luaL_setfuncs(L, lib, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	return 1;
}