#include <iostream>

#include "wolf_sim.h"

const int TOTAL_PAYLOAD = 20;
const int PROCESS_DELAY = 3;

class Producer : public wolf_sim::Module {
 public:
  /* 端口定义 */
  Output(payloadValid, bool);
  Output(payload, int);
  Input(payloadReady, bool);

 private:
  /* 内部状态定义 */
  Reg(nextPayload, int);

  /* 初始化函数 */
  void init() {
    nextPayload = 0;
    payloadValid = false;
  }

  /* 状态更新函数 */
  void updateStateOutput() {
    if (payloadValid && payloadReady) {
      logger() << "Producing payload " << payload << std::endl;
      nextPayload = nextPayload + 1;
    }
    payloadValid = true;
    payload = nextPayload;
  }
};

class Consumer : public wolf_sim::Module {
 public:
  /* 端口定义 */
  Input(payloadValid, bool);
  Input(payload, int);
  Output(payloadReady, bool);

 private:
  /* 内部状态定义 */
  Reg(busyCount, int);

  /* 初始化函数 */
  void init() {
    busyCount = 0;
    payloadReady = true;
  }

  /* 状态和输出更新函数 */
  void updateStateOutput() {
    if (payloadValid && payloadReady) {
      busyCount = PROCESS_DELAY - 1;
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
 public:
  /* 端口定义 */
  Input(anUselessInput, int);
  Output(anUselessOutput, int);

 private:
  /* 子模块定义 */
  ChildModule(producer, Producer);
  ChildModuleWithLabel(consumer, Consumer, "consumer");  // 为子模块显式设置标签

  /* 子模块输入更新函数 */
  void updateChildInput() {
    consumer->payloadValid = producer->payloadValid;
    consumer->payload = producer->payload;
    producer->payloadReady = consumer->payloadReady;
  }

  /* 状态和输出更新函数 */
  void updateStateOutput() { anUselessOutput = anUselessInput + 47; }
};

int main() {
  /* 建议总是使用智能指针保存仿真模型 */
  std::shared_ptr<Top> top = std::make_shared<Top>();
  // tick once;
  top->anUselessInput = 19780823;
  top->tick();
  std::cout << "anUselessOutput: " << top->anUselessOutput << " @ "
            << top->whatTime() << std::endl;
  // tick 10 times;
  top->tick(10);
  // tick to termination;
  while (!top->terminated()) {
    // 可在此处设置输入
    top->tick();
    // 可在此处读取输出
  }

  std::cout << ">> reset the model and start again <<" << std::endl;
  // create (reset) a new model
  top = std::make_shared<Top>();
  // and then another way to tick to termination
  top->tickToTermination();
  return 0;
}
