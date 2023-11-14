#include "wolf_sim/Environment.h"
#include "wolf_sim/AlwaysBlock.h"
#include "wolf_sim/Register.h"
#include <iostream>
#include <memory>

class Producer : public wolf_sim::AlwaysBlock {
private:
    wolf_sim::RegRef<int, 10> reg;
    int delay;
public:
    Producer(wolf_sim::Register<int, 10> &reg_, int delay_) : delay(delay_) {
        reg.asOutput(this, &reg_);
    }

    wolf_sim::ReturnNothing always() override {
        int payload = 0;
        while (1) {
            std::cout << "Producer try to put " << payload << " at " << blockTimestamp << std::endl;
            blockTimestamp += delay;
            co_await reg.put(payload);
            std::cout << "Producer put " << payload << " at " << blockTimestamp << std::endl;
            if (payload == 20) {
                break;
            }
            payload++;
        }
    }
};

class Consumer : public wolf_sim::AlwaysBlock {
private:
    int idx;
    int delay;
    wolf_sim::RegRef<int, 10> reg;
public:
    Consumer(wolf_sim::Register<int, 10>& reg_, int idx_, int delay_) : idx(idx_), delay(delay_) {
        std::cout << "Consumer " << idx << " created" << std::endl;
        reg.asInput(this, &reg_);
    }

    wolf_sim::ReturnNothing always() override {
        while (1) {
            std::cout << "Consumer " << idx << " try to get at " << blockTimestamp << std::endl;
            blockTimestamp += delay;
            auto payload = co_await reg.get();
            std::cout << "Consumer " << idx << " get " << payload << " at " << blockTimestamp << std::endl;
        }
    }
};


int main() {
    wolf_sim::Environment env(1);
    wolf_sim::Register<int, 10> reg;
    std::shared_ptr<Producer> producer = std::make_shared<Producer>(reg, 1);
    std::vector<std::shared_ptr<Consumer>> consumersVec;
    for(int i = 0; i < 10; i++){
        consumersVec.push_back(std::make_shared<Consumer>(reg, i , 5));
        env.addAlwaysBlock(consumersVec[i]);
    }
    env.addAlwaysBlock(producer);
    env.run();
}