# Wolf-Sim: 一个轻量化周期精确系统级仿真框架 A Lightweight Cycle-Accurate System-Level Simulation Framework


<!-- 目录 -->
- [Wolf-Sim: 一个轻量化周期精确系统级仿真框架 A Lightweight Cycle-Accurate System-Level Simulation Framework](#wolf-sim-一个轻量化周期精确系统级仿真框架-a-lightweight-cycle-accurate-system-level-simulation-framework)
- [简介](#简介)
- [安装](#安装)
- [快速上手教程](#快速上手教程)
  - [导入头文件](#导入头文件)
  - [定义生产者模块](#定义生产者模块)
  
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

在这个例子中，我们将展示一个生产者-消费者系统在 Wolf-Sim 中的建模方法。

该系统的行为描述如下：
* 生产者每周期产生 TOTAL_PAYLOAD 个负载（用整数表示）
* 消费者处理一个负载所需的时间为 PROCESS_DELAY 个周期
* 消费者处理负载期间，生产者被阻塞，不会产生新的负载
* 生产者和消费者之间通过一对 valid-ready 信号握手

我们将建立三个模块：
* Producer：生产者模块
* Consumer：消费者模块
* Top：顶层模块



## 导入头文件

并且定义一些常量
    
```cpp
#include "wolf_sim.h"

const int TOTAL_PAYLOAD = 20;
const int PROCESS_DELAY = 3;
```

## 定义生产者模块

```cpp
class Producer : public wolf_sim::Module {
 public:
  /* 端口定义 */
  Output(payloadValid, bool);
  Output(payload, int);
  Input(payloadReady, bool);

 private:
  /* 状态定义 */
  Reg(nextPayload, int);

  /* 初始化函数 */
  void init() {
    nextPayload = 0;
    payloadValid = false;
  }

  /* 状态更新函数 */
  void updateStateOutput() {
    if (payloadValid && payloadReady) {
      logger() << "Producing payload " << payload << std::endl;
      nextPayload = nextPayload + 1;
    }
    payloadValid = true;
    payload = nextPayload;
  }
};
```


