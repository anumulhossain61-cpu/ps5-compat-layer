cmake_minimum_required(VERSION 3.24)
project(PS5CompatLayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ps5emu WIN32 main.cpp)
target_link_libraries(ps5emu PRIVATE gdiplus)
