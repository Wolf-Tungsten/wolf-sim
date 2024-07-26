
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
#include <functional>
#include "wolf_sim.h"

#define IPort(name, type) \
    public: \
    void name##Connect(std::shared_ptr<wolf_sim::Register> reg){ \
        int id = inputRegisterMap.size(); \
        inputRegisterMap[id] = reg; \
        reg -> connectAsInput(shared_from_this()); \
        name##HasValue = [this, id](){ \
            return inputRegPayload.contains(id); \
        }; \
        name##Read = [this, id](){ \
            return std::any_cast<type>(inputRegPayload[id]); \
        }; \
    } \
    private: \
    std::function<bool()> name##HasValue; \
    std::function<type()> name##Read; 

#define IPortArray(name, type, arraySize) \
    public: \
    void name##Connect(std::vector<std::shared_ptr<wolf_sim::Register>> regVec){ \
        int id = inputRegisterMap.size(); \
        for(int i=0; i<arraySize; i++){ \
            inputRegisterMap[id+i] = regVec[i]; \
            regVec[i] -> connectAsInput(shared_from_this()); \
        } \
        name##HasValue = [this, id](int idx){ \
            return inputRegPayload.contains(id+idx); \
        }; \
        name##Read = [this, id](int idx){ \
            return std::any_cast<type>(inputRegPayload[id+idx]); \
        }; \
    } \
    private: \
    std::function<bool(int)> name##HasValue; \
    std::function<type(int)> name##Read;

#define  OPort(name, type) \
    public: \
    void name##Connect(std::shared_ptr<wolf_sim::Register> reg){ \
        int id = outputRegisterMap.size(); \
        outputRegisterMap[id] = reg; \
        reg -> connectAsOutput(shared_from_this()); \
        name##Write = [this, id](type value){ \
            writeRegister(id, value); \
        }; \
        name##WriteEnhanced = [this, id](type value, int delay, bool overwrite){ \
            writeRegister(id, value, delay, overwrite); \
        }; \
    } \
    private: \
    std::function<void(type)> name##Write; \
    std::function<void(type, int, bool)> name##WriteEnhanced; 

#define OPortArray(name, type, arraySize) \
    public: \
    void name##Connect(std::vector<std::shared_ptr<wolf_sim::Register>> regVec){ \
        int id = outputRegisterMap.size(); \
        for(int i=0; i<arraySize; i++){ \
            outputRegisterMap[id+i] = regVec[i]; \
            regVec[i] -> connectAsOutput(shared_from_this()); \
        } \
        name##Write = [this, id](int idx, type value){ \
            writeRegister(id+idx, value); \
        }; \
        name##WriteEnhanced = [this, id](int idx, type value, int delay, bool overwrite){ \
            writeRegister(id+idx, value, delay, overwrite); \
        }; \
    } \
    private: \
    std::function<void(int, type)> name##Write; \
    std::function<void(int, type, int, bool)> name##WriteEnhanced;

namespace wolf_sim {

    class AlwaysBlock : public std::enable_shared_from_this<AlwaysBlock> {
    public:
        void assignInput(int id, std::shared_ptr<Register> regPtr);
        void assignOutput(int id, std::shared_ptr<Register> regPtr);
        virtual void construct(){};
        friend class Environment;
        void setNameAndParent(std::string _name, std::weak_ptr<AlwaysBlock> _parentPtr);
    
    protected:
        std::string name;
        std::weak_ptr<AlwaysBlock> parentPtr;
        std::map<int, std::shared_ptr<Register>> inputRegisterMap;
        std::map<int, std::shared_ptr<Register>> outputRegisterMap;

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
            std::cout << "createAlwaysBlock: " << name << std::endl;
            p -> construct();
            p -> setNameAndParent(name, shared_from_this());
            return p;
        };
        std::shared_ptr<Register> createRegister(std::string name = "");
        void planWakeUp(Time_t delay, std::any wakeUpPayload = std::any());
        void writeRegister(int id, std::any writePayload, Time_t delay=1, bool overwrite=false);
    
    private:
        Time_t internalFireTime = 0;
        ReturnNothing simulationLoop();
        #if OPT_OPTIMISTIC_READ
        std::map<int, Time_t> inputRegActiveTimeOptimistic;
        std::map<int, bool> inputRegLockedOptimistic;
        #endif
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