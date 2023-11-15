cmake_minimum_required(VERSION 3.15)
project(wolf_sim
        VERSION 0.1
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -fcoroutines -msse2 -mavx2")
set(ASYNC_SIMPLE_ENABLE_TESTS OFF)

include_directories(${CMAKE_SOURCE_DIR})

set(ASYNC_SIMPLE_BUILD_DEMO_EXAMPLE OFF)

add_subdirectory(async_simple)

add_subdirectory(ramulator)

add_subdirectory(wolf_sim)

add_subdirectory(example)

