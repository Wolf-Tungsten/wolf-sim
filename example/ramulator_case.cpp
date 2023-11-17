#include "wolf_sim/Environment.h"
#include "wolf_sim/AlwaysBlock.h"
#include "wolf_sim/Register.h"
#include <iostream>
#include <memory>
#include "Request.h"
#include "RamulatorConnector.h"
class Producer : public wolf_sim::AlwaysBlock {
private:
    wolf_sim::RegRef<std::pair<long, ramulator::Request::Type>, 10> reg;
    int delay;
public:
    Producer(wolf_sim::Register <std::pair<long, ramulator::Request::Type>, 10> &reg_, int delay_) : delay(delay_) {
        reg.asOutput(this, &reg_);
    }

    wolf_sim::ReturnNothing always() override {
        long payload =0;
        std::pair<long, ramulator::Request::Type> instr;
        instr.first = payload;
        instr.second = ramulator::Request::Type::WRITE;
        while (1) {
            //std::cout << "Producer try to put " << payload << " at " << blockTimestamp << std::endl;
            blockTimestamp += delay;
            co_await reg.put(instr);
            //std::cout << "Producer put " << payload << " at " << blockTimestamp << std::endl;
            payload++;
        }
        std::cout << "Producer" <<" finished at:" << blockTimestamp << std::endl;
    }
};

// class Consumer : public wolf_sim::RamulatorConnector {
// private:
//     int idx;
//     int delay;
//     wolf_sim::RegRef<std::pair<long, Request::Type>, 10> reg;

// public:
//     Consumer(wolf_sim::Register<int, 10>& reg_, int idx_, int delay_) : idx(idx_), delay(delay_) {
//         std::cout << "Consumer " << idx << " created" << std::endl;
//         blockIdentifier = "Consumer " + std::to_string(idx);
//         reg.asInput(this, &reg_);
//     }

//     wolf_sim::ReturnNothing always() override {
//         int reqCount = 0;
//         while (1) {
//             blockTimestamp += delay;
//             //std::cout << "Consumer " << idx << " request " << reqCount++ << " get at " << blockTimestamp << std::endl;
//             auto payload = co_await reg.get();
//             //std::cout << "Consumer " << idx << " get " << payload << " at " << blockTimestamp << std::endl;
//             if(payload == 10000){
//                 break;
//             }
//         }
//         std::cout << blockIdentifier <<" finished at:" << blockTimestamp << std::endl;
//     }
// };


int main() {
    wolf_sim::Environment env(1);
    // wolf_sim::Register<std::pair<long, ramulator::Request::Type>, 10> reg;
    // std::shared_ptr<Producer> producer = std::make_shared<Producer>(reg, 1);
    std::string infilename = "/home/shishunchen/hdd0/my_projects/wolf-sim/ramulator/configs/DDR4-config.cfg";
    int timescale = 1000;
    std::shared_ptr<wolf_sim::RamulatorConnector> ramulatorConnector = std::make_shared<wolf_sim::RamulatorConnector>(infilename,timescale);
    // env.addAlwaysBlock(producer);
    env.addAlwaysBlock(ramulatorConnector);
    env.run();

}