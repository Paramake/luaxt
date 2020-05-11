/**
 * File Name: luaxt.cpp
 * Author(s): Pelanyo M. G. Kamara, Jack Fox
 *
 * Entry Point
 */

#include "luaxt.h"
#include <iostream>
#include <lua.hpp>

int main()
{
    std::cout << "Hello World!" << std::endl;
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);
    luaL_dostring(L, "print'Hello World from Lua!'");
    return 0;
}