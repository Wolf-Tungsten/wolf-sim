#ifndef WOLF_SIM
#define WOLF_SIM

#include <cstdint>
#include <limits>

#define OPT_OPTIMISTIC_READ 0
#define ENABLE_MODULE_LOG 0 

namespace wolf_sim {
    using Time_t = int64_t;
    const Time_t MAX_TIME = std::numeric_limits<Time_t>::max();
    class Register;
    class Environment;
    class Module;
}

#include "Register.h"
#include "Module.h"
#include "Environment.h"



#endif