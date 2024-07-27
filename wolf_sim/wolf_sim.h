#ifndef WOLF_SIM
#define WOLF_SIM

#include <cstdint>
#include <limits>
#include <cassert>
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Mutex.h"
#include "async_simple/coro/ConditionVariable.h"

#define OPT_OPTIMISTIC_READ 0

namespace wolf_sim {
    using Time_t = int64_t;
    const Time_t MAX_TIME = std::numeric_limits<Time_t>::max();
    using ReturnNothing = async_simple::coro::Lazy<>;
    class Register;
    class Environment;
    class Module;
}

#include "Register.h"
#include "Module.h"
#include "Environment.h"



#endif