cmake_minimum_required(VERSION 3.5)
project(escroom C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "-g -Wall")

macro(add_executable _name)
    _add_executable(${ARGV})
    if (TARGET ${_name})
        target_link_libraries(${_name} utils game pthread)
        if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            target_link_libraries(${_name} rt)
        endif ()
    endif ()
endmacro()

add_definitions(-DDEBUG -DENABLE_ASSERT)


add_library(utils utils.c)
add_library(ipc ipc.c)
add_library(game game.c)
add_library(messaging messaging.c)
add_executable(manager game_server.c)
add_executable(player game_client.c)
add_executable(simpletest test.c)

install(TARGETS DESTINATION .)
