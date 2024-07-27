#include <functional>
#include <iostream>
#include <memory>

#include "wolf_sim/wolf_sim.h"

class Producer : public wolf_sim::Module {
 public:
  IPort(readyIPort, bool);
  OPort(payloadOPort, int);

 private:
  int payload;
  void fire() {
    bool isConsumerReady;
    if (readyIPort >> isConsumerReady) {
      std::cout << "Producer got ready signal at " << whatTime()
                << " and sent payload " << payload << std::endl;
      payloadOPort << payload;
      payload++;
    }
  };
};

class Consumer : public wolf_sim::Module {
 public:
  IPort(payloadIPort, int);
  OPort(readyOPort, bool);

 private:
  bool pending = false;
  wolf_sim::Time_t doneTime = 0;
  int pendingPayload;
  void fire() {
    if (pending) {
      if (whatTime() == doneTime) {
        std::cout << "Consumer completed payload " << pendingPayload
                  << " processing at " << whatTime() << std::endl;
        pending = false;
        if (pendingPayload == 10) {
          std::cout << "Consumer terminated at " << whatTime() << std::endl;
          terminate();
        } else {
          readyOPort << true;
          std::cout << "Consumer sent ready signal at " << whatTime()
                    << std::endl;
        }
      }
    } else {
      if (payloadIPort >> pendingPayload) {
        pending = true;
        doneTime = whatTime() + 10;
        std::cout << "Consumer got payload " << pendingPayload
                  << " at " << whatTime() << std::endl;
        planWakeUp(10);
      } else {
        std::cout << "Consumer sent ready signal at " << whatTime()
                  << std::endl;
        readyOPort << true;
      }
    }
  }
};

class Top : public wolf_sim::Module {
 public:
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
  wolf_sim::Environment env(2);
  auto top = std::make_shared<Top>();
  env.addTopModule(top);
  env.run();
}