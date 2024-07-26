#include "wolf_sim/wolf_sim.h"
#include <iostream>
#include <memory>
#include <functional>

class Producer : public wolf_sim::AlwaysBlock {
    OPortArray(o, int, 10);
    void fire(){
        oWrite(0, 1);
        if(fireTime <= 100){
            planWakeUp(5);
        } else {
            oWrite(0, 114514);
        }
    };
};

class Consumer : public wolf_sim::AlwaysBlock {
    IPortArray(i, int, 10);
    void fire(){
        if(iHasValue(0)){
            std::cout << "Consumer Time= " << fireTime << ", Payload=" << iRead(0) << std::endl;
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
        auto producer = createChildBlock<Producer>("producer");
        auto consumer = createChildBlock<Consumer>("consumer");
        auto regVec = std::vector<std::shared_ptr<wolf_sim::Register>>();
        for(int i=0; i<10; i++){
            regVec.push_back(createRegister());
        }
        //producer -> assignOutput(Producer::O::o, reg);
        producer -> oConnect(regVec);
        consumer -> iConnect(regVec);
    }
};

int main() {
    wolf_sim::Environment env(1);
    env.createTopBlock<ProducerConsumerSystem>();
    env.run();
}