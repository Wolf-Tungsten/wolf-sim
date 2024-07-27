//
// Created by gaoruihao on 6/23/23.
//

#include "Environment.h"

namespace wolf_sim
{
    Environment::Environment(int _threadNum) : threadNum(_threadNum) {};
   
    void Environment::addTopModule(std::shared_ptr<Module> topModulePtr) {
        topModulePtr -> construct(); /*从这里开始递归构造整个模块树*/
        addModule(topModulePtr);
    };

    void Environment::addModule(std::shared_ptr<Module> modulePtr)
    {
        modulePtrVec.push_back(modulePtr);
        for (const auto &internalModulePair : modulePtr->childModuleMap)
        {
            addModule(internalModulePair.second);
        }
    }

    void wolf_sim::Environment::run()
    {
        // 启动所有 Module 的 simulationLoop，为每个 Module 分配一个线程
        std::vector<std::thread> threads;
        for (const auto& modulePtr : modulePtrVec) {
            threads.emplace_back([&modulePtr]() {
                modulePtr->simulationLoop();
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

    }
};

