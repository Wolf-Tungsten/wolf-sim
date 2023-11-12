//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_ALWAYSBLOCK_H
#define WOLF_SIM_ALWAYSBLOCK_H
#include "async_simple/coro/Lazy.h"
#include <atomic>

namespace wolf_sim {

    typedef async_simple::coro::Lazy<void> ReturnNothing;

    class AlwaysBlock {
    public:
        virtual ReturnNothing always() = 0;
        long blockTimestamp;
        AlwaysBlock(){ 
            blockTimestamp = 0;
        };
    };
    
}


#endif //WOLF_SIM_ALWAYSBLOCK_H
