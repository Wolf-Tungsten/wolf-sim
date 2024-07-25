#include "Register.h"
#include <stdexcept>

namespace wolf_sim
{
    Register::Register() {
        active = false;
        activeTime = 0;
        asInputConnected = false;
        asOutputConnected = false;
    }

    void Register::connectAsInput(AlwaysBlock* blockPtr) {
        if(asInputConnected){
            throw std::runtime_error("This register is already used as an input for another block.");
        }
        outputToPtr = blockPtr;
        asInputConnected = true;
    }

    void Register::connectAsOutput(AlwaysBlock* blockPtr) {
        if(asOutputConnected){
            throw std::runtime_error("This register is already used as an output for another block.");
        }
        inputFromPtr = blockPtr;
        asOutputConnected = true;
    }

    ReturnNothing Register::acquireRead() {
        co_await mutex.coLock();
        co_await condWaitActive.wait(mutex, [&]{ return active; });
    }

    Time_t Register::getActiveTime() {
        return activeTime;
    }

    std::any Register::read() {
        return payload;
    }

    void Register::clear() {
        active = false;
        condWaitInactive.notifyOne();
    }

    void Register::releaseRead() {
        mutex.unlock();
    }

    ReturnNothing Register::write(Time_t _writeTime, std::any _payload, bool overwrite) {
        co_await mutex.coLock();

        if(_writeTime <= activeTime) {
            if(_payload.has_value()){
                // 当输入 payload 包含有效值的情况下，则不允许发生这种情况
                throw std::runtime_error("write time is early than last active");
            } else {
                // 允许忽略 time packet，减少 always block 的判断
                mutex.unlock();
                co_return;
            }
        }

        if(!overwrite && payload.has_value()){
            // 如果不是 overwrite 请求，并且当前寄存器中有值，那就要等待排空了
            co_await condWaitInactive.wait(mutex, [&]{ return !active; });
        } 
        
        // activeTime 仍然是单调增长的
        active = true;
        activeTime = _writeTime;
        payload = _payload;

        condWaitActive.notifyAll();
        mutex.unlock();
    }

} 
