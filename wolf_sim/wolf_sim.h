#ifndef WOLF_SIM
#define WOLF_SIM

#include <cstdint>
#include <limits>
#include <cassert>
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Mutex.h"
#include "async_simple/coro/ConditionVariable.h"

namespace wolf_sim {
    using Time_t = uint64_t;
    const Time_t MAX_TIME = std::numeric_limits<Time_t>::max();
    using ReturnNothing = async_simple::coro::Lazy<>;
    class Register;
    class Environment;
    class AlwaysBlock;
}

#include "Register.h"
#include "AlwaysBlock.h"
#include "Environment.h"



#endif