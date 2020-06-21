// TODO: refactor and make code better

#include "ApiBridge.h"
#include "../rendering/RenderCache.h"

Renderer* renderer;
RenderCache* renderCache;

static RenColor check_color(lua_State* L, int idx, int def) 
{
	RenColor color;
	if (lua_isnoneornil(L, idx)) {
		auto color_def = static_cast<uint8_t>(def);
		return RenColor { color_def, color_def, color_def, 255 };
	}
	lua_rawgeti(L, idx, 1);
	lua_rawgeti(L, idx, 2);
	lua_rawgeti(L, idx, 3);
	lua_rawgeti(L, idx, 4);
	color.r = luaL_checknumber(L, -4);
	color.g = luaL_checknumber(L, -3);
	color.b = luaL_checknumber(L, -2);
	color.a = luaL_optnumber(L, -1, 255);
	lua_pop(L, 4);
	return color;
}

static int f_show_debug(lua_State* L)
{
	luaL_checkany(L, 1);
	renderCache->showDebug(lua_toboolean(L, 1));
	return 0;
}

static int f_get_size(lua_State* L)
{
	int w, h;
	renderer->getSize(w, h);
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	return 2;
}

static int f_begin_frame(lua_State* L)
{
	renderCache->beginFrame();
	return 0;
}

static int f_end_frame(lua_State* L)
{
	renderCache->endFrame();
	return 0;
}

static int f_set_clip_rect(lua_State* L)
{
	RenRect rect;
	rect.x = luaL_checknumber(L, 1);
	rect.y = luaL_checknumber(L, 2);
	rect.width = luaL_checknumber(L, 3);
	rect.height = luaL_checknumber(L, 4);
	renderCache->setClipRect(rect);
	return 0;
}

static int f_draw_rect(lua_State* L)
{
	RenRect rect;
	rect.x = luaL_checknumber(L, 1);
	rect.y = luaL_checknumber(L, 2);
	rect.width = luaL_checknumber(L, 3);
	rect.height = luaL_checknumber(L, 4);
	
	auto color = check_color(L, 5, 0xFF);
	renderCache->drawRect(rect, color);
	
	return 0;
}

static int f_draw_text(lua_State* L)
{
	RenFont** font = reinterpret_cast<RenFont**>(luaL_checkudata(L, 1, "Font"));
	auto text = luaL_checkstring(L, 2);
	auto x = luaL_checknumber(L, 3);
	auto y = luaL_checknumber(L, 4);
	auto color = check_color(L, 5, 0xFF);
	x = renderCache->drawText(*font, text, x, y, color);
	lua_pushnumber(L, x);
	return 1;
}


extern int InitializeFontRenderer(lua_State* L);
int InitializeLuaRenderer(lua_State* L)
{
	const luaL_Reg lib[] = 
	{
		{ "show_debug",		f_show_debug	},
		{ "get_size",		f_get_size		},
		{ "begin_frame",	f_begin_frame	},
		{ "end_frame",		f_end_frame		},
		{ "set_clip_rect",	f_set_clip_rect	},
		{ "draw_rect",		f_draw_rect		},
		{ "draw_text",		f_draw_text		},
		{ NULL,				NULL			},
	};

	luaL_newlib(L, lib);
	InitializeFontRenderer(L);
	lua_setfield(L, -2, "font");
	return 1;
}

