cmake_minimum_required(VERSION 3.10)
project(rpc_wine)

# If your Wineprefix is in 64 bit, disable this (-D32BIT=OFF)
option(32BIT "Build rpc_wine as 32-bit library" ON)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRC src/*.cc src/*.hh)

# Wine compiler
#set(CMAKE_C_COMPILER "winegcc")
#set(CMAKE_CXX_COMPILER "wineg++")
#set(CMAKE_RC_COMPILER "wrc")
#set(CMAKE_AR "ar")

if (NOT 32BIT)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./bin64)
else()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./bin32)
endif()

if (NOT CMAKE_BUILD_TYPE)
    message(WARNING "No build type has been selected, defaulting to Debug.")
    set(CMAKE_BUILD_TYPE DEBUG)
endif()

set(CMAKE_CXX_FLAGS "-O3 -Wall -std=c++17") # -mno-cygwin

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g3 -ggdb")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

#include_directories(.)
#add_definitions(discord-rpc.spec)
add_library(rpc_wine SHARED ${SRC})
target_link_libraries(rpc_wine pthread) # odbc32 ole32 oleaut32 winspool odbccp32 uuid
set_target_properties(rpc_wine PROPERTIES OUTPUT_NAME "discord-rpc" PREFIX "" SUFFIX ".dll.so")

if (32BIT)
    set_target_properties(rpc_wine PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
endif()
