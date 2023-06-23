cmake_minimum_required(VERSION 3.15)
project(wolf_sim
        VERSION 0.1
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -fcoroutines -msse2 -mavx2")
set(ASYNC_SIMPLE_ENABLE_TESTS OFF)

add_subdirectory(async_simple)

include_directories(wolf_sim)

add_executable(wolf_sim main.cpp wolf_sim/Environment.cpp wolf_sim/Register.cpp )
target_link_libraries(wolf_sim async_simple aio)