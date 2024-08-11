#ifndef WOLF_SIM_CHILD_TICK_SCHEDULER_H
#define WOLF_SIM_CHILD_TICK_SCHEDULER_H

#include <thread>
#include <map>
#include <memory>
#include <vector>

#include "SimTime.h"
#include "Module.h"

namespace wolf_sim {
class Module;
class ChildTickScheduler {
    public:
        void setup(Module* module);
        void forceSerial();
        void forceParallel();
        bool determined();
        void scheduledTick(SimTime_t currentTime);
    private:
        const static int MAX_THREAD_COUNT = 4;
        const static int MAX_BENCHMARK_COUNT = 500;
        int benchmarkCount = 0;
        double serialOverhead = 0;
        double parallelOverhead = 0;
        Module* module = nullptr;
        enum Mode {pending, benchmarkSerial, benchmarkParallel, serial, parallel} mode = pending;
        void serialTick(SimTime_t currentTime);
        void parallelTick(SimTime_t currentTime);
        bool allChildrenDetermined();
};
}


#endif //WOLF_SIM_TICK_SCHEDULER_H