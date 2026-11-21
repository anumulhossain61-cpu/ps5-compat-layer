cmake_minimum_required(VERSION 3.24)
project(PS5CompatLayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com
    GIT_TAG release-3.2.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

file(DOWNLOAD
    https://githubusercontent.com
    ${CMAKE_BINARY_DIR}/stb_image.h
)

add_executable(ps5emu main.cpp)
target_include_directories(ps5emu PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(ps5emu PRIVATE SDL3::SDL3)
