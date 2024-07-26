//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_ENVIRONMENT_H
#define WOLF_SIM_ENVIRONMENT_H
#include <vector>
#include <memory>
#include "wolf_sim.h"

namespace wolf_sim {

    class Environment {
    private:
        int threadNum;
        std::vector<std::shared_ptr<Module>> modulePtrVec;
        bool running;
        async_simple::coro::Lazy<void> coroStart();
        void addBlock(std::shared_ptr<Module> blockPtr);
    public:
        Environment(int _threadNum);
        template<typename TopBlockType> void createTopBlock(){   
            auto topBlockPtr = std::make_shared<TopBlockType>();
            topBlockPtr -> construct(); /*从这里开始递归构造整个模块树*/
            addBlock(topBlockPtr);
        };
        void run();
    };
}



#endif //WOLF_SIM_ENVIRONMENT_H
