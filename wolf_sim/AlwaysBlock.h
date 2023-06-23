//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_ALWAYSBLOCK_H
#define WOLF_SIM_ALWAYSBLOCK_H
#include "async_simple/coro/Lazy.h"

namespace {
    class AlwaysBlock {
    private:
        double timestamp;
    public:
        virtual async_simple::coro::Lazy<void> always() = 0;
    };
}


#endif //WOLF_SIM_ALWAYSBLOCK_H
