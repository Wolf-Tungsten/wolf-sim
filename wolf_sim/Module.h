
//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_MODULE_H
#define WOLF_SIM_MODULE_H
#include <memory>
#include <map>
#include <string>
#include <queue>
#include <vector>
#include <stdexcept>
#include <tuple>
#include <functional>
#include "wolf_sim.h"

#define IPortConnect(name, type) \
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
    }; 

#define IPortOperation(name, type) \
    std::function<bool()> name##HasValue; \
    std::function<type()> name##Read; 

#define IPortArrayConnect(name, type, arraySize) \
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
    };

#define IPortArrayOperation(name, type, arraySize) \
    int name##Size = arraySize; \
    std::function<bool(int)> name##HasValue; \
    std::function<type(int)> name##Read;

#define OPortConnect(name, type) \
    void name##Connect(std::shared_ptr<wolf_sim::Register> reg){ \
        int id = outputRegisterMap.size(); \
        outputRegisterMap[id] = reg; \
        reg -> connectAsOutput(shared_from_this()); \
        name##Write = [this, id](type value){ \
            writeRegister(id, value); \
        }; \
        name##WriteDelay = [this, id](type value, wolf_sim::Time_t delay){ \
            writeRegister(id, value, delay); \
        }; \
        name##Terminate = [this, id](wolf_sim::Time_t delay){ \
            writeRegister(id, std::any(), delay, true); \
        }; \
    };

#define OPortOperation(name, type) \
    std::function<void(type)> name##Write; \
    std::function<void(type, wolf_sim::Time_t)> name##WriteDelay; \
    std::function<void(wolf_sim::Time_t)> name##Terminate;

#define OPortArrayConnect(name, type, arraySize) \
    void name##Connect(std::vector<std::shared_ptr<wolf_sim::Register>> regVec){ \
        int id = outputRegisterMap.size(); \
        for(int i=0; i<arraySize; i++){ \
            outputRegisterMap[id+i] = regVec[i]; \
            regVec[i] -> connectAsOutput(shared_from_this()); \
        } \
        name##Write = [this, id](int idx, type value){ \
            writeRegister(id+idx, value); \
        }; \
        name##WriteDelay = [this, id](int idx, type value, int delay){ \
            writeRegister(id+idx, value, delay); \
        }; \
        name##Terminate = [this, id](int idx, wolf_sim::Time_t delay){ \
            writeRegister(id+idx, std::any(), delay, true); \
        }; \
    };

#define OPortArrayOperation(name, type, arraySize) \
    int name##Size = arraySize; \
    std::function<void(int, type)> name##Write; \
    std::function<void(int, type, wolf_sim::Time_t)> name##WriteDelay; \
    std::function<void(int, wolf_sim::Time_t)> name##Terminate;

#define IPort(name, type) \
    public: \
        IPortConnect(name, type) \
    private: \
        IPortOperation(name, type)

#define IPortArray(name, type, arraySize) \
    public: \
        IPortArrayConnect(name, type, arraySize)\
    private: \
        IPortArrayOperation(name, type, arraySize)

#define  OPort(name, type) \
    public: \
        OPortConnect(name, type)\
    private: \
        OPortOperation(name, type)

#define OPortArray(name, type, arraySize) \
    public: \
        OPortArrayConnect(name, type, arraySize)\
    private: \
        OPortArrayOperation(name, type, arraySize)

#define FromChildPort(name, type) \
    private: \
        IPortConnect(name, type)\
        IPortOperation(name, type)

#define FromChildPortArray(name, type, arraySize) \
    private: \
        IPortArrayConnect(name, type, arraySize)\
        IPortArrayOperation(name, type, arraySize)

#define  ToChildPort(name, type) \
    private: \
        OPortConnect(name, type)\
        OPortOperation(name, type)

#define ToChildPortArray(name, type, arraySize) \
    private: \
        OPortArrayConnect(name, type, arraySize)\
        OPortArrayOperation(name, type, arraySize)

namespace wolf_sim {

    class Module : public std::enable_shared_from_this<Module> {
    public:
        void assignInput(int id, std::shared_ptr<Register> regPtr);
        void assignOutput(int id, std::shared_ptr<Register> regPtr);
        virtual void construct(){};
        friend class Environment;
        void setNameAndParent(std::string _name, std::weak_ptr<Module> _parentPtr);
    
    protected:
        std::string name;
        std::weak_ptr<Module> parentPtr;
        std::map<int, std::shared_ptr<Register>> inputRegisterMap;
        std::map<int, std::shared_ptr<Register>> outputRegisterMap;

        Time_t fireTime;
        std::map<int, std::any> inputRegPayload;
        std::vector<std::any> wakeUpPayload;
        virtual void fire(){};

        template<typename ModuleDerivedType>
        std::shared_ptr<ModuleDerivedType> createChildModule(std::string name = ""){
            static_assert(std::is_base_of<Module, ModuleDerivedType>::value, "child module must be derived from Module class");
            if(name == ""){
                name = std::string("anonymous_block_") + std::to_string(childModuleMap.size());
            }
            if(childModuleMap.contains(name)){
                throw std::runtime_error("create Module name conflict!");
            }
            auto p = std::make_shared<ModuleDerivedType>();
            childModuleMap[name] = p;
            p -> construct();
            p -> setNameAndParent(name, shared_from_this());
            return p;
        };
        std::shared_ptr<Register> createRegister(std::string name = "");
        void planWakeUp(Time_t delay, std::any wakeUpPayload = std::any());
        void writeRegister(int id, std::any writePayload, Time_t delay=1, bool terminate=false);
    
    private:
        Time_t internalFireTime = 0;
        ReturnNothing simulationLoop();
        #if OPT_OPTIMISTIC_READ
        std::map<int, Time_t> inputRegActiveTimeOptimistic;
        std::map<int, bool> inputRegLockedOptimistic;
        #endif
        std::vector<int> terminatedInputRegNote;
        std::map<std::string, std::shared_ptr<Register>> childRegisterMap;
        std::map<std::string, std::shared_ptr<Module>> childModuleMap;
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
        std::map<int, Time_t> pendingRegisterTerminate;
    };
    
}


#endif //WOLF_SIM_MODULE_H