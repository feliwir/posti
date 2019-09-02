cmake_minimum_required(VERSION 3.7)

project(posti)
set(POSTI_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

option(BUILD_TESTING "Enable testing" ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

add_subdirectory(deps)
add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()