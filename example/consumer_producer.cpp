#include "wolf_sim/wolf_sim.h"
#include <iostream>
#include <memory>

class Producer : public wolf_sim::AlwaysBlock {
    public:
    enum O {
        o
    };
    protected:
    void fire(){
        writeRegister(O::o, (fireTime+114));
        if(fireTime <= 100){
            planWakeUp(5);
        } else {
            writeRegister(O::o, wolf_sim::MAX_TIME);
        }
    };
};

class Consumer : public wolf_sim::AlwaysBlock {
    public:
    enum I {
        i
    };
    protected:
    void fire(){
        if(inputRegPayload.contains(I::i)){
            std::cout << "Consumer Time= " << fireTime << ", Payload=" << std::any_cast<wolf_sim::Time_t>(inputRegPayload[I::i]) << std::endl;
            std::cout << "Consumer 计划在 Time=" << fireTime+2 << "唤醒" << std::endl;
            planWakeUp(2, (int)fireTime);
        } else if (wakeUpPayload[0].has_value()){
            std::cout << "Consumer Time= " << fireTime << ", WakeUpPayload=" << std::any_cast<int>(wakeUpPayload[0]) << std::endl;
        }
    }
};

class ProducerConsumerSystem : public wolf_sim::AlwaysBlock {
    public:
    void construct(){
        auto producer = createAlwaysBlock<Producer>("producer");
        auto consumer = createAlwaysBlock<Consumer>("consumer");
        auto reg = createRegister();
        producer -> assignOutput(Producer::O::o, reg);
        consumer -> assignInput(Consumer::I::i, reg);
    }
};

int main() {
    wolf_sim::Environment env(1);
    env.createTopBlock<ProducerConsumerSystem>();
    env.run();
}