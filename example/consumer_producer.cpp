#include "wolf_sim/wolf_sim.h"
#include <iostream>
#include <memory>
#include <functional>

class Producer : public wolf_sim::Module {
    public:
    OPort(o, int);
    private:
    void fire(){
        o.write(fireTime);
        std::cout << "Producer Time= " << fireTime << ", Payload=" << fireTime << std::endl;
        if(fireTime <= 100){
            planWakeUp(5);
        } else {
            o.terminate(0);
        }
    };
};

class Consumer : public wolf_sim::Module {
    public:
    IPortArray(i, int, 10);
    private:
    void fire(){
        std::cout << "consumer" << std::endl;
        if(i[0].valid()){
            std::cout << "Consumer Time= " << fireTime << ", Payload=" << i[0].read() << std::endl;
            std::cout << "Consumer 计划在 Time=" << fireTime+2 << "唤醒" << std::endl;
            planWakeUp(2, (int)fireTime);
        } else if (wakeUpPayload[0].has_value()){
            std::cout << "Consumer Time= " << fireTime << ", WakeUpPayload=" << std::any_cast<int>(wakeUpPayload[0]) << std::endl;
        }
    }
};

class ProducerConsumerSystem : public wolf_sim::Module {
    public:
    void construct(){
        auto producer = createChildModule<Producer>("producer");
        auto consumer = createChildModule<Consumer>("consumer");
        auto reg = createRegister("reg");
        producer -> o.connect(reg);
        consumer -> i[0].connect(reg);
    }
};

int main() {
    wolf_sim::Environment env(1);
    env.createTopBlock<ProducerConsumerSystem>();
    env.run();
}