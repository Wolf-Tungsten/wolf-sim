//
// Created by gaoruihao on 6/23/23.
//


#include "wolf_sim/Environment.h"
#include "wolf_sim/AlwaysBlock.h"
#include "wolf_sim/Register.h"
#include <iostream>

class Producer : public wolf_sim::AlwaysBlock {
private:
    wolf_sim::Register<int> &reg;
    int delay;
public:
    Producer(wolf_sim::Register<int> &reg_, int delay_) : reg(reg_), delay(delay_) {
        timestamp = 0.0;
    }

    async_simple::coro::Lazy<void> always() override {
        int payload = 0;
        while (1) {
            timestamp += delay;
            co_await reg.put(payload, timestamp);
            std::cout << "Producer put " << payload << " at " << timestamp << std::endl;
            if (payload == 9) {
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
    wolf_sim::Register<int> &reg;
public:
    Consumer(wolf_sim::Register<int> &reg_, int idx_, int delay_) : reg(reg_), idx(idx_), delay(delay_) {
        timestamp = 0.0;
    }

    async_simple::coro::Lazy<void> always() override {
        while (1) {
            timestamp += delay;
            auto payload = co_await reg.get(idx, timestamp);
            std::cout << "Consumer " << idx << " get " << payload << " at " << timestamp << std::endl;
        }
    }
};

class FIFO : public wolf_sim::AlwaysBlock {
private:
    wolf_sim::Register<int> &regIn;
    wolf_sim::Register<int> &regOut;
    std::vector<int> buffer;
    double timestamp;
public:
    FIFO(wolf_sim::Register<int>& regIn_, wolf_sim::Register<int>& regOut_) : regIn(regIn_), regOut(regOut_) {
        timestamp = 0.0;
    }
    async_simple::coro::Lazy<void> always() override {
        while (1) {
            timestamp += 1.0;
            int payload;
            if(co_await regIn.nonBlockGet(0, timestamp, payload)) {
                std::cout << "FIFO get " << payload << " at " << timestamp << std::endl;
                buffer.push_back(payload);
            }
            if(buffer.size() > 0 && co_await regOut.nonBlockPut(buffer[0], timestamp)) {
                std::cout << "FIFO put " << buffer[0] << " at " << timestamp << std::endl;
                buffer.erase(buffer.begin());
            }
        }
    }
};

int main() {
    wolf_sim::Environment env(0);
    wolf_sim::Register<int> regIn(1), regOut(2);
    Producer p(regIn, 1);
    FIFO fifo(regIn, regOut);
    Consumer c0(regOut, 0, 3);
    Consumer c1(regOut, 1, 7);
    env.addAlwaysBlock(p);
    env.addAlwaysBlock(fifo);
    env.addAlwaysBlock(c0);
    env.addAlwaysBlock(c1);
    env.run();
}