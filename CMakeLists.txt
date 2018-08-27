cmake_minimum_required(VERSION 3.7)

project(posti)

add_subdirectory(deps)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(src)

option(BUILD_TESTING "Enable testing" ON)

add_subdirectory(test)