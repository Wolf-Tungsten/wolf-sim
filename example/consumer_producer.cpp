#include <iostream>

#include "wolf_sim/wolf_sim.h"

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
        doneTime = whatTime() + 10; /* 记录处理完成时间 */
        std::cout << "Consumer got payload " << pendingPayload << " at "
                  << whatTime() << std::endl;
        planWakeUp(10); /* 计划 10 个时间单位后唤醒 */
      } else {
        /* 没有新的 payload，发出 ready 信号 */
        std::cout << "Consumer sent ready signal at " << whatTime()
                  << std::endl;
        readyOPort << true;
      }
    } else {
      /* 有正在处理的任务 */
      if (whatTime() == doneTime) {
        /* 到达处理完成时间 */
        std::cout << "Consumer completed payload " << pendingPayload
                  << " processing at " << whatTime() << std::endl;
        pending = false; /* 模拟处理完成 */
        if (pendingPayload == 10) {
          /* 如果处理的是最后一个任务，调用 terminateSimulation() 仿真终止 */
          std::cout << "Consumer terminated at " << whatTime() << std::endl;
          terminateSimulation();
        } else {
          /* 否则继续发出 ready 信号通知 Producer */
          readyOPort << true;
          std::cout << "Consumer sent ready signal at " << whatTime()
                    << std::endl;
        }
      } else {
        /* 还未到处理完成时间，即使有新的 payload 也不处理 */
        /* 在这个例子中不会进入这个分支 */
        throw std::runtime_error("Consumer should not have new payload");
      }
    }
  }
};

class Top : public wolf_sim::Module {
 private:
  void construct() {
    auto producer = createChildModule<Producer>("Producer");
    auto consumer = createChildModule<Consumer>("Consumer");

    auto payloadReg = createRegister("payloadReg");
    producer->payloadOPort >>= payloadReg;
    consumer->payloadIPort <<= payloadReg;

    auto readyReg = createRegister("readyReg");
    consumer->readyOPort >>= readyReg;
    producer->readyIPort <<= readyReg;
  }
};

int main() {
  wolf_sim::Environment env;
  auto top = std::make_shared<Top>();
  env.addTopModule(top);
  env.run();
}

