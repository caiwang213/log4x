cmake_minimum_required(VERSION 2.8)
project(log4x)

message(STATUS "cmake default debug CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "cmake default release CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_RELEASE}")

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
#SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

option(BUILD_SHARED_LIBS "Build liblog4x as a shared library." OFF)
#option(BUILD_SHARED_LIBS "Build liblog4x as a shared library." ON)
option(BUILD_STATIC_LIBS "Build liblog4x static library." ON)

# PROJECT_BINARY_DIR and PROJECT_SOURCE_DIR are the variables of cmake
set(LOG4X_INC_DIR ${PROJECT_SOURCE_DIR}/include)
set(LOG4X_LIB_DIR ${PROJECT_SOURCE_DIR}/lib)
set(LOG4X_SRC_DIR ${PROJECT_SOURCE_DIR}/src)

if(NOT MSVC)
    #set(CMAKE_CXX_FLAGS "-std=c++11 -pthread ${CMAKE_CXX_FLAGS}")
    add_definitions(-std=c++11 -pthread)
endif()

message(STATUS "C++ flags have ${CMAKE_CXX_FLAGS}")

add_subdirectory(src)
add_subdirectory(test)
