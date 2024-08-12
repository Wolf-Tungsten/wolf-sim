cmake_minimum_required(VERSION 3.10)

# 项目名称
project(Wolf_Sim_SystemC_Baseline)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_OSX_ARCHITECTURES "arm64")

set(CMAKE_BUILD_TYPE Release)

# 保留原有的 CMAKE_PREFIX_PATH 并添加新的路径
list(APPEND CMAKE_PREFIX_PATH "/Users/wolf/Develop/systemc-3.0.0")
find_package(SystemCLanguage CONFIG REQUIRED)

# 添加源文件
add_executable(mesh2d mesh2d.cpp)

# 链接 SystemC 库
target_link_libraries(mesh2d PRIVATE SystemC::systemc)