#ifndef WOLF_SIM_ENVIRONMENT_H
#define WOLF_SIM_ENVIRONMENT_H

#include <vector>
#include <thread>
#include <memory>
#include "AlwaysBlock.h"

#include <iostream>

namespace wolf_sim {
    class Environment {
    private:
        std::vector<std::shared_ptr<AlwaysBlock>> alwaysBlockPtrVec;
        std::vector<std::thread> threadVec;
    public:
        void addAlwaysBlock(std::shared_ptr<AlwaysBlock> alwaysBlockPtr){
            alwaysBlockPtrVec.push_back(alwaysBlockPtr);
        };
        void run(){
            for (auto alwaysBlockPtr : alwaysBlockPtrVec) {
                threadVec.emplace_back([alwaysBlockPtr](){
                    alwaysBlockPtr->always();
                });
            }
            for (auto& thread : threadVec) {
                thread.join();
            }
        };
    };
}

#endif //WOLF_SIM_ENVIRONMENT_H
