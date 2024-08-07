# Wolf-Sim：一个C++并行离散事件框架

<!-- 目录 -->
- [简介](#简介)
  
## 开发笔记

### configRoutine

1. 顶层模块的配置在模型外部设置
2. 父模块配置子模块
3. 自动递归调用 config 方法
4. config 路线自顶向下绑定 ModuleContext

### initRoutine

1. 初始化模块的仿真时间
2. 初始化 modulePhase
3. 调用用户重载的 init()
4. 调用子模块的 initRoutine

### 全系统的初始化

1. 判断是否初始化的依据：顶层模块的 mcPtr 是否为空
2. 第一次调用 tick 时初始化顶层模块的 mcPtr，包括设置其中的变量初始值
3. 先调用 configRoutine，再调用 initRoutine
4. reset 就是将 mcPtr 置空