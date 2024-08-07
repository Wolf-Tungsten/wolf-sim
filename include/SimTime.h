#ifndef WOLF_SIM_TIME_H
#define WOLF_SIM_TIME_H
#include <cstdint>
#include <limits>


namespace wolf_sim {
    using SimTime_t = int64_t;
    const SimTime_t MAX_TIME = std::numeric_limits<SimTime_t>::max();
}
#endif