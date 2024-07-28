# Wolf-Sim：一个C++并行离散事件框架

- [Wolf-Sim：一个C++并行离散事件框架](#wolf-sim一个c并行离散事件框架)
  - [快速认识三个基础组件](#快速认识三个基础组件)
    - [Module](#module)
    - [Register](#register)
    - [Environment](#environment)
  - [一个手把手教程](#一个手把手教程)
    - [导入头文件](#导入头文件)
    - [从定义生产者模块开始](#从定义生产者模块开始)
    - [定义消费者模块](#定义消费者模块)
    - [定义顶层模块](#定义顶层模块)
    - [启动仿真](#启动仿真)
  - [复制这些代码创建你的模块](#复制这些代码创建你的模块)
  - [复制这些代码，创建你的 main 函数](#复制这些代码创建你的-main-函数)
  - [Module 的端口系统](#module-的端口系统)
    - [端口的读写](#端口的读写)
      - [基本的读写操作](#基本的读写操作)
      - [端口延迟写操作](#端口延迟写操作)
    - [端口的连接](#端口的连接)
      - [通过寄存器的端口连接](#通过寄存器的端口连接)
      - [不通过寄存器的端口连接](#不通过寄存器的端口连接)
      - [如何理解 ToChildPort 和 FromChildPort](#如何理解-tochildport-和-fromchildport)
    - [端口组](#端口组)
  - [Module 的 `fire()` 方法](#module-的-fire-方法)
    - [查看当前仿真时间](#查看当前仿真时间)
    - [向输出端口写数据](#向输出端口写数据)
    - [Module 的计划唤醒](#module-的计划唤醒)
    - [终止仿真](#终止仿真)
  - [Module 的 `construct()` 方法](#module-的-construct-方法)
    - [创建子模块](#创建子模块)
    - [创建寄存器](#创建寄存器)
  - [Module 的 `finalStop()` 方法](#module-的-finalstop-方法)

## 快速认识三个基础组件

### Module

Wolf-Sim 构造的仿真系统是由多个 Module 子类的实例组成的：

1. Module 可以定义输入端口和输出端口，用于与其他 Module 交换数据；
2. Module 在 `fire()` 方法中描述自己的行为；
3. Module 可递归地包含 **子 Module** ，在 `construct()` 方法中创建 **子 Module** 并描述 **子 Module** 之间的连接关系；
4. 一个仿真系统中的 Module 呈现树状结构，根节点称为 **顶层 Module**。

Wolf-Sim 中的 Module 可类比于 Verilog 中的模块，或 SystemC 中的模块。

### Register

Register 建立 Module 之间的连接，完成数据交换、时间同步。

如果一个 Register R 连接了 Module A 的输出端口和 Module B 的输入端口：

* Register R 能且仅能被 Module A 写入数据，能且仅能被 Module B 读取数据；
* 那么，Module A 写入到 R 的数据**一定会被 Module B 读取到**，但从仿真行为的角度看，用户逻辑可以选择忽略；
* Register **为空时会阻塞** Module B 的读取操作，直到 Register 中有数据；Register **永远不会阻塞** Module A 的写入操作；因此，Register 可以视为一个无限容量的 FIFO 队列。

### Environment

包装了仿真系统确立、仿真线程启动等功能细节，使得用户可以专注于模型的构建。

将 **顶层 Module** 添加到 Enivronment，然后调用 run() 即可开始仿真。

## 一个手把手教程

[完整代码](example/consumer_producer.cpp)

我们将用 Wolf-Sim 构造一个支持反压的生产者-消费者模型。

该模型包含三个模块：Producer，Consumer 和 Top。

Producer 检查 Consumer 是否准备好接收 payload，如果确认 Consumer 准备好，则发送 payload （一个int整数）。

Consumer 每接收一个 payload 就要花 10 个单位时间进行处理（打印到控制台），处理期间 Consumer 不接受输入；当 Consumer 可以接受输入时，会向 Producer 发送 ready 信号；收到 10 个 payload 之后仿真终止。

Top 是顶层模块，描述了 Producer 和 Consumer 之间的连接关系。

### 导入头文件

```cpp
#include "wolf_sim.h"
```

### 从定义生产者模块开始

```cpp
class Producer : public wolf_sim::Module {
 public:
  /* 端口定义部分 */
  IPort(readyIPort, bool);
  OPort(payloadOPort, int);

 private:
  /* 根据需要定义的状态 */
  int payload;
  /* 描述模块行为的 fire 函数 */
  void fire() {
    if (readyIPort.valid()) {
      /* 判断 readyIPort 上是否有效输入 */
      std::cout << "Producer got ready signal at " << whatTime()
              << " and sent payload " << payload << std::endl;
      payloadOPort << payload;
      payload++;
    }
  };
};
```

* 首先，进行端口定义，Producer 有一个输入端口 readyIPort 和一个输出端口 payloadOPort：
     * 我们使用 IPort 和 OPort 宏来定义端口，第一个参数指定端口的名称，第二个参数指定端口的数据类型；
     * 端口的数据类型可以是任何支持拷贝构造的类型；
     * 注意 IPort 和 OPort 都需要标记为 public，否则无法在外层模块中连接。

* 然后，我们根据仿真行为的需要定义了一个状态 payload。

* 最后，我们重载了 fire 函数，描述模块的行为：
     * fire 函数在三种情况下被仿真内核调用，一是仿真的 0 时刻，二是在模块的输入端口有数据到达时，三是在模块被计划唤醒时，**在 Producer 中我们只关心第二种情况**；
     * 在 fire 中，我们检查 readyIPort 是否有有效输入，如果有，我们打印当前时间和 payload 的值，并将 payload 发送到 payloadOPort，然后 payload 自增。

### 定义消费者模块

```cpp
class Consumer : public wolf_sim::Module {
 public:
  /* 端口定义部分 */
  IPort(payloadIPort, int);
  OPort(readyOPort, bool);

 private:
  /* 根据仿真行为需要自定义的状态 */
  bool pending = false;
  wolf_sim::Time_t doneTime = 0;
  int pendingPayload;
  /* 描述模块行为的 fire 函数 */
  void fire() {
    if (!pending) {
      /* 没有正在处理的任务 */
      if (payloadIPort >> pendingPayload) {
        /* 有新的 payload */
        pending = true;
        doneTime = whatTime() + 10; 
        /* 记录处理完成时间 */
        std::cout << "...payload msg...";
        planWakeUp(10); 
        /* 计划 10 个时间单位后唤醒 */
      } else {
        /* 没有新的 payload，发出 ready 信号 */
        std::cout << "...ready msg...";
        readyOPort << true;
      }
    } else {
      /* 有正在处理的任务 */
      if (whatTime() == doneTime) {
        /* 到达处理完成时间 */
        std::cout << "...complete msg...";
        pending = false; 
        /* 模拟处理完成 */
        if (pendingPayload == 10) {
          /* 如果处理的是最后一个任务，调用 terminate() 仿真终止 */
          std::cout << "...terminate msg...";
          terminate();
        } else {
          /* 否则继续发出 ready 信号通知 Producer */
          readyOPort << true;
          std::cout << "...ready msg...";
        }
      } else {
        /* 还未到处理完成时间，即使有新的 payload 也不处理 */
      }
    }
  }
};
```

* 消费者模块的端口定义部分不再赘述，注意方向和生产者相对。

* 消费者模块的自定义状态部分包括 pending、doneTime 和 pendingPayload，分别表示是否有正在处理的任务、处理完成时间和正在处理的任务的 payload。

* 为了文档简洁，fire 部分的输出信息被省略，可参阅完整代码。

* 在生产者模块介绍部分我们提到，fire 函数在三种情况下被调用，在消费者模块中，我们会遇到这三种情况：

  * 仿真的 0 时刻；
  * 模块的输入端口有数据到达时；
  * 模块被计划唤醒时。

* 消费者模块 fire 函数启动后，首先通过 pending 变量判断检查是否有正在处理的任务，如果没有正在处理的任务，则执行以下代码，我们需要认真分析一下：

  ```cpp
        if (payloadIPort >> pendingPayload) {
          /* 有新的 payload */
          pending = true;
          doneTime = whatTime() + 10; 
          /* 记录处理完成时间 */
          std::cout << "...payload msg...";
          planWakeUp(10); 
          /* 计划 10 个时间单位后唤醒 */
        } else {
          /* 没有新的 payload，发出 ready 信号 */
          std::cout << "...ready msg...";
          readyOPort << true;
        }
  ```

  * `if (payloadIPort >> pendingPayload) {...}` 利用了 Wolf-Sim 中标准的输入读取方法，该方法通过重载的 `>>` 操作符实现。如果 payloadIPort 上有数据到达，那么 pendingPayload 将被赋值为 payloadIPort 上的数据，并且表达式返回 true。否则，pendingPayload 变量值不会被修改，表达式返回 false。也就意味着，当输入数据有效时，我们会进入到这个分支。
  * 你可能注意到，在 [Producer](#从定义生产者模块开始) 中，我们使用的不是 `>>` 操作符，而是 `readyIPort.valid()` 函数。这是 Wolf-Sim 对于 bool 型输入端口的特殊处理，因为 bool 型输入端口只有两种状态，所以我们不需要读取具体的数据，只需要知道端口是否有数据到达即可。
  * `doneTime = whatTime() + 10; ` 在 fire 函数中，`whatTime()` 函数返回当前仿真时间。我们记录下当前时间加上 10 个时间单位，表示处理完成时间。注意，doneTime 只是这个仿真模型中的自定义状态，不会影响仿真内核的行为。
  * 真正确保 10 个时间单位后会再次 fire 的动作是 `planWakeUp(10)`。
  * 如果 payloadIPort 上没有新的 payload，那么我们会进入 else 分支，在这里，我们向 readyOPort 发送 ready 信号，通知 Producer 模块可以发送 payload。
  * 从 fire 函数调用的时机角度分析，当 fire 是在输入端口有数据到达时被调用，我们会进入 if 分支；如果是在仿真的 0 时刻调用，既没有正在处理的数据，也没有新的 payload 到达，我们会进入 else 分支。

* 如果有正在处理的任务，我们会执行以下代码：

  ```cpp
        if (whatTime() == doneTime) {
          /* 到达处理完成时间 */
          std::cout << "...complete msg...";
          pending = false; 
          /* 模拟处理完成 */
          if (pendingPayload == 10) {
            /* 如果处理的是最后一个任务，调用 terminate() 仿真终止 */
            std::cout << "...terminate msg...";
            terminate();
          } else {
            /* 否则继续发出 ready 信号通知 Producer */
            readyOPort << true;
            std::cout << "...ready msg...";
          }
        } else {
          /* 还未到处理完成时间，即使有新的 payload 也不处理 */
          throw std::runtime_error("Consumer should not have new payload");
        }
  ```

  * `if (whatTime() == doneTime)` 我们检查当前仿真时间是否等于先前记录的处理完成时间，如果是，我们会进入 if 分支；否则，我们会进入 else 分支，也就意味本次 fire 函数调用结束，什么也不做。在这个例子中，由于 Producer 只在 Consumer 准备好时发送数据，所以我们不应该进入到这个 else 分支，我们在这里抛出一个异常。
  * 在 if 分支中，我们模拟 payload 处理完成的过程，输出到控制台，清除 pending 状态。
  * 如果 pendingPayload 是 10，也就是最后一个任务，我们调用 `terminate()` 函数终止仿真，这是 Wolf-Sim 中标准的仿真结束方法。
  * 如果 pendingPayload 不是 10，我们向 readyOPort 发送 ready 信号，通知 Producer 模块可以发送下一个 payload。`readyOPort << true` 是 Wolf-Sim 标准的输出写入方法。

### 定义顶层模块

```cpp
class Top : public wolf_sim::Module {
 private:
  void construct() {
    auto producer = createChildModule<Producer>("MyProducer");
    auto consumer = createChildModule<Consumer>("MyConsumer");

    auto payloadReg = createRegister("payloadReg");
    producer->payloadOPort >>= payloadReg;
    consumer->payloadIPort <<= payloadReg;

    auto readyReg = createRegister("readyReg");
    consumer->readyOPort >>= readyReg;
    producer->readyIPort <<= readyReg;
  }
};
```

* 顶层模块不需要输入和输出端口定义。
* 顶层模块的 construct 函数中，我们首先创建了 Producer 和 Consumer 两个子模块：
  * 我们使用 `createChildModule<T>(std::string name)` 模板函数创建子模块；
  * 模板参数 T 指定子模块的类名；
  * 函数参数 name 指定子模块的实例名称，可以是任意合法的字符串，在一个父模块中，子模块的实例名称必须唯一；
  * 该模板函数返回的是一个指向子模块实例的智能指针，后续我们可以通过这个指针访问子模块的端口。
* 之后，我们使用 `createRegister(std::string name)` 函数创建了两个 Register 实例，分别用于连接 Producer 和 Consumer 的 payload 和 ready 信号：
  * 由于在 Module 定义时已经确定了端口的类型，创建 Register 时不需要再指定类型；
  * 我们使用 `<<=` 和 `>>=` 操作符将 Register 和 Module 的端口连接起来，这两个操作符是 Wolf-Sim 中标准的端口连接方法；
  * `<<=` 操作符表示将 Register 连接到 Module 的输入端口；
  * `>>=` 操作符表示将 Register 连接到 Module 的输出端口。

### 启动仿真

```cpp
int main() {
  wolf_sim::Environment env;
  auto top = std::make_shared<Top>();
  env.addTopModule(top);
  env.run();
}
```

* 在 main 函数中，我们首先创建了一个 Environment 实例 env。
* 然后，我们使用 `std::make_shared<Top>()` 创建了一个顶层模块 Top 的智能指针 top。
* 最后，我们将 top 添加到 env 中，调用 `env.run()` 启动仿真。

## 复制这些代码创建你的模块

```cpp
#include "wolf_sim.h"

class MyModule : public wolf_sim::Module {
 public:
  /* 端口定义部分 */
  // IPort(portName, type);
  // IPortArray(portName, type, arraySize);
  // OPort(portName, type);
  // OPortArray(portName, type, arraySize);
  // FromChildPort(portName, type);
  // FromChildPortArray(portName, type, arraySize);
  // ToChildPort(portName, type);
  // ToChildPortArray(portName, type, arraySize);

 private:
  /* 根据仿真行为需要自定义的状态 */
  // some custom code
  /* 描述子结构的 construct 函数 */
  void construct() {
    // auto childModuleA = createChildModule<ChildModule>("ChildModuleNameA");
    // auto childModuleB = createChildModule<ChildModule>("ChildModuleNameB");
    // auto myRegister = createRegister("MyRegister");
    // childModuleA->outputPort >>= myRegister;
    // childModuleB->inputPort <<= myRegister;
    
    // toChildPort >>= myRegister;
    // childModuleA->fromParentPort <<= myRegister;
    // childModuleB->toParentPort >>= myRegister;
    // fromChildPort <<= myRegister;

    // myInputPort >>= childModuleA->inputPort;
    // childModuleB->outputPort >>= myOutputPort;
  }
  /* 描述模块行为的 fire 函数 */
  void fire() {
    // Time_t currentTime = whatTime();
    // if (inputPort >> data) {
    //   outputPort << data;
    //   outputPort.write(data, delay);
    //   planWakeUp(delay);
    // }
  }
  /* 在模块仿真结束后调用 */
  void finalStop() {
    // 在这里可以进行一些数据收集、上报工作，例如将仿真结果写入文件
    // 注意 finalStop() 仍然是多线程进行的，需要处理好线程安全问题
  }
};
```

## 复制这些代码，创建你的 main 函数

```
int main() {
  wolf_sim::Environment env;
  auto top = std::make_shared<TopModule>();
  env.addTopModule(top);
  env.run();
}
```

## Module 的端口系统

Wolf-Sim 提供了八个宏来定义模块的输入输出端口：

```cpp
IPort(portName, type) // 输入端口
OPort(portName, type) // 输出端口

FromChildPort(portName, type) // 从子模块读取数据的端口
ToChildPort(portName, type) // 向子模块写入数据的端口

IPortArray(portName, type, arraySize) // 输入端口组
OPortArray(portName, type, arraySize) // 输出端口组

FromChildPortArray(portName, type, arraySize) // 从子模块读取数据的端口组
ToChildPortArray(portName, type, arraySize) // 向子模块写入数据的端口组
```

IPort 和 OPort 是模块对外的输入输出接口。

如果模块包含子模块，并且希望在 fire 过程中与子模块交互，则需要定义 FromChildPort 和 ToChildPort，并在 construct 函数中通过寄存器建立连接。

### 端口的读写

在 Module 的 fire 方法中，可对端口进行读写。

端口的读写是 Module 在仿真过程中与其他 Module 交换数据的唯一方式。

#### 基本的读写操作

端口的写操作使用 `<<` 操作符：

```cpp
  portName << data;
```

端口的读操作使用 `>>` 操作符，假设端口的数据类型为 T：

```cpp
  T data;
  if(portName >> data) {
    // 读取成功，可访问 data
  }
```
为什么要用 if 语句包裹读取操作？假如一个 Module 有多个输入端口，或者存在计划唤醒，那么 fire 被调用时，端口上可能没有数据到达。

当没有数据到达时，>> 表达式会**返回 false**，**data 的值不会被修改**。

对于 bool 型端口，可以使用 valid() 判断端口是否有数据到达，且数据是否为 true，这对于握手信号建模场景非常有用：

```cpp
  if(portName.valid()) {
    // 端口输入有效
  }
```

#### 端口延迟写操作

有的时候，我们希望在一定延迟之后写出数据，例如建模一个流水线，可通过 write 方法实现：

```cpp
  portName.write(data, delay);
```

### 端口的连接

端口的连接在 construct 方法中进行，不需要在意连接建立的先后顺序，Wolf-Sim 会自动处理细节保证连接成功。

#### 通过寄存器的端口连接

绝大多数连接需要通过寄存器实现，寄存器是 Wolf-Sim 中的数据交换中介。

寄存器的创建：

```cpp
  auto myRegister = createRegister("myRegister");
```

提供了 `<<=` 和 `>>=` 操作符，实现端口和寄存器的连接，使用这些操作符时，记住以下口诀：

*端口在左，寄存器在右，箭头指向，数据流向*

下面是一些例子。

子模块的输出端口连接到寄存器：
  
```cpp
  childModule->outputPort >>= myRegister;
```

寄存器连接到子模块的输入端口：
  
  ```cpp
    childModule->inputPort <<= myRegister;
  ```

模块的 ToChildPort 通过寄存器连接到子模块的输入端口：
  
  ```cpp
    toChildPort >>= myRegister;
    childModule->inputPort <<= myRegister;
  ```

模块的 FromChildPort 通过寄存器连接到父模块的输出端口：
  
  ```cpp
    childModule->outputPort >>= myRegister;
    fromChildPort <<= myRegister;
  ```

#### 不通过寄存器的端口连接

仅适用于父子模块之间同方向端口的连接。

适用于父模块包装子模块时，希望直接将子模块端口暴露到父模块外部的场景（[例子](example/parent_child.cpp)）。

需要在父模块上定义端口，然后在 construct 方法中使用 `>>=` 操作符连接，记住以下口诀：

*箭头向右，数据向右*

```cpp
  IPort(parentInputPort, int);
  OPort(parentOutputPort, int);

  void construct() {
    parentInputPort >>= childModule->inputPort;
    childModule->outputPort >>= parentOutputPort;
  }
```

#### 如何理解 ToChildPort 和 FromChildPort

当模块的 fire 方法需要和子模块交换数据时，使用 ToChildPort 和 FromChildPort。

ToChildPort 表示向子模块写入数据，FromChildPort 表示从子模块读取数据。

ToChildPort 实际上是 OPort 的别名，FromChildPort 实际上是 IPort 的别名。

### 端口组

由于端口的数据类型可以包含 vector、array 等容器，如果端口能满足你的需求，那么就不要使用端口组。

端口组包含多个端口，每个端口在仿真内核中是独立握手的，所以不必要的端口组会增加仿真内核的负担。

事实上，端口组只是把端口放到了 std::vector 里，所以只需要指定下标，就可以操作端口组中的单个端口了：

```cpp
  IPortArray(inputPortArray, int, 10);
  OPortArray(outputPortArray, int, 10);

  void fire() {
    for (int i = 0; i < 10; i++) {
      if (inputPortArray[i] >> data) {
        // 读取成功
      }
    }
  }
```

## Module 的 `fire()` 方法

Module 的 `fire()` 方法描述模块的仿真行为，由仿真内核调用。

`fire()` 方法在三种情况下被仿真内核调用：

1. 仿真的 0 时刻；
2. 模块的输入端口（IPort, IPortArray, FromChildPort, FromChildPortArray）有数据到达时；
3. 模块被计划唤醒时。

在 `fire()` 方法中，我们可以进行以下操作：

1. 查看当前仿真时间；
2. 操作自定义的状态；
3. 读取输入端口的数据；
4. 向输出端口（OPort, OPortArray, ToChildPort, ToChildPortArray）写入数据；
5. 计划自己的唤醒；
6. 终止仿真。

### 查看当前仿真时间

在 `fire()` 方法中，我们可以通过 `whatTime()` 方法查看当前仿真时间：

```cpp
  void fire() {
    wolf_sim::Time_t now = whatTime();
    std::cout << "Current time is " << now<< std::endl;
  }
```

在单次 fire 调用过程中 `whatTime()` 返回的时间是恒定的。

在连续的 fire 调用过程中，`whatTime()` 返回的时间是严格递增的。

### 向输出端口写数据

通过[操作输出端口](#基本的读写操作)我们可以向输出端口写入数据，**当前时刻 t**写入的数据，会在**下一时刻 t+1 **被接受方读取。

[延迟写入](#端口延迟写操作)端口的数据，在 delay 个时间单位后被写入，这意味着数据接受方在 delay+1 个时间单位时才能读取到数据。

### Module 的计划唤醒

在 `fire()` 方法中，我们可以通过 `planWakeUp(delay)` 方法计划自己的唤醒：

```cpp
  void fire() {
    // do something
    planWakeUp(10); // 10 个时间单位后唤醒
  }
```

这意味着，在10个周期后，即使输入端口没有有效数据，fire 也会被调用一次。

### 终止仿真

可以在任意模块的 `fire()` 方法中调用 `terminate()` 方法终止仿真：

```cpp
  void fire() {
    if (condition) {
      terminate();
    }
  }
```

`terminate()` 方法会关闭整个仿真系统，使得 `env.run()` 返回。

Wolf-Sim 也支持自动仿真终止，当系统中不再有计划的任务和数据交换时，仿真会自动终止。

## Module 的 `construct()` 方法

Module 的 `construct()` 方法建立子结构并描述子结构之间的连接关系。

在 `construct()` 方法中，我们可以进行以下操作：

1. 创建子模块
2. 创建寄存器
3. [建立连接](#端口的连接)

### 创建子模块

在 `construct()` 方法中，我们可以通过 `createChildModule<T>(std::string name)` 方法创建子模块：

```cpp
  void construct() {
    auto childModule = createChildModule<ChildModule>("ChildModuleName");
  }
```

### 创建寄存器

在 `construct()` 方法中，我们可以通过 `createRegister(std::string name)` 方法创建寄存器：

```cpp
  void construct() {
    auto myRegister = createRegister("MyRegister");
  }
```

## Module 的 `finalStop()` 方法

无论是仿真被手动终止还是自动终止，模块仿真结束时都会调用 `finalStop()` 方法。

在 `finalStop()` 方法中，我们可以进行一些数据收集、上报工作，例如将仿真结果写入文件，或者在全局变量中记录仿真结果。

需要注意的是，`finalStop()` 方法仍然是多线程进行的，需要处理好线程安全问题。



