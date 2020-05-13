# luaxt cmake configuration
cmake_minimum_required(VERSION 3.8)

project(luaxt)

if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++17" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++17")
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions("-DHAVE_LIBC")
add_definitions("-DFORCE_STATIC_VCRT=ON")
add_subdirectory(src)
add_subdirectory(LuaJIT)
add_subdirectory(SDL2)