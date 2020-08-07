cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME itiviti-cpp)
project(${PROJECT_NAME})

set(COMPILE_OPTS -Wall -Wextra -Werror -pedantic -pedantic-errors)
set(LINK_OPTS "")

set(CMAKE_CXX_FLAGS "-g")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_INCLUDES ${PROJECT_SOURCE_DIR}/include)
include_directories(${COMMON_INCLUDES})

file(GLOB LIB_FILES ${COMMON_INCLUDES}/*.h)

add_executable(lib main.cpp ${LIB_FILES})

