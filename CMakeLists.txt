cmake_minimum_required(VERSION 3.15)
project(wolf_sim
        VERSION 0.1
        LANGUAGES CXX)

add_subdirectory(async_simple)

add_executable(wolf_sim main.cpp)
target_link_libraries(wolf_sim PRIVATE async_simple)