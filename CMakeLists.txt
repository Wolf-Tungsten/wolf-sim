cmake_minimum_required(VERSION 3.15)
project(wolf_sim
        VERSION 1.0
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

# 设置库的版本号
set(WOLF_SIM_VERSION_MAJOR 1)
set(WOLF_SIM_VERSION_MINOR 0)

# 配置项目以支持安装和导出
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 添加库源文件
file(GLOB_RECURSE WOLF_SIM_SRC "src/*.cpp")
add_library(wolf_sim ${WOLF_SIM_SRC})

# 添加包含目录
target_include_directories(wolf_sim PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# 安装目标和导出设置
install(TARGETS wolf_sim
    EXPORT wolf_simTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 安装头文件
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 导出目标
install(EXPORT wolf_simTargets
    FILE wolf_simTargets.cmake
    NAMESPACE wolf_sim::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wolf_sim
)

# 生成配置文件
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/wolf_simConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/wolf_simConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wolf_sim
)

# 安装配置文件
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/wolf_simConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/wolf_sim
)

# 如果当前项目是顶层项目，则添加 example 目录
if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    add_subdirectory(example)
endif()

# CMakeLists.txt
# option(USE_ASAN "Enable AddressSanitizer" YES)

# if (USE_ASAN)
#     message(STATUS "AddressSanitizer enabled")
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
#     set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
# endif ()

#file(GLOB_RECURSE RAMULATOR_SRC "ramulator/src/*.cpp")
#add_library(ramulator STATIC ${RAMULATOR_SRC})
# add_subdirectory(wolf_sim)
# add_subdirectory(example)

#target_compile_definitions(ramulator PRIVATE RAMULATOR)
