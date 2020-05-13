#pragma once

#include <iostream>
#include <lua.hpp>
#include <SDL.h>
#undef main

class App 
{
private:
	SDL_Window* window;
	lua_State* L;

	double getDPIScale();

	void createWindow();
	void setupLuaState();

public:
	App();
	~App();

	void start(int argc, char** argv);
};

inline const char* InitScript = R"MULTI(
local core
xpcall(function()
  SCALE = tonumber(os.getenv("LITE_SCALE")) or SCALE
  PATHSEP = package.config:sub(1, 1)
  package.path = EXEDIR .. '/data/?.lua;' .. package.path
  package.path = EXEDIR .. '/data/?/init.lua;' .. package.path
  core = require('core')
  core.init()
  core.run()
end, function(err)
  print('Error: ' .. tostring(err))
  print(debug.traceback(nil, 2))
  if core and core.on_error then
    pcall(core.on_error, err)
  end
  os.exit(1)
end)
)MULTI";