# luaxt source cmake configuration
file(GLOB_RECURSE luaxt_src "*.h" "*.cpp")
add_executable (luaxt WIN32 ${luaxt_src})
target_link_libraries(luaxt liblua-static SDL2-static)