# Wolf-Sim：一个C++并行离散事件框架

## 三个组件

### Module

### Register

### Environment

## 如何构建一个仿真模型

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



