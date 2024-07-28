# Wolf-Sim：一个C++并行离散事件框架

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

我们将尝试用 Wolf-Sim 构造一个支持反压的生产者消费者模型。

该模型包含三个模块：Producer，Consumer 和 Top。

Producer 向 Consumer 发送 payload （一个int整数）。

Consumer 每接收一个 payload 就要花 10 个单位时间进行处理（打印到控制台），处理期间 Consumer 不接受输入；当 Consumer 可以接受输入时，会向 Producer 发送 ready 信号；收到 10 个 payload 之后仿真终止。

Producer 在收到 Consumer 的 ready 信号后，会向 Consumer 发送一个数字作为 payload。

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

* 然后，我们根据仿真的需要定义了一个状态 payload，表示当前要发送的 payload。

* 最后，我们重载了 fire 函数，描述模块的行为：
     * fire 函数在三种情况下被仿真内核调用，一是仿真的 0 时刻，二是在模块的输入端口有数据到达时，三是在模块被计划唤醒时，**在 Producer 中我们只关心第二种情况**；
     * 在 fire 中，我们检查 readyIPort 是否有有效输入，如果有，我们打印当前时间和 payload 的值，并将 payload 发送到 payloadOPort，然后 payload 加一。








