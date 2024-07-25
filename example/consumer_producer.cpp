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
        writeRegister(O::o, (int)(fireTime+114));
        if(fireTime <= 100){
            planWakeUp(5);
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
            std::cout << "Consumer Time= " << fireTime << ", Payload=" << std::any_cast<int>(inputRegPayload[I::i]) << std::endl;
        }
    }
};

class ProducerConsumerSystem : public wolf_sim::AlwaysBlock {
    public:
    void construct(){
        auto producer = createAlwaysBlock<Producer>();
        auto consumer = createAlwaysBlock<Consumer>();
        auto reg = createRegister();
        producer -> assignOutput(Producer::O::o, reg);
        consumer -> assignInput(Consumer::I::i, reg);
    }
};

int main() {
    wolf_sim::Environment env(2);
    env.createTopBlock<ProducerConsumerSystem>();
    env.run();
}