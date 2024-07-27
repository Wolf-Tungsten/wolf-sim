#include "Register.h"
#include <stdexcept>

namespace wolf_sim
{
    Register::Register() {
        lastWriteTime = -1;
        terminated = false;
        asInputConnected = false;
        asOutputConnected = false;
    }

    void Register::setName(std::string _name) {
        name = _name;
    }
    std::string Register::getName() {
        return name;
    }

    void Register::connectAsInput(std::weak_ptr<Module> modulePtr) {
        if(asInputConnected){
            throw std::runtime_error("This register is already used as an input for another module.");
        }
        outputToPtr = modulePtr;
        asInputConnected = true;
    }

    void Register::connectAsOutput(std::weak_ptr<Module> modulePtr) {
        if(asOutputConnected){
            throw std::runtime_error("This register is already used as an output for another module.");
        }
        inputFromPtr = modulePtr;
        asOutputConnected = true;
    }

    void Register::acquireRead() {
        std::unique_lock<std::mutex> lock(mutex);
        while(payloadQueue.empty() && !terminated){
            condWaitActive.wait(lock);
        }
        if(terminated){
            throw Module::SimulationTerminateException();
        }
    }

    Time_t Register::getActiveTime() {
        /** 需要 acquireRead 保护，一定不会为空 */
        return payloadQueue.front().first;
    }

    std::any Register::read() {
        /** 需要 acquireRead 保护，一定不会为空 */
        return payloadQueue.front().second;
    }

    void Register::pop() {
        /** 需要 acquireRead 保护，一定不会为空 */
        payloadQueue.pop_front();
    }

    void Register::releaseRead() {
        mutex.unlock();
    }

    void Register::write(Time_t _writeTime, std::any _payload) {
        std::unique_lock<std::mutex> lock(mutex);

        if(_writeTime <= lastWriteTime) {
            if(_payload.has_value()){
                // 当输入 payload 包含有效值的情况下，则不允许发生这种情况
                throw std::runtime_error("write time is early than last active");
            } else {
                // 输入的是一个空的 time packet，减少 always block 的判断
                return;
            }
        }

        lastWriteTime = _writeTime;
        // 尝试与队尾的 packet 合并
        if(!payloadQueue.empty() && !payloadQueue.back().second.has_value()){
            payloadQueue.back().first = _writeTime;
            payloadQueue.back().second = _payload;
        } else {
            payloadQueue.push_back({_writeTime, _payload});
        }

        condWaitActive.notify_all();
    }

    void Register::terminationNotify() {
        std::unique_lock<std::mutex> lock(mutex);
        terminated = true;
        condWaitActive.notify_all();
    }
} 
