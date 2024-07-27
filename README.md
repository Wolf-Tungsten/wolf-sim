# Wolf-Sim：一个C++并行离散事件框架

## 三个基础组件

### Module

Wolf-Sim 构造的仿真系统是由多个 Module 子类的实例组成的：

1. Module 描述自己的行为
2. Module 可递归地包含 **子 Module** ，并描述 **子 Module**之间的连接关系
3. 一个仿真系统中的 Module 呈现层次树状结构，根节点称为 **顶层 Module**。

Wolf-Sim 中的 Module 可类比于 Verilog 中的模块，或 SystemC 中的模块。

### Register

Register 建立 Module 之间的连接，完成数据交换、时间同步。

如果一个 Register R 连接了 Module A 的输出和 Module B 的输入，那么，Module A 写入到 R 的数据一定会被 Module B 读取到，但从仿真行为的角度看，可以选择忽略。

### Environment

包装了仿真系统确立、仿真线程启动等功能细节，使得用户可以专注于模型的构建。

将 **顶层Module** 添加到 Enivronment，然后调用 run() 即可开始仿真。

## 一个手把手教程

我们将尝试用 Wolf-Sim 构造一个支持反压的生产者消费者模型。

该模型包含三个模块：Producer，Consumer 和 Top。

Consumer 每接收一个数字就要花 10 个单位时间进行处理（打印到控制台），处理期间 Consumer 不能接受输入，当 Consumer 可以接受输入时，会向 Producer 发送 ready 信号。

Producer 在收到 Consumer 的 ready 信号后，会向 Consumer 发送一个数字作为 payload，一共发送 10 个 payload 后仿真终止。

Top 是顶层模块，描述了 Producer 和 Consumer 之间的连接关系。

### 1. 创建自定义的 Module

```cpp
class MyModule : public Module {
    /** Part 1 端口声明 **/
    IPort(port1, int); /**声明一个名称为 port1，数据类型为 int 的输入端口**/
    IPort(port2, bool); /**声明一个名称为 port2，数据类型为 bool 的输入端口**/
    OPort(port3, std::string); /**声明一个名称为 port3，数据类型为 std::string 的输出端口**/
    OPort(port4, someCustomType); /**端口的类型可以是任何支持复制的类型**/
    IPortArray(port5, int, 114); /**声明一个名称为 port5，数据类型为 int 的输入端口数组，长度为 114**/
    OPortArray(port6, double, 514); /**声明一个名称为 port6，数据类型为 double 的输出端口数组，长度为 514**/
    /** 什么时候使用端口数组？
     * 如果 I/OPort(portName, std::vector<xxx>)，就能满足你的要求，那么不！要！使！用！端口数组
     * 直接使用 I/OPort.
     * 上述情况搞不定时才用 IPortArray / OPortArray
    */

   /** Part 2 建立子结构 **/
   /** 模块可以拥有子结构，并且可以和子结构交互 
    * 和子结构交互的端口，需要在这里声明 
    * 向子结构写数据声明 To， 从子结构读数据声明 From**/
   ToChildPort(name, type);
   ToChildPortArray(name, type, size);
   FromChildPort(name, type);
   FromChildPortArray(name, type, size);
   void construct() {
        /** 创建子模块 **/
        auto childBlockP = createChildModule<ChildBlockType>();
        /** 创建一个寄存器并连接到子模块的端口 **/
        auto reg = createRegister();
        childBlockP -> xxxConnect(reg);
        /** 把寄存器连接到当前模块的内部端口 **/
        xxxConnect(reg);
   }

}
```



