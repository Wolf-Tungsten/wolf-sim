#ifndef WOLF_SIM
#define WOLF_SIM

#include <cstdint>
#include <limits>


namespace wolf_sim {
    using Time_t = int64_t;
    const Time_t MAX_TIME = std::numeric_limits<Time_t>::max();
}

#include "ModuleContext.h"
#include "Module.h"
#include "ChildModuleRef.h"
#include "StateRef.h"



#endif