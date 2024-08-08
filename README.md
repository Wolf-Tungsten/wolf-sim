# Wolf-Sim: A Lightweight Cycle-Accurate System-Level Simulation Framework

**轻量化周期精确系统级仿真框架** 

<!-- 目录 -->
- [Wolf-Sim: A Lightweight Cycle-Accurate System-Level Simulation Framework](#wolf-sim-a-lightweight-cycle-accurate-system-level-simulation-framework)
- [简介](#简介)
- [安装](#安装)
- [快速上手教程](#快速上手教程)
  - [导入头文件](#导入头文件)
  - [定义生产者模块](#定义生产者模块)
    - [生产者的端口定义](#生产者的端口定义)
    - [生产者的内部状态定义](#生产者的内部状态定义)
    - [生产者的初始化函数](#生产者的初始化函数)
    - [生产者的状态更新函数](#生产者的状态更新函数)
  - [定义消费者模块](#定义消费者模块)
  - [定义顶层模块](#定义顶层模块)
    - [顶层模块也可以定义端口](#顶层模块也可以定义端口)
    - [子模块定义](#子模块定义)
    - [子模块输入更新函数](#子模块输入更新函数)
    - [顶层模块的状态和输出更新函数](#顶层模块的状态和输出更新函数)
  - [运行仿真](#运行仿真)
    - [设置输入，`tick()`，获取输出](#设置输入tick获取输出)
    - [`tick(n)`](#tickn)
    - [仿真到结束](#仿真到结束)
    - [`reset()`](#reset)
    - [`tickToTermination()`](#ticktotermination)
    - [仿真的输出](#仿真的输出)
- [完整的 Module 定义](#完整的-module-定义)
  - [继承自 wolf\_sim::Module](#继承自-wolf_simmodule)
  - [端口定义](#端口定义)
  - [内部状态定义](#内部状态定义)
  - [子模块定义](#子模块定义-1)
  - [生命周期函数](#生命周期函数)
- [Wolf-Sim 仿真模型的生命周期](#wolf-sim-仿真模型的生命周期)
  - [模型构造阶段](#模型构造阶段)
  - [状态初始化阶段](#状态初始化阶段)
  - [仿真运行阶段](#仿真运行阶段)
  - [仿真终止阶段](#仿真终止阶段)
- [端口与内部状态的访问](#端口与内部状态的访问)
  - [基本类型端口或内部状态的访问](#基本类型端口或内部状态的访问)
  - [复杂类型端口或内部状态的访问](#复杂类型端口或内部状态的访问)
- [子模块动态创建以及参数化](#子模块动态创建以及参数化)
- [仿真模型的运行](#仿真模型的运行)
- [仿真确定性](#仿真确定性)
  
# 简介

Wolf-Sim 是一个以 C++ 类库形式分发的轻量化系统级仿真框架，提供周期精确能力，支持多层次的模块化设计，用于满足数字电路系统架构原型设计过程中的仿真需求。

Wolf-Sim 的定位与 SystemC 类似，但更加简洁易用。

Wolf-Sim 摒弃了基于离散时间优先队列调度事件的仿真方法，提出新的仿真模型构建方法。

# 安装

推荐使用 CMake 来构建 Wolf-Sim 项目，将 Wolf-Sim 作为一个子目录添加到你的项目中。

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

[完整代码](example/consumer_producer.cpp)

## 导入头文件

并且定义常量
    
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
  /* 内部状态定义 */
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

生产者模块的定义包含了：端口定义、内部状态定义、初始化函数和状态更新函数四个部分，下面分别简要介绍。

### 生产者的端口定义

```cpp
public:
  Output(payloadValid, bool);
  Output(payload, int);
  Input(payloadReady, bool);
```

生产者模块面向外部的三个接口，共同构成一个 valid-ready 握手协议。

模块的端口需要在模块外部访问，所以做为public 成员。

`Input` 和 `Output` 是 Wolf-Sim 提供的宏，用于快速、整齐地定义模块的输入和输出端口。

Wolf-Sim 会利用 `Input` 和 `Output` 宏限制端口只在正确的时机被读写。

熟悉 Verilog 的读者可将 Output 类比为 `output reg` 端口，Input 类比为 `input` 端口。



### 生产者的内部状态定义
  
```cpp
private:
  Reg(nextPayload, int);
```

生产者模块的状态只有一个：`nextPayload`，表示下一个要产生的负载。

模块的内部状态不应在模块外部访问，所以做为private 成员。

`Reg` 是 Wolf-Sim 提供的宏，用于定义模块的状态，类比于 Verilog 中时钟同步的寄存器。

同样的，`Reg` 宏限制内部状态只在正确的时机被读写。

### 生产者的初始化函数

```cpp
void init() {
  nextPayload = 0;
  payloadValid = false;
}
```

`init()` 函数由用户定义，并被 Wolf-Sim 框架用于初始化模块的状态，包括初始化输出端口和内部状态。

Wolf-Sim 框架会在**仿真开始前**自动调用模块的 `init()` 函数，用户无需手动调用。

### 生产者的状态更新函数

```cpp
void updateStateOutput() {
  if (payloadValid && payloadReady) {
    logger() << "Producing payload " << payload << std::endl;
    nextPayload = nextPayload + 1;
  }
  payloadValid = true;
  payload = nextPayload;
}
```

`updateStateOutput()` 函数由用户定义，是主要的行为描述函数。该函数根据**输入端口和当前周期的内部状态更新输出端口并计算下一周期内部状态**。

Wolf-Sim 会在每个仿真周期调用模块的 `updateStateOutput()` 函数，并且保证在调用该函数之前，输入端口和内部状态均已更新。

`logger()` 是 Wolf-Sim 提供的日志接口，输出的日志信息将包含模块标签和仿真时间，例如：

```
[producer @ 25] Producing payload 8
```

## 定义消费者模块

```cpp
class Consumer : public wolf_sim::Module {
 public:
  /* 端口定义 */
  Input(payloadValid, bool);
  Input(payload, int);
  Output(payloadReady, bool);

 private:
  /* 内部状态定义 */
  Reg(busyCount, int);

  /* 初始化函数 */
  void init() {
    busyCount = 0;
    payloadReady = true;
  }

  /* 状态和输出更新函数 */
  void updateStateOutput() {
    if (payloadValid && payloadReady) {
      busyCount = PROCESS_DELAY - 1;
      if (payload == TOTAL_PAYLOAD) {
        terminate();
        return;
      }
      logger() << "Consuming payload " << payload << std::endl;
    }
    if (busyCount > 0) {
      busyCount = busyCount - 1;
      payloadReady = false;
    } else {
      payloadReady = true;
    }
  }
};
```

消费者模块的定义与生产者模块类似，只介绍重点差异之处。

消费者模块的端口定义部分需要注意生产者出入方向相反。

消费者模块的状态更新函数中，我们引入了 `terminate()` 函数，该函数通知仿真框架整个系统的仿真已结束。

## 定义顶层模块

```cpp
class Top : public wolf_sim::Module {
 public:
  /* 端口定义 */
  Input(anUselessInput, int);
  Output(anUselessOutput, int);

 private:
  /* 子模块定义 */
  ChildModule(producer, Producer);
  ChildModuleWithLabel(consumer, Consumer, "consumer");  // 为子模块显式设置标签

  /* 子模块输入更新函数 */
  void updateChildInput() {
    consumer->payloadValid = producer->payloadValid;
    consumer->payload = producer->payload;
    producer->payloadReady = consumer->payloadReady;
  }

  /* 状态和输出更新函数 */
  void updateStateOutput() { anUselessOutput = anUselessInput + 47; }
};
```

### 顶层模块也可以定义端口

```cpp
Input(anUselessInput, int);
Output(anUselessOutput, int);
```

这些端口允许外部环境和顶层模块进行交互，使得 Wolf-Sim 可以与其他模拟器框架集成。

在这里，我们定义了两个无用的端口，用于演示顶层模块的端口定义。

### 子模块定义

```cpp
ChildModule(producer, Producer);
ChildModuleWithLabel(consumer, Consumer, "consumer");
```

将 producer 和 consumer 定义为顶层模块的子模块。

ChildModuleWithLabel 允许用户为子模块设置自定义标签，用于在日志中区分不同的子模块。

ChildModule 指定一个默认的和子模块成员同名的标签。

### 子模块输入更新函数

```cpp
void updateChildInput() {
  consumer->payloadValid = producer->payloadValid;
  consumer->payload = producer->payload;
  producer->payloadReady = consumer->payloadReady;
}
```

在 `updateChildInput()` 函数中，用户有机会根据模块当前内部状态和输入端口更新子模块的输入端口。注意，子模块的输出端口也属于父模块的内部状态，可在此处访问。

在这里，我们将生产者的 `payloadValid` 和 `payload` 传递给消费者，将消费者的 `payloadReady` 传递给生产者。

在一个仿真周期内，Wolf-Sim 会首先调用当前模块的 `updateChildInput()` 函数，将子模块的输入准备好；然后，子模块可以进行本周期的仿真；最后，在所有子模块仿真结束后，当前模块调用 `updateStateOutput()` 函数，依据当前模块的输入、内部状态、子模块的输出更新当前模块的输出和内部状态，本周期仿真结束。

### 顶层模块的状态和输出更新函数

```cpp
void updateStateOutput() { anUselessOutput = anUselessInput + 47; }
```

顶层模块没有实际的仿真行为，这里的输出更新是为了演示顶层模块的端口功能。

## 运行仿真

在 main 函数中创建顶层模块对象，并用多种花式方法运行仿真：

```cpp
int main() {
  Top top;
  // tick once;
  top.anUselessInput = 19780823;
  top.tick();
  std::cout << "anUselessOutput: " << top.anUselessOutput << " @ "
            << top.whatTime() << std::endl;
  // tick 10 times;
  top.tick(10);
  // tick to termination;
  while (!top.terminated()) {
    // 可在此设置输入
    top.tick();
    // 可在此处读取输出
  }

  std::cout << ">> reset the model and start again <<" << std::endl;
  // reset
  top.reset();
  // and then another way to tick to termination
  top.tickToTermination();
  return 0;
}
```

### 设置输入，`tick()`，获取输出

每次调用 tick 会推进仿真一个周期。

以下代码部分展示了如何设置输入，推进一个周期，获取输出：

```cpp
  Top top;
  // tick once;
  top.anUselessInput = 19780823;
  top.tick();
  std::cout << "anUselessOutput: " << top.anUselessOutput 
            << " @ " << top.whatTime() << std::endl;
```

在 tick 之前，可设置顶层模块的输入；在 tick 之后，可获取顶层模块的输出。

该设计模式旨在便于 Wolf-Sim 建立的仿真模型与其他仿真框架集成。

`whatTime()` 函数返回当前仿真时间。

上述代码的输出为：

```
anUselessOutput: 19780870 @ 1
```

### `tick(n)`

```cpp
  // tick 10 times;
  top.tick(10);
```

调用一次推进 n 个周期，等价于调用 n 次 tick，但是不修改输入，也不读取输出。

### 仿真到结束

```cpp
while (!top.terminated()) {
  // 可在此处设置输入
  top.tick();
  // 可在此处读取输出
}
```

运行仿真直到结束，可在每周期操作输入和输出。

### `reset()`

如果仿真模型已经终止（terminated 为真），则不能继续调用 tick() 函数，需要调用 `reset()` 函数重置模型，恢复到初始状态。

`tick(n)` 暗含了在仿真终止前停止的条件，即使 n 很大，也不会超过仿真终止。

### `tickToTermination()`

更简洁的运行到仿真终止的方法。

### 仿真的输出

在上述例子中，我们可以看到以下输出：

```
anUselessOutput: 19780870 @ 1
[producer @ 1] Producing payload 0
[consumer @ 1] Consuming payload 0
[producer @ 4] Producing payload 1
[consumer @ 4] Consuming payload 1
[producer @ 7] Producing payload 2
[consumer @ 7] Consuming payload 2
...omit...
[producer @ 55] Producing payload 18
[consumer @ 55] Consuming payload 18
[producer @ 58] Producing payload 19
[consumer @ 58] Consuming payload 19
[producer @ 61] Producing payload 20
>> reset the model and start again <<
[producer @ 1] Producing payload 0
[consumer @ 1] Consuming payload 0
...omit...
[producer @ 58] Producing payload 19
[consumer @ 58] Consuming payload 19
[producer @ 61] Producing payload 20
```

# 完整的 Module 定义

```cpp
class MyModule : public wolf_sim::Module {
 public:
  /* 端口定义 */
  Input(inputPort, int);
  Output(outputPort, int);

 private:
  /* 内部状态定义 */
  Reg(internalState, int);
  /* 子模块定义 */
  ChildModule(childModuleName, ChildModuleType);
  ChildModuleWithLabel(childModuleName, ChildModuleType, "$custom~label");

  /* 构造函数 */
  void construct() {/* 对子模块进行参数化配置 */};

  /* 初始化函数 */
  void init() {/* 初始化内部状态和输出端口 */};

  /* 子模块输入更新函数 */
  void updateChildInput() {
    childModuleName->inputPort = inputPort;
  }

  /* 状态更新函数 */
  void updateStateOutput() {
    outputPort = internalState + inputPort;
    internalState = internalState + 1;
  }
};
```
## 继承自 wolf_sim::Module

所有用户定义的模块都应该继承自 `wolf_sim::Module` 类。

在类定义中，用户根据需要添加端口、内部状态、子模块、构造函数、初始化函数、子模块输入更新函数和状态更新函数。

## 端口定义

Wolf-Sim 中的模块具有输入和输出端口，用于与其他模块在仿真过程中交互。

Wolf-Sim 提供了两个宏 `Input` 和 `Output` 用于定义模块的输入和输出端口。

```cpp
Input(portName, portType);
Output(portName, portType);
```

为了能在模块外部访问端口，端口定义应该在 public 部分。

端口类型要求：

* 任何基础类型，如 int、bool、float 等
* 任何可默认构造、可复制的类型，如 std::vector、std::string 等
* 用户自定义的类型，只要满足上述要求

端口的读写访问将在 [端口与内部状态的访问](#端口与内部状态的访问) 中详细介绍。

## 内部状态定义

Wolf-Sim 提供了 `Reg` 宏用于定义模块的内部状态。

```cpp
Reg(stateName, stateType);
```

内部状态定义应该在 private 部分，不应该在模块外部访问。

内部状态的类型要求与[端口定义](#端口定义)相同。

Module 的内部状态随时可读取，但只能在初始化和状态更新函数中修改。

内部状态的读写访问将在 [端口与内部状态的访问](#端口与内部状态的访问) 中详细介绍。

建议用户总是按照 Mealy 状态机的形式描述模块，即在状态更新函数中根据输入和当前状态计算输出和下一状态。

## 子模块定义

Wolf-Sim 提供了 `ChildModule` 和 `ChildModuleWithLabel` 宏用于定义模块的子模块。

```cpp
ChildModule(childModuleName, ChildModuleType);
ChildModuleWithLabel(childModuleName, ChildModuleType, "$custom~label");
```

使用上述宏定义的子模块会自动加入到仿真模型中，并作为定义模块的子模块，用户无需手动关联。

父模块可设置子模块的输入、读取子模块的输出。

子模块的输出也可以视为父模块的内部状态的一部分。

子模块也可以在父模块的构造函数中进行动态添加，详见[参数化子模块以及动态创建](#子模块动态创建以及参数化)。

## 生命周期函数

模块的生命周期函数包括：

* 构造函数 `construct()`
* 初始化函数 `init()`
* 子模块输入更新函数 `updateChildInput()`
* 状态更新函数 `updateStateOutput()`

将在 [Wolf-Sim 仿真模型的生命周期](#wolf-sim-仿真模型的生命周期) 中详细介绍。

# Wolf-Sim 仿真模型的生命周期

Wolf-Sim 中的模块以树状形式组织，树的根节点是**顶层模块**，一个顶层模块及其包含的所有子模块构成的一棵模块树，称为一个**仿真模型**。

一个仿真程序中可以包含多个仿真模型，除非用户主动协调，每个仿真模型的生命周期都是独立的。

我们将一个仿真模型的生命周期分为以下几个阶段：
1. 模型构造阶段
2. 状态初始化阶段
3. 仿真运行阶段
4. 仿真终止阶段

每个仿真模型建立后都会顺序依次经历这些阶段，调用模型中各个模块的生命周期函数。用户通过在定义模块时重载这些生命周期函数，描述模块的行为，进而描述整个仿真模型的行为。

仿真模型到达终止阶段后，用户可以重置仿真模型，使仿真模型返回到模块构造阶段，然后重新经历整个生命周期。

接下来对各个阶段进行详细介绍。

## 模型构造阶段

在这个阶段，Wolf-Sim 会从**顶层模块**开始，**自顶向下**递归调用每个模块的 `construct()` 生命周期函数。

在 `construct()` 函数中，用户可以动态创建子模块，为子模块进行参数化配置。

Wolf-Sim 会保证父模块的 `construct()` 函数在子模块的 `construct()` 函数之前调用，所以用户可以使用父模块的成员变量来配置子模块。

关于子模块的动态创建和参数化，将在[子模块动态创建以及参数化](#子模块动态创建以及参数化)中详细介绍。

## 状态初始化阶段

完成模型构造阶段后，Wolf-Sim 会**自底向上**递归调用每个模块的 `init()` 生命周期函数。

在 `init()` 函数中，用户可以初始化模块的内部状态和输出端口，设置初始值。

输出端口可类比于 Verilog 中 output reg 端口，因此也可以被初始化。

`init()` 函数自底向上调用的顺序确保了子模块的初始化在父模块之前完成，因此父模块在初始化时，可以依赖子模块的输出端口。

## 仿真运行阶段

从用户视角看，模型构造和状态初始化阶段是自动完成的，用户无需手动调用，完成上述两个阶段后，仿真模型准备好开始运行。

用户此时可以设置顶层模块的输入端口，然后调用 `tick()` 函数运行一个周期的仿真。

为了便于理解，我们可以视作模型中的每一个模块都有一个 `tick()` 函数，该函数的流程如下：

1. 调用用户重载定义的 `updateChildInput()` 生命周期函数，在该函数中，用户可根据当前模块的**内部状态**和**输入端口**，更新子模块的输入端口。
2. 调用所有子模块的 `tick()` 函数，子模块仿真推进到本周期结束。
3. 此时，子模块的输出端口均已更新，调用用户重载定义的 `updateStateOutput()` 生命周期函数，在该函数中，用户可根据当前模块的**内部状态**、**输入端口**和**子模块的输出端口**，更新当前模块的**输出端口**和**内部状态**。

## 仿真终止阶段

在仿真运行阶段，用户可在任意一个模块中 `updateStateOutput()` 函数中调用 `terminate()` 函数，通知 Wolf-Sim 框架该模型的仿真已结束，在顶层模块的 `tick()` 函数返回后，模型进入仿真终止阶段。

处于仿真终止阶段的模型调用 `terminated()` 函数会返回 true，继续调用 tick() 方法会发生运行时错误，该机制保障用户不会错误的推进已终止的模型。

已终止的仿真模型如需重新运行仿真，用户需要调用 `reset()` 函数，将模型恢复到状态初始化阶段。

# 端口与内部状态的访问

Wolf-Sim 提供了 `Input`、`Output` 和 `Reg` 宏用于定义模块的输入、输出端口和内部状态。

这些宏会自动为用户定义的模块生成对应的成员变量，用户可以通过这些成员变量访问端口和内部状态。

同时，Wolf-Sim 会对端口和内部状态的修改加以限制，保证在正确的时机修改端口和内部状态，而非法的修改会导致运行时错误。

下面以 `Reg` 为例进行讲解，假设定义时包含了以下定义：

```
Reg(myIntState, int);
Reg(myBoolState, bool);
Reg(myVectorState, std::vector<int>);
Reg(myCustomState, MyCustomType);
```

## 基本类型端口或内部状态的访问

在生命周期函数中，基本类型的端口和内部状态访问几乎可以按照原类型的方式进行，例如：

```cpp
if(myBoolState) {
  myIntState = 1;
}
```

但是++、--、+=、-=、*=、/=、%=、&=、|=、^=、<<=、>>=等运算符尚不支持，需要使用赋值运算符。

```cpp
myIntState = myIntState + 1;
```

## 复杂类型端口或内部状态的访问

可以分别通过 `r()` 和  `w()` 方法获取端口或内部状态的只读和可写引用。

```cpp
if(myVectorState.r().size() < 10) {
  myVectorState.w().push_back(1);
}

myCustomState.r().someReadMethod();
myCustomState.w().someWriteMethod();
```

# 子模块动态创建以及参数化

为了更好地支持模块化设计以及设计空间探索，Wolf-Sim 提供了动态创建子模块的功能。

[一个例子](example/consumer_producer_dynamic.cpp)

在上述例子中，消费者模块的构造函数具有两个参数，因此不能使用 `ChildModule` 宏直接定义。

```cpp
class Consumer : public wolf_sim::Module {
 public:
  Consumer(int maxPayload, int processDelay)
      : maxPayload(maxPayload), processDelay(processDelay) {};
      // ...
}
```

在顶层模块的 construct 函数中，可以动态创建消费者模块，并为其传递参数。

```cpp
class Top : public wolf_sim::Module {
  // ...
  std::shared_ptr<Consumer> consumer;

  void construct() {
    consumer = std::make_shared<Consumer>(TOTAL_PAYLOAD, PROCESS_DELAY);
    addChildModule(consumer);
    consumer->setModuleLabel("dynamic consumer");
  }
  // ...
};
```

在 construct() 函数中，用户需要使用 `std::make_shared` 函数将子模块初始化成智能指针，并使用 `addChildModule` 函数将子模块添加到父模块中。

如需设置标签，可以使用 `setModuleLabel` 函数。

此外，还需要在顶层模块中持有子模块的智能指针，以保证子模块能够在其他生命周期函数中被访问。

注意，子模块的动态创建仅能在 construct 函数中进行，不能在其他生命周期函数中进行。

# 仿真模型的运行

Wolf-Sim 提供了多种仿真模型运行的方法，用户可以根据需求选择合适的方法。

最简单的运行模式如下：

```cpp  
Top top;
while(!top.terminated()) {
  top.inputPort = someInput;
  top.tick();
  someOutput = top.outputPort;
}
```

设置输入、仿真一拍、获取输出，循环直到仿真终止。

如果不关心顶层模块的输入和输出，可以使用 `tickToTermination()` 函数：

```cpp
Top top;
top.tickToTermination();
```

如果不关心顶层模块的输入和输出，但只仿真固定时长，可以使用 `tick(n)` 函数：

```cpp
Top top;
top.tick(100);
```

如果需要重置模型，可以使用 `reset()` 函数：

```cpp
Top top;
top.tickToTermination();
top.reset();
// 重新运行
top.tickToTermination();
```

# 仿真确定性

Wolf-Sim 会尝试利用自动多线程机制，以提高仿真运行效率，但是这可能会导致仿真结果的不确定性。

考虑到一些特殊的场景下，用户可能需要保证仿真结果的确定性，Wolf-Sim 提供了 `setDeterministic()` 函数，启用仿真确定性。

```cpp
Top top;
top.setDeterministic(true);
top.tickToTermination();
```

顶层模块的 `setDeterministic()` 函数需要在首次调用 `tick()` 函数之前或者 `reset()` 后调用。

内部模块可在 `construct()` 函数中调用 `setDeterministic()` 函数，以保证其子模块的仿真确定性。


