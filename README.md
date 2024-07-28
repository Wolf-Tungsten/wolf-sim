# Wolf-Sim：一个C++并行离散事件框架

- [Wolf-Sim：一个C++并行离散事件框架](#wolf-sim一个c并行离散事件框架)
  - [认识三个基础组件](#认识三个基础组件)
    - [Module](#module)
    - [Register](#register)
    - [Environment](#environment)
  - [一个手把手教程](#一个手把手教程)
    - [导入头文件](#导入头文件)
    - [从定义生产者模块开始](#从定义生产者模块开始)
    - [定义消费者模块](#定义消费者模块)
    - [定义顶层模块](#定义顶层模块)
    - [启动仿真](#启动仿真)

## 认识三个基础组件

### Module

Wolf-Sim 构造的仿真系统是由多个 Module 子类的实例组成的：

1. Module 在 `fire()` 方法中描述自己的行为；
2. Module 可递归地包含 **子 Module** ，在 `construct()` 方法中创建 **子 Module** 并描述 **子 Module** 之间的连接关系；
3. 一个仿真系统中的 Module 呈现树状结构，根节点称为 **顶层 Module**。

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
  * `doneTime = whatTime() + 10; ` 在 fire 函数中，`whatTime()` 函数返回当前仿真时间。我们记录下当前时间加上 10 个时间单位，表示处理完成时间。
  * `planWakeUp(10)` 表示我们计划在当前 payload 处理完成，也就是 10 个时间单位后再次唤醒消费者模块。
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




