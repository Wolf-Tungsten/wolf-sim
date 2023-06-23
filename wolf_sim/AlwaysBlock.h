//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_ALWAYSBLOCK_H
#define WOLF_SIM_ALWAYSBLOCK_H
#include "async_simple/coro/Lazy.h"

namespace wolf_sim {
    class AlwaysBlock {
    protected:
        double timestamp;
    public:
        virtual async_simple::coro::Lazy<void> always() = 0;
    };
}


#endif //WOLF_SIM_ALWAYSBLOCK_H
