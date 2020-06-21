#pragma once

#include <lua.hpp>
#include <functional>

#include "../rendering/RenderCache.h"
#include "../rendering/Renderer.h"

namespace ApiBridge
{
	void InitializeLibs(RenderCache* renderCache, Renderer* render, SDL_Window* window, lua_State* L);
}