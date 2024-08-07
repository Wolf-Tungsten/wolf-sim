#include <iostream>

#include "wolf_sim.h"

const int TOTAL_PAYLOAD = 2000;
const int PROCESS_DELAY = 1;

class Producer : public wolf_sim::Module {
 public:
  /* 端口定义部分 */

  Output(payloadValid, bool);
  Output(payload, int);
  Output(payloadReady, bool);

 private:
  Reg(nextPayload, int);
  Reg(vectorState, std::vector<int>);

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
    payload = nextPayload.r();
    vectorState.w().push_back(nextPayload.r());
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
    busyCount.w() = 0;
    payloadReady.w() = true;
  }

  void updateStateOutput() {
    if (*payloadValid && *payloadReady) {
      //busyCount.w() = PROCESS_DELAY;
      busyCount = PROCESS_DELAY;
      if (*payload == TOTAL_PAYLOAD) {
        terminate();
        return;
      }
      logger() << "Consuming payload " << *payload << std::endl;
    }
    if (*busyCount > 0) {
      busyCount.w() = busyCount.r() - 1;
      payloadReady.w() = false;
    } else {
      payloadReady.w() = true;
    }
  }
};

class Top : public wolf_sim::Module {
 private:
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
