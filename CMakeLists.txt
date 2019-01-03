cmake_minimum_required(VERSION 3.13)
project(escroom C)

set(CMAKE_C_STANDARD 99)

add_executable(escroom_debug main.c commons.c)
add_executable(escroom main.c commons.c)
add_executable(adder adder.c commons.c)
add_executable(mngr mngr.c commons.c)
target_compile_definitions(escroom_debug PRIVATE DEBUG INPUT_DIR="../debug_input")
target_compile_definitions(mngr PRIVATE DEBUG INPUT_DIR="../debug_input")
target_compile_definitions(adder PRIVATE DEBUG INPUT_DIR="../debug_input")
