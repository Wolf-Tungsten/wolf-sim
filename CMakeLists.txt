cmake_minimum_required(VERSION 3.15)
project(wolf_sim
        VERSION 0.1
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -fcoroutines -msse2 -mavx2")
set(ASYNC_SIMPLE_ENABLE_TESTS OFF)
set(CMAKE_BUILD_TYPE Debug)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ramulator/src)

set(ASYNC_SIMPLE_BUILD_DEMO_EXAMPLE OFF)


#file(GLOB_RECURSE RAMULATOR_SRC "ramulator/src/*.cpp")
#add_library(ramulator STATIC ${RAMULATOR_SRC})
add_subdirectory(async_simple)
add_subdirectory(wolf_sim)
add_subdirectory(example)

#target_compile_definitions(ramulator PRIVATE RAMULATOR)
