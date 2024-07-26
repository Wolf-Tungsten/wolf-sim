#include "wolf_sim/wolf_sim.h"
#include <iostream>
#include <memory>
#include <functional>

class Producer : public wolf_sim::Module {
    OPort(o, int);
    void fire(){
        oWrite((int)fireTime);
        std::cout << "Producer Time= " << fireTime << ", Payload=" << fireTime << std::endl;
        if(fireTime <= 100){
            planWakeUp(5);
        } else {
            oTerminate(0);
        }
    };
};

class Consumer : public wolf_sim::Module {
    IPort(i, int);
    void fire(){
        std::cout << "consumer" << std::endl;
        if(iHasValue()){
            std::cout << "Consumer Time= " << fireTime << ", Payload=" << iRead() << std::endl;
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
        //producer -> assignOutput(Producer::O::o, reg);
        producer -> oConnect(reg);
        consumer -> iConnect(reg);
    }
};

int main() {
    wolf_sim::Environment env(1);
    env.createTopBlock<ProducerConsumerSystem>();
    env.run();
}