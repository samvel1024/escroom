cmake_minimum_required(VERSION 3.5)
project(escroom)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "-Wall -Wextra -O3")

macro(add_executable _name)
    _add_executable(${ARGV})
    if (TARGET ${_name})
        target_link_libraries(${_name} pthread)
        if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            target_link_libraries(${_name} rt)
        endif ()
    endif ()
endmacro()

#add_definitions(-DDEBUG)
#add_definitions(-DENABLE_ASSERT)

add_executable(manager game_server.c messaging.c messaging.h ipc.c ipc.h utils.c utils.h game.h game.c)
add_executable(player game_client.c messaging.c messaging.h ipc.c ipc.h utils.c utils.h game.h game.c)
add_executable(testgen testgen.cpp)
install(TARGETS DESTINATION .)
