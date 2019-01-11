cmake_minimum_required(VERSION 3.5)
project(escroom C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-g -Wall")

macro(add_executable _name)
    _add_executable(${ARGV})
    if (TARGET ${_name})
        target_link_libraries(${_name} pthread)
        if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            target_link_libraries(${_name} rt)
        endif ()
    endif ()
endmacro()

add_definitions(-DDEBUG -DENABLE_ASSERT)

add_executable(manager game_server.c messaging.c messaging.h ipc.c ipc.h utils.c utils.h game.h game.c)
add_executable(player game_client.c messaging.c messaging.h ipc.c ipc.h utils.c utils.h game.h game.c)
install(TARGETS DESTINATION .)
