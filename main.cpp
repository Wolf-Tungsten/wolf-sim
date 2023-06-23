//
// Created by gaoruihao on 6/23/23.
//


#include "wolf_sim/Environment.h"
#include "wolf_sim/AlwaysBlock.h"
#include "wolf_sim/Register.h"
#include <iostream>

class Producer : public wolf_sim::AlwaysBlock {
private:
    wolf_sim::Register<int>& reg;
    int delay;
public:
    Producer(wolf_sim::Register<int>& reg_, int delay_): reg(reg_), delay(delay_) {
        timestamp = 0.0;
    }
    async_simple::coro::Lazy<void> always() override {
        int payload = 0;
        while(1){
            timestamp += delay;
            co_await reg.put(payload, timestamp);
            std::cout << "Producer put " << payload << " at " << timestamp << std::endl;
            if(payload == 10){
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
    wolf_sim::Register<int>& reg;
public:
    Consumer(wolf_sim::Register<int>& reg_, int idx_, int delay_): reg(reg_), idx(idx_), delay(delay_) {
        timestamp = 0.0;
    }
    async_simple::coro::Lazy<void> always() override {
        while(1){
            timestamp += delay;
            auto payload = co_await reg.get(idx, timestamp);
            std::cout << "Consumer " << idx << " get " << payload << " at " << timestamp << std::endl;
        }
    }
};

int main() {
    wolf_sim::Environment env(1);
    wolf_sim::Register<int> reg(2);
    Producer p(reg, 1);
    Consumer c0(reg, 0, 3);
    Consumer c1(reg, 1, 7);
    env.addAlwaysBlock(p);
    env.addAlwaysBlock(c0);
    env.addAlwaysBlock(c1);
    env.run();
}