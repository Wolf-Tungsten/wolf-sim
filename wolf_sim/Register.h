//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_REGISTER_H
#define WOLF_SIM_REGISTER_H
#include <vector>
#include <deque>
#include <map>
#include <utility>
#include <iostream>
#include <any>
#include <memory>

#include "wolf_sim.h"

namespace wolf_sim
{   
    
    class Register {
    public:
        Register();
        // 禁止拷贝
        Register(const Register &r) = delete;
        Register &operator=(const Register &r) = delete;
        void connectAsInput(std::weak_ptr<Module> modulePtr);
        void connectAsOutput(std::weak_ptr<Module> modulePtr);
        // 寄存器读操作
        ReturnNothing acquireRead();
        Time_t getActiveTime();
        bool hasTerminated();
        std::any read();
        void pop();
        void releaseRead();
        // 寄存器写操作
        ReturnNothing write(Time_t _writeTime, std::any _payload=std::any());
        ReturnNothing terminate(Time_t _writeTime);
    private:
        bool asInputConnected;
        bool asOutputConnected;
        std::weak_ptr<Module> outputToPtr;
        std::weak_ptr<Module> inputFromPtr;
        std::deque<std::pair<Time_t, std::any>> payloadQueue;
        Time_t lastWriteTime;
        bool terminated;
        async_simple::coro::Mutex mutex;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condWaitActive;
    };
}
#endif // WOLF_SIM_REGISTER_H
