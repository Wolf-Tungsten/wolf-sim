#include <iostream>

#include "wolf_sim.h"

const int TOTAL_PAYLOAD = 2000;
const int PROCESS_DELAY = 1;

class Producer : public wolf_sim::Module {
 public:
  /* 端口定义部分 */
  Output(payloadValid, bool);
  Output(payload, int);
  Input(payloadReady, bool);

 private:
  /* 状态定义部分 */
  Reg(nextPayload, int);

  void init() {
    nextPayload = 0;
    payloadValid = false;
  }

  void updateStateOutput() {
    if (*payloadValid && *payloadReady) {
      logger() << "Producing payload " << *payload << std::endl;
      nextPayload = nextPayload + 1;
    }
    payloadValid = true;
    payload = nextPayload;
  }
};

class Consumer : public wolf_sim::Module {
 public:
  /* 端口定义部分 */
  Input(payloadValid, bool);
  Input(payload, int);
  Output(payloadReady, bool);

 private:
  Reg(busyCount, int);

  void init() {
    busyCount = 0;
    payloadReady = true;
  }

  void updateStateOutput() {
    if (payloadValid && payloadReady) {
      busyCount = PROCESS_DELAY;
      if (payload == TOTAL_PAYLOAD) {
        terminate();
        return;
      }
      logger() << "Consuming payload " << payload << std::endl;
    }
    if (busyCount > 0) {
      busyCount = busyCount - 1;
      payloadReady = false;
    } else {
      payloadReady = true;
    }
  }
};

class Top : public wolf_sim::Module {
 private:
  Reg(dumy, int);
  ChildModule(producer, Producer);
  ChildModuleWithLabel(consumer, Consumer, "consumer");
  void updateChildInput() {
    consumer->payloadValid = producer->payloadValid;
    consumer->payload = producer->payload;
    producer->payloadReady = consumer->payloadReady;
  }
};

int main() {
  Top top;
  top.tickToTermination();
  top.reset();
  while(!top.terminated()){
    top.tick();
  }
  top.reset();
  top.tick(100);
  return 0;
}
