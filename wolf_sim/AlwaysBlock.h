//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_ALWAYSBLOCK_H
#define WOLF_SIM_ALWAYSBLOCK_H

namespace wolf_sim {

    class AlwaysBlock {
    public:
        virtual void always() = 0;
        long blockTimestamp;
        AlwaysBlock(){ 
            blockTimestamp = 0;
        };
        private:
            AlwaysBlock(const AlwaysBlock&);
            AlwaysBlock& operator=(const AlwaysBlock&);
            AlwaysBlock(AlwaysBlock&&) = delete;
            AlwaysBlock& operator=(AlwaysBlock&&) = delete;
    };
    
}


#endif //WOLF_SIM_ALWAYSBLOCK_H
