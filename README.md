# Wolf Sim

A lightweight coroutine-based discrete event simulation framework.

基于协程的轻量级离散事件仿真框架

## 仿真模型：Register—Synchornized Simulation Model（RSSM）


仿真模型希望通过软件代码模仿数字电路运行的过程，获取运行的细节信息，以评估架构设计的有效性。

数字电路的运行过程可以抽象成一系列状态之间的转换，如果我们将外部输入也视为状态的一部分，则下一状态总是可以从上一状态唯一确定。

在数字电路中，状态是保存在存储元件中的，因此，RSSM模型使用：

- Register 建模所有状态相关的寄存器
- AlwaysBlock 表达寄存器之间的交互关系

## 寄存器 Register

RSSM 中的寄存器具有：

- 一个且仅有一个输入端口
- 至少一个或多个输出端口

不同于数字电路中的寄存器只能保存一个元素，RSSM中的寄存器可以具有深度，即可以保存多个元素，多个元素以FIFO规则进出寄存器。



