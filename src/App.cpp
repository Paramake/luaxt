/**
 * File Name: App.cpp
 * Author(s): Jack Fox
 *
 * App class, (real entrypoint)
 */

#include "App.h"
#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#endif


App::App() : renderCache(nullptr), renderer(nullptr), window(nullptr), L(luaL_newstate())
{
}

App::~App()
{
	if (renderCache != nullptr)
		delete renderCache;

	if (renderer != nullptr)
		delete renderer;

	if (window != nullptr)
		SDL_DestroyWindow(window);

	if (L != nullptr)
		lua_close(L);

	renderCache = nullptr;
	renderer = nullptr;
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

std::string App::getExePath() 
{
	constexpr uint32_t size = 2048;
	std::vector<char> buff(size);

#if _WIN32
	auto len = GetModuleFileName(NULL, &buff[0], size - 1);
	buff[len] = '\0';
#elif __linux__
	char path[512];
	sprintf(path, "/proc/%d/exe", getpid());
	int len = readlink(path, &buf[0], size - 1);
	buf[len] = '\0';
#elif __APPLE__
	unsigned sz = size;
	_NSGetExecutablePath(buf, &sz);
#else
	strcpy(&buf[0], ".");
#endif
	
	return std::filesystem::path(buff.begin(), buff.end())
		.parent_path().string();
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

	renderer = new Renderer(window);
	renderCache = new RenderCache(*renderer);
}

void App::setupLuaState() 
{
	luaL_openlibs(L);
	ApiBridge::InitializeLibs(renderCache, renderer, window, L);

	lua_pushstring(L, "1.0");
	lua_setglobal(L, "VERSION");

	lua_pushstring(L, SDL_GetPlatform());
	lua_setglobal(L, "PLATFORM");

	lua_pushnumber(L, getDPIScale());
	lua_setglobal(L, "SCALE");

	lua_pushstring(L, getExePath().c_str());
	lua_setglobal(L, "EXEDIR");

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

// entry point
int main(int argc, char** argv)
{
	App app;
	app.start(argc, argv);
	return EXIT_SUCCESS;
}