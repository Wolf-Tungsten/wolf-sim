cmake_minimum_required(VERSION 3.1)
project(simulator CXX)

set(CMAKE_BUILD_TYPE Debug)

set(SYSTEMC_HOME /home/gaoruihao/systemc-2.3.4)

set (CMAKE_PREFIX_PATH ${SYSTEMC_HOME})
find_package(SystemCLanguage CONFIG REQUIRED)

set (CMAKE_CXX_STANDARD ${SystemC_CXX_STANDARD} CACHE STRING
     "C++ standard to build all targets. Supported values are 98, 11, 14, and 17.")
set (CMAKE_CXX_STANDARD_REQUIRED ${SystemC_CXX_STANDARD_REQUIRED} CACHE BOOL
     "The with CMAKE_CXX_STANDARD selected C++ standard is a requirement.")

find_package(OpenMP)


add_executable(chiplet_ring chiplet_ring.cpp)
target_link_libraries(chiplet_ring SystemC::systemc)