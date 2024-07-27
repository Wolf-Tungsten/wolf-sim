#include "wolf_sim/wolf_sim.h"
#include <iostream>
#include <memory>
#include <functional>

class Producer : public wolf_sim::Module {
    public:
    IPort(readyIPort, bool);
    OPort(payloadOPort, int);
    private:
    int payload;
    void fire(){
        bool isConsumerReady;
        if(readyIPort >> isConsumerReady){
            std::cout << "Producer get consumer ready signal at " << whatTime() << " and send payload " << payload << std::endl;
            payloadOPort << payload;
            if(payload >= 10){
                payloadOPort.terminate(1);
            }
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
    void fire(){ 
        if(pending){
            if(whatTime() == doneTime){
                std::cout << "Consumer processes payload " << pendingPayload << " done at " << whatTime() << std::endl;
                pending = false;
                if(pendingPayload == 10){
                    readyOPort.terminate(1);
                    std::cout << "Consumer terminates at " << whatTime() << std::endl;
                } else {
                    readyOPort << true;
                    std::cout << "Consumer sends consumer ready signal at " << whatTime() << std::endl;
                }
            }
        } else {
            if(payloadIPort >> pendingPayload){
                pending = true;
                doneTime = whatTime() + 10;
                std::cout << "Consumer processes payload " << pendingPayload << " start at " << whatTime() << std::endl;
                planWakeUp(10);
            } else {
                std::cout << "Consumer sends consumer ready signal at " << whatTime() << std::endl;
                readyOPort << true;
            }
        }
    }
};

class Top : public wolf_sim::Module {
    public:
    void construct(){

        auto producer = createChildModule<Producer>("Producer");
        auto consumer = createChildModule<Consumer>("Consumer");

        auto payloadReg = createRegister("payloadReg");
        producer -> payloadOPort >>= payloadReg;
        consumer -> payloadIPort <<= payloadReg;

        auto readyReg = createRegister("readyReg");
        consumer -> readyOPort >>= readyReg;
        producer -> readyIPort <<= readyReg;
        
    }
};

int main() {
    wolf_sim::Environment env(2);
    auto top = std::make_shared<Top>();
    env.addTopModule(top);
    env.run();
}