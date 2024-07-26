#include "Module.h"

namespace wolf_sim {

    void Module::setNameAndParent(std::string _name, std::weak_ptr<Module> _parentPtr){
        name = _name;
        parentPtr = _parentPtr;
    }
    
    int Module::assignInput(std::shared_ptr<Register> reg) {
        int id = inputRegisterMap.size();
        inputRegisterMap[id] = reg;
        reg -> connectAsInput(shared_from_this());
        return id;
    }

    int Module::assignOutput(std::shared_ptr<Register> reg){
        int id = outputRegisterMap.size();
        outputRegisterMap[id] = reg;
        reg -> connectAsOutput(shared_from_this());
        return id;
    }
 
    std::shared_ptr<Register> Module::createRegister(std::string name){
        if(name == ""){
            name = std::string("anonymous_reg_") + std::to_string(childRegisterMap.size());
        }
        if(childRegisterMap.contains(name)){
            throw std::runtime_error("create Register name conflict!");
        }
        auto p = std::make_shared<Register>();
        childRegisterMap[name] = p;
        return p;
    }

    void Module::planWakeUp(Time_t delay, std::any wakeUpPayload) {
        if(delay <= 0){
            throw std::runtime_error("delay should be positive");
        }
        wakeUpSchedule.push(std::make_pair(internalFireTime+delay, wakeUpPayload));
    }

    void Module::writeRegister(int id, std::any writePayload, Time_t delay, bool terminate){
        if(delay < 0) {
           throw std::runtime_error("delay should not be negetive"); 
        } else if (delay == 0){
           pendingRegisterWrite[id] = std::make_pair(writePayload, terminate);
        } else {
           registerWriteSchedule.push(std::make_tuple(internalFireTime+delay, id, writePayload, terminate));
        }
    }

    ReturnNothing Module::simulationLoop(){
        if(inputRegisterMap.empty()){
            /*对于没有输入寄存器的模块，给一个在0时刻启动的机会*/
            wakeUpSchedule.push(std::make_pair(0, std::any()));
        }
        while(!inputRegisterMap.empty() || 
        !wakeUpSchedule.empty() || 
        !registerWriteSchedule.empty()){
            /* 计算最小唤醒时间 */
            Time_t minTime = MAX_TIME;
            #if OPT_OPTIMISTIC_READ
            inputRegLockedOptimistic.clear();
            #endif
            for(const auto& inputRegPair : inputRegisterMap){
                int regId = inputRegPair.first;
                auto regPtr = inputRegPair.second;
                #if OPT_OPTIMISTIC_READ
                if(inputRegActiveTimeOptimistic.contains(regId) && inputRegActiveTimeOptimistic[regId] > minTime){
                    continue;
                }
                #endif
                co_await regPtr->acquireRead();
                if(regPtr->hasTerminated()){
                    terminatedInputRegNote.push_back(regId); // 标记删除
                    regPtr -> releaseRead();
                    continue;
                }
                Time_t regActiveTime = regPtr -> getActiveTime();
                #if OPT_OPTIMISTIC_READ
                inputRegLockedOptimistic[regId] = true;
                inputRegActiveTimeOptimistic[regId] = regActiveTime;
                #endif
                if(regActiveTime < minTime){
                    minTime = regActiveTime;
                }
            }
            // 根据 terminatedInputRegNote 删除已经终止的寄存器
            for(const auto& id: terminatedInputRegNote){
                inputRegisterMap.erase(id);
            }
            terminatedInputRegNote.clear();
            if(!wakeUpSchedule.empty()){
                Time_t wakeUpTime = wakeUpSchedule.top().first;
                if(wakeUpTime < minTime){
                    minTime = wakeUpTime;
                }
            }
            if(!registerWriteSchedule.empty()){
                Time_t writeTime = std::get<0>(registerWriteSchedule.top());
                if(writeTime < minTime){
                    minTime = writeTime;
                }
            }
            /* 构造传给 fire 的 payload 列表 */
            inputRegPayload.clear();
            for(const auto& inputRegPair: inputRegisterMap){
                #if OPT_OPTIMISTIC_READ
                int regId = inputRegPair.first;
                if(!inputRegLockedOptimistic.contains(regId)){
                    continue;
                }
                #endif
                auto regPtr = inputRegPair.second;
                Time_t regActiveTime = regPtr -> getActiveTime();
                std::any payload = regPtr -> read();
                if(regActiveTime == minTime){
                    if(payload.has_value()){
                        inputRegPayload[inputRegPair.first] = payload;
                    }
                    regPtr -> pop();
                }
                regPtr -> releaseRead();
            }
            wakeUpPayload.clear();
            while(!wakeUpSchedule.empty() && wakeUpSchedule.top().first == minTime){
                wakeUpPayload.push_back(wakeUpSchedule.top().second);
                wakeUpSchedule.pop();
            }
            /* 将寄存器写请求构造出来 */
            pendingRegisterWrite.clear();
            while(!registerWriteSchedule.empty() && std::get<0>(registerWriteSchedule.top()) == minTime){
                const auto& regWriteTuple = registerWriteSchedule.top();
                int id = std::get<1>(regWriteTuple);
                std::any payload = std::get<2>(regWriteTuple);
                bool terminate = std::get<3>(regWriteTuple);
                pendingRegisterWrite[id] = std::make_pair(payload, terminate);
                registerWriteSchedule.pop();
            }
            /* 记录（可能的）fire 时间 */
            internalFireTime = minTime;
            fireTime = minTime;
            /* 如果有 payload 就 fire */
            if(!inputRegPayload.empty() || !wakeUpPayload.empty()){
                try {
                    fire();
                } catch (const std::exception& e){
                    std::cerr << "Exception caught in Module fire: " << e.what() << std::endl;
                    exit(1);
                }
            }
            /* 将 fire 产生的结果写出 */
            for(const auto& outputRegPair: outputRegisterMap){
                int id = outputRegPair.first;
                if(pendingRegisterWrite.contains(id)){
                    auto& writePair = pendingRegisterWrite[id];
                    std::any payload = writePair.first;
                    bool terminate = writePair.second;
                    if(!terminate){
                        co_await outputRegPair.second -> write(internalFireTime, payload);
                    } else {
                        co_await outputRegPair.second -> terminate(internalFireTime);
                    }
                } else {
                    /* 向所有没有写入的寄存器打入一个空的 time packet，
                    这里并不检查这些寄存器的 activeTime，交给 register 中的逻辑自动丢弃较小的 time packet */
                    co_await outputRegPair.second -> write(internalFireTime, std::any());
                }
            }
        }
    }
}