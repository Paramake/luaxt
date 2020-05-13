/**
 * File Name: App.cpp
 * Author(s): Jack Fox
 *
 * App class, (real entrypoint)
 */

#include "App.h"

#ifdef _WIN32
#include <Windows.h>
#endif


App::App() : window(nullptr), L(luaL_newstate())
{
}

App::~App()
{
	if (window != nullptr)
		SDL_DestroyWindow(window);

	if (L != nullptr)
		lua_close(L);

	window = nullptr;
	L = nullptr;
}

double App::getDPIScale()
{
	float dpi;
	SDL_GetDisplayDPI(0, NULL, &dpi, NULL);
#if _WIN32
	return dpi / 96.0f;
#else
	return 1.0;
#endif
}

void App::createWindow()
{
	if (window != nullptr)
		throw new std::runtime_error("Cannot create window as it already exists");

	SDL_DisplayMode dm;
	SDL_GetCurrentDisplayMode(0, &dm);

	window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		dm.w * 0.8, dm.h * 0.8, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

	// TODO: init window icon
}

void App::setupLuaState() 
{
	luaL_openlibs(L);
	// TODO: init api

	lua_pushstring(L, "1.0");
	lua_setglobal(L, "VERSION");

	lua_pushstring(L, SDL_GetPlatform());
	lua_setglobal(L, "PLATFORM");

	lua_pushnumber(L, getDPIScale());
	lua_pushstring(L, "SCALE");

	// Sanitize Env
	
}

void App::start(int argc, char** argv)
{
#ifdef _WIN32
	SetProcessDPIAware();
#endif

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	SDL_EnableScreenSaver();
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
	std::atexit(SDL_Quit);

#if SDL_VERSION_ATLEAST(2, 0, 5)
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif

	createWindow();
	setupLuaState();

	luaL_dostring(L, InitScript);
	std::getchar();
}