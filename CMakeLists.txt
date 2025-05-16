cmake_minimum_required(VERSION 3.7)

if(NOT WIN32)
    message(FATAL_ERROR "Windows 10/11 only, aborting!" ...)
endif()

project(Dedi)
set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_CONFIGURATION_TYPES "Release;Debug" CACHE STRING "" FORCE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_BUILD_TYPE "Release")
    
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB headers inc/*.h)
file(GLOB sources src/*.c src/*.cpp)
set(headers ${headers})
set(sources ${sources}) 

include_directories(inc)
include_directories(inc/clip)
include_directories(inc/dirent)
include_directories(inc/inih)
include_directories(inc/miniz)
include_directories(inc/raygui)
include_directories(inc/raylib)
include_directories(inc/vdf)

add_executable(Dedi ${headers} ${sources})

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "Dedi")
add_compile_options(/W4)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
target_link_libraries(Dedi ../lib/raylib_x64 ../lib/clip_x64 winmm.lib Urlmon.lib Dwmapi.lib opengl32.lib)
