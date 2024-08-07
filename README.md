# Wolf-Sim: 一个轻量化周期精确系统级仿真框架 A Lightweight Cycle-Accurate System-Level Simulation Framework


<!-- 目录 -->
- [Wolf-Sim: 一个轻量化周期精确系统级仿真框架 A Lightweight Cycle-Accurate System-Level Simulation Framework](#wolf-sim-一个轻量化周期精确系统级仿真框架-a-lightweight-cycle-accurate-system-level-simulation-framework)
- [简介](#简介)
- [安装](#安装)
- [快速上手教程](#快速上手教程)
  
# 简介

Wolf-Sim 是一个以 C++ 类库形式分发的轻量化系统级仿真框架，提供周期精确能力，支持多层次的模块化设计，用于支持数字电路系统架构的设计过程。

Wolf-Sim 的定位与 SystemC 类似，但更加简洁易用，并且提供仿真性能优化的可能性。

# 安装

我们推荐使用 CMake 来构建 Wolf-Sim 项目，将 Wolf-Sim 作为一个子目录添加到你的项目中。

添加为 git 子模块：

```
git submodule add https://github.com/Wolf-Tungsten/wolf-sim wolf_sim
git submodule update --init --recursive
```

在主项目的 CMakeList.txt 中添加子目录：
```
# cmake_minimum_required(VERSION 3.10)
# project(YourProject)

# 添加子模块目录
add_subdirectory(wolf_sim)

# 添加可执行文件
add_executable(main_executable main.cpp)

# 链接子模块库
target_link_libraries(main_executable PRIVATE wolf_sim)
```

# 快速上手教程

在这个例子中，我们将展示一个生产者-消费者系统的设计过程。

## 导入头文件
    
    ```cpp
    #include "wolf_sim.h"
    ```

## 定义生产者模块

    ```cpp
    
    ```

