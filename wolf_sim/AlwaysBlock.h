
//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_ALWAYSBLOCK_H
#define WOLF_SIM_ALWAYSBLOCK_H
#include "async_simple/coro/Lazy.h"

namespace wolf_sim {

    typedef async_simple::coro::Lazy<void> ReturnNothing;

    class AlwaysBlock {
    public:
        virtual ReturnNothing always() = 0;
        long blockTimestamp;
        std::string blockIdentifier;
        AlwaysBlock(){ 
            blockTimestamp = 0;
        };
        AlwaysBlock(const AlwaysBlock&) = delete;
        AlwaysBlock& operator=(const AlwaysBlock&) = delete;
        AlwaysBlock(AlwaysBlock&&) = delete;
        AlwaysBlock& operator=(AlwaysBlock&&) = delete;
    };
    
}


#endif //WOLF_SIM_ALWAYSBLOCK_H