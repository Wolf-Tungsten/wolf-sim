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
        void addModule(std::shared_ptr<Module> modulePtr);
    public:
        Environment(int _threadNum);
        void addTopModule (std::shared_ptr<Module> topModulePtr);
        void run();
    };
}



#endif //WOLF_SIM_ENVIRONMENT_H
