cmake_minimum_required(VERSION 3.7)

project(posti)
set(POSTI_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(deps)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_subdirectory(src)

option(BUILD_TESTING "Enable testing" ON)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()