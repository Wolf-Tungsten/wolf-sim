#include "wolf_sim/wolf_sim.h"
#include <iostream>
#include <memory>
#include <functional>

class Producer : public wolf_sim::Module {
    public:
    IPort(consumerReady, bool);
    OPort(payloadOutput, int);
    private:
    int payload;
    void fire(){
        std::cout << "Producer fire at " << fireTime << std::endl;
        if(consumerReady.valid()){
            std::cout << "Producer get consumer ready signal at " << fireTime << " and send payload " << payload << std::endl;
            payloadOutput.write(payload++);
        }
    };
};

class Consumer : public wolf_sim::Module {
    public:
    IPort(payloadInput, int);
    OPort(consumerReady, bool);
    private:
    bool pending = false;
    wolf_sim::Time_t doneTime = 0;
    int pendingPayload;
    void fire(){
        std::cout << "Consumer fire at " << fireTime << std::endl; 
        if(pending){
            if(doneTime == fireTime){
                std::cout << "Consumer process payload " << pendingPayload << " done at " << fireTime << std::endl;
                pending = false;
                consumerReady.write(true);
            }
        } else {
            if(payloadInput.valid()){
                pending = true;
                pendingPayload = payloadInput.read();
                doneTime = fireTime + 10;
                planWakeUp(10);
            } else {
                std::cout << "Consumer get no payload at " << fireTime << std::endl;
                consumerReady.write(true);
            }
        }
        

    }
};

class Top : public wolf_sim::Module {
    public:
    void construct(){
        auto producer = createChildModule<Producer>("Producer");
        auto consumer = createChildModule<Consumer>("Consumer");
        auto readyReg = createRegister("readyReg");
        auto payloadReg = createRegister("payloadReg");
        producer -> payloadOutput.connect(payloadReg);
        consumer -> payloadInput.connect(payloadReg);
        producer -> consumerReady.connect(readyReg);
        consumer -> consumerReady.connect(readyReg);
    }
};

int main() {
    wolf_sim::Environment env(2);
    env.createTopBlock<Top>();
    env.run();
}