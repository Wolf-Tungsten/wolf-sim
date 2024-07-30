#include <iostream>

#include "wolf_sim/wolf_sim.h"

class Producer : public wolf_sim::Module {
 public:
  /* 端口定义部分 */
  IPort(busyIPort, bool);
  OPort(payloadOPort, int);

 private:
  /* 根据需要定义的状态 */
  int payload = -1;
  /* 描述模块行为的 fire 函数 */
  void fire() {
    std::cout << "Producer fired at " << whatTime() << std::endl;
    bool isConsumerBusy;
    if (busyIPort.valid()) {
      // 重新发送当前 payload
      payloadOPort << payload;
      std::cout << "Producer resent payload " << payload << " at "
                << whatTime() << std::endl;
      planWakeUp(1);
    } else {
      // 发送下一个 payload
      payload++;
      payloadOPort << payload;
      std::cout << "Producer sent payload " << payload << " at " << whatTime()
                << std::endl;
      if(payload != 0 && whatTime() != (payload-1) * 11 + 1){
        throw std::runtime_error("Producer sent payload at wrong time");
      }
      planWakeUp(1);
    }
  };
};

class Consumer : public wolf_sim::Module {
 public:
  /* 端口定义部分 */
  IPort(payloadIPort, int);
  OPort(busyOPort, bool);

 private:
  /* 根据仿真行为需要自定义的状态 */
  bool busy = false;
  wolf_sim::Time_t doneTime = 0;
  int pendingPayload;
  /* 描述模块行为的 fire 函数 */
  void fire() {
    std::cout << "Consumer fired at " << whatTime() << std::endl;
    if (busy) {
      if (doneTime == whatTime()) {
        busy = false;
        std::cout << "Consumer processed done payload " << pendingPayload
                  << " at " << whatTime() << std::endl;
        if(pendingPayload == 10){
          terminateSimulation();
        }
      } else {
        std::cout << "Consumer sent busy at " << whatTime() << std::endl;
        busyOPort << true;
        planWakeUp(1);
      }
    } else if (payloadIPort >> pendingPayload) {
      std::cout << "Consumer received payload " << pendingPayload << " and sent busy at "
                << whatTime() << std::endl;
      busy = true;
      busyOPort << true;
      doneTime = whatTime() + 10;
      std::cout << "Consumer will done at " << doneTime << std::endl;
      planWakeUp(1);
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
