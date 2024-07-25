
//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_ALWAYSBLOCK_H
#define WOLF_SIM_ALWAYSBLOCK_H
#include <memory>
#include <map>
#include <string>
#include <queue>
#include <vector>
#include <stdexcept>
#include <tuple>
#include "wolf_sim.h"

namespace wolf_sim {

    class AlwaysBlock {
    public:
        void assignInput(int id, std::shared_ptr<Register> regPtr);
        void assignOutput(int id, std::shared_ptr<Register> regPtr);
        virtual void construct(){};
        friend class Environment;
    
    protected:
        Time_t fireTime;
        std::map<int, std::any> inputRegPayload;
        std::vector<std::any> wakeUpPayload;
        virtual void fire(){};

        template<typename AlwaysBlockDerivedType>
        std::shared_ptr<AlwaysBlockDerivedType> createAlwaysBlock(std::string name = ""){
            static_assert(std::is_base_of<AlwaysBlock, AlwaysBlockDerivedType>::value, "internal block must be derived from AlwaysBlock class");
            if(name == ""){
                name = std::string("anonymous_block_") + std::to_string(internalAlwaysBlockMap.size());
            }
            if(internalAlwaysBlockMap.contains(name)){
                throw std::runtime_error("create AlwaysBlock name conflict!");
            }
            auto p = std::make_shared<AlwaysBlockDerivedType>();
            internalAlwaysBlockMap[name] = p;
            p -> construct();
            return p;
        };
        std::shared_ptr<Register> createRegister(std::string name = "");
        void planWakeUp(Time_t delay, std::any wakeUpPayload = std::any());
        void writeRegister(int id, std::any writePayload, Time_t delay=1, bool overwrite=false);
    
    private:
        Time_t internalFireTime = 0;
        ReturnNothing simulationLoop();
        std::map<int, std::shared_ptr<Register>> inputRegisterMap;
        std::map<int, std::shared_ptr<Register>> outputRegisterMap;
        std::map<std::string, std::shared_ptr<Register>> internalRegisterMap;
        std::map<std::string, std::shared_ptr<AlwaysBlock>> internalAlwaysBlockMap;
        std::map<int, std::pair<std::any, bool>> pendingRegisterWrite;
        struct EarliestWakeUpComparator {
            bool operator()(const std::pair<Time_t, std::any>& p1, const std::pair<Time_t, std::any>& p2) {
                return p1.first > p2.first;
            }
        };
        std::priority_queue<std::pair<Time_t, std::any>, std::vector<std::pair<Time_t, std::any>>, EarliestWakeUpComparator> wakeUpSchedule;
        struct EarliestRegisterWriteComparator {
            bool operator()(const std::tuple<Time_t, int, std::any, bool>& p1, const std::tuple<Time_t, int, std::any, bool>& p2) {
                return std::get<0>(p1) > std::get<0>(p2);
            }
        };
        std::priority_queue<std::tuple<Time_t, int, std::any, bool>, std::vector<std::tuple<Time_t, int, std::any, bool>>, EarliestRegisterWriteComparator> registerWriteSchedule;
    };
    
}


#endif //WOLF_SIM_ALWAYSBLOCK_H