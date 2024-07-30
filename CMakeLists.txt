cmake_minimum_required(VERSION 3.15)
project(wolf_sim
        VERSION 1.0
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ramulator/src)

set(ASYNC_SIMPLE_BUILD_DEMO_EXAMPLE OFF)

# CMakeLists.txt
option(USE_ASAN "Enable AddressSanitizer" YES)

if (USE_ASAN)
    message(STATUS "AddressSanitizer enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
endif ()

#file(GLOB_RECURSE RAMULATOR_SRC "ramulator/src/*.cpp")
#add_library(ramulator STATIC ${RAMULATOR_SRC})
add_subdirectory(wolf_sim)
add_subdirectory(example)

#target_compile_definitions(ramulator PRIVATE RAMULATOR)
