//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_ENVIRONMENT_H
#define WOLF_SIM_ENVIRONMENT_H
#include <vector>
#include <memory>
#include "AlwaysBlock.h"
#include "async_simple/coro/Lazy.h"

namespace wolf_sim {
    class Environment {
    private:
        int threadNum;
        std::vector<std::shared_ptr<AlwaysBlock>> alwaysBlockVec;
        bool running;
        async_simple::coro::Lazy<void> coroStart();
    public:
        Environment(int _threadNum);
        void addAlwaysBlock(std::shared_ptr<AlwaysBlock> alwaysBlockPtr);
        void run();
    };
}



#endif //WOLF_SIM_ENVIRONMENT_H
