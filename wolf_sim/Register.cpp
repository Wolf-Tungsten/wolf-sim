#include "Register.h"
#include <stdexcept>

namespace wolf_sim
{
    Register::Register() {
        lastWriteTime = 0;
        terminated = false;
        asInputConnected = false;
        asOutputConnected = false;
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

    ReturnNothing Register::acquireRead() {
        co_await mutex.coLock();
        co_await condWaitActive.wait(mutex, [&]{ return (!payloadQueue.empty()) || terminated; });
    }

    Time_t Register::getActiveTime() {
        /** 受到 acquireRead 保护，一定不会为空 */
        return payloadQueue.front().first;
    }

    std::any Register::read() {
        /** 受到 acquireRead 保护，一定不会为空 */
        std::cout << "寄存器读 " << payloadQueue.front().second.has_value() << std::endl;
        return payloadQueue.front().second;
    }

    void Register::pop() {
        /** 受到 acquireRead 保护，一定不会为空 */
        payloadQueue.pop_front();
    }

    bool Register::hasTerminated() {
        return terminated && payloadQueue.empty();
    }

    void Register::releaseRead() {
        mutex.unlock();
    }

    ReturnNothing Register::write(Time_t _writeTime, std::any _payload) {
        co_await mutex.coLock();

        if(_writeTime <= lastWriteTime) {
            if(_payload.has_value()){
                // 当输入 payload 包含有效值的情况下，则不允许发生这种情况
                throw std::runtime_error("write time is early than last active");
            } else {
                // 输入的是一个空的 time packet，减少 always block 的判断
                mutex.unlock();
                co_return;
            }
        }

        lastWriteTime = _writeTime;
        // 尝试与队尾的 packet 合并
        std::cout << "寄存器写前size " << payloadQueue.size() << " value=" << _payload.has_value() << std::endl;
        if(!payloadQueue.empty() && !payloadQueue.back().second.has_value()){
            payloadQueue.back().first = _writeTime;
            payloadQueue.back().second = _payload;
        } else {
            payloadQueue.push_back({_writeTime, _payload});
        }
        std::cout << "寄存器写后size " << payloadQueue.size() << std::endl;

        condWaitActive.notifyAll();
        mutex.unlock();
    }

    ReturnNothing Register::terminate(Time_t _writeTime) {
        co_await mutex.coLock();
        if(_writeTime <= lastWriteTime){
            throw std::runtime_error("terminate time is early than last active");
        }
        lastWriteTime = _writeTime;
        terminated = true;
        condWaitActive.notifyAll();
        mutex.unlock();
    }
} 
