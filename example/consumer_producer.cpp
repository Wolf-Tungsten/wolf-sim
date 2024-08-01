#include <iostream>

#include "wolf_sim.h"

const int TOTAL_PAYLOAD = 1000000;
const int PROCESS_DELAY = 0;

class Producer : public wolf_sim::Module {
 public:
  /* 端口定义部分 */
  IPort(busyIPort, bool);
  OPort(payloadOPort, int);

 private:
  /* 根据需要定义的状态 */
  int payload = 0;
  /* 描述模块行为的 fire 函数 */
  void fire() {
    if (whatTime() == 0) {
      payloadOPort << payload;
    } else {
      if (busyIPort.valid()) {
        // t-1 时刻 consumer 处于 busy 状态，所以 t 时刻 Producer 还是发送 payload
        payloadOPort << payload;
      } else {
        // t-1 时刻 consumer 出于 idle 状态，payload 发送成功
        std::cout << "Producer sent " << payload << " at " << whatTime() - 1
                  << std::endl;
        // t 时刻发送下一个 payload
        ++payload;
        if(payload < TOTAL_PAYLOAD){
          payloadOPort << payload;
        } else {
          terminateModuleSimulation();
        }
      }
    }
    planWakeUp(1);
  };
};

class Consumer : public wolf_sim::Module {
 public:
  /* 端口定义部分 */
  IPort(payloadIPort, int);
  OPort(busyOPort, bool);

 private:
  bool busy = false;
  int pendingPayload;
  wolf_sim::Time_t doneTime;
  void fire() {
    if (busy) {
      if (whatTime() == doneTime) {
        // 模拟处理完成
        std::cout << "Consumer processed " << pendingPayload << " at "
                  << whatTime() << std::endl;
        if (pendingPayload == TOTAL_PAYLOAD - 1) {
          std::cout << "Consumer finished at " << whatTime() << std::endl;
          terminateSimulation();
        }
        busy = false;
      } else {
        // 模拟处理中
        busyOPort << true;
        planWakeUp(1);
      }
    } else {
      if (payloadIPort >> pendingPayload) {
        if (PROCESS_DELAY == 0) {
          // 模拟处理完成
          std::cout << "Consumer processed " << pendingPayload << " at "
                    << whatTime() << std::endl;
          if (pendingPayload == TOTAL_PAYLOAD - 1) {
            std::cout << "Consumer finished at " << whatTime() << std::endl;
            terminateSimulation();
          }
        } else {
          // 模拟处理中
          busy = true;
          doneTime = whatTime() + PROCESS_DELAY;
          busyOPort << true;
          planWakeUp(1);
        }
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

    auto busyReg = createRegister("busyReg");
    consumer->busyOPort >>= busyReg;
    producer->busyIPort <<= busyReg;
  }
};

int main() {
  wolf_sim::Environment env;
  auto top = std::make_shared<Top>();
  env.addTopModule(top);
  env.run();
}
