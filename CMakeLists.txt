cmake_minimum_required(VERSION 3.15)
project(wolf_sim
        VERSION 0.1
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -msse2 -mavx2")
set(ASYNC_SIMPLE_ENABLE_TESTS OFF)

include_directories(${CMAKE_SOURCE_DIR})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")

add_subdirectory(example)

