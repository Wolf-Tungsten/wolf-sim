#ifndef WOLF_SIM_CHILD_TICK_SCHEDULER_H
#define WOLF_SIM_CHILD_TICK_SCHEDULER_H

#include <thread>
#include <map>
#include <memory>
#include <vector>

#include "Module.h"

namespace wolf_sim {
class ChildTickScheduler {
    public:
        void setup(std::weak_ptr<Module> module);
        void reset();
        void forceSerial();
        void forceParallel();
        bool determined();
        void scheduledTick(Time_t currentTime);
    private:
        const static int MAX_THREAD_COUNT = 4;
        const static int MAX_BENCHMARK_COUNT = 50;
        int benchmarkCount = 0;
        double serialOverhead = 0;
        double parallelOverhead = 0;
        std::weak_ptr<Module> module;
        enum Mode {pending, benchmarkSerial, benchmarkParallel, serial, parallel} mode;
        void serialTick(Time_t currentTime);
        void parallelTick(Time_t currentTime);
        bool allChildrenDetermined();
};
}


#endif //WOLF_SIM_TICK_SCHEDULER_H