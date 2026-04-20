cmake_minimum_required(VERSION 3.16)
project(isa_assembler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(isa_tool
    src/main.cpp
    src/cpu.cpp
    src/assembler.cpp
)

target_include_directories(isa_tool PRIVATE include)

if(MSVC)
    target_compile_options(isa_tool PRIVATE /W4 /permissive-)
else()
    target_compile_options(isa_tool PRIVATE -Wall -Wextra -Wpedantic)
endif()
