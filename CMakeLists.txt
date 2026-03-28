cmake_minimum_required(VERSION 3.10)
project(pdr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)
add_subdirectory(tools)

enable_testing()
add_subdirectory(test)
