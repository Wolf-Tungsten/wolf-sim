//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_REGISTER_H
#define WOLF_SIM_REGISTER_H
#include <vector>
#include <queue>
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
        void connectAsInput(std::weak_ptr<AlwaysBlock> blockPtr);
        void connectAsOutput(std::weak_ptr<AlwaysBlock> blockPtr);
        // 寄存器支持的操作
        ReturnNothing acquireRead();
        void releaseRead();
        Time_t getActiveTime();
        std::any read();
        void clear();
        ReturnNothing write(Time_t _writeTime, std::any _payload=std::any(), bool overwrite=false);
    private:
        bool asInputConnected;
        bool asOutputConnected;
        std::weak_ptr<AlwaysBlock> outputToPtr;
        std::weak_ptr<AlwaysBlock> inputFromPtr;
        bool active;
        std::atomic<Time_t> activeTime;
        std::any payload;
        async_simple::coro::Mutex mutex;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condWaitActive;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condWaitInactive;
    };
}
#endif // WOLF_SIM_REGISTER_H
