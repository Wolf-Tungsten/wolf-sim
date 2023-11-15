//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_REGISTER_H
#define WOLF_SIM_REGISTER_H
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Mutex.h"
#include "async_simple/coro/ConditionVariable.h"
#include <vector>
#include <queue>
#include <map>
#include <utility>
#include <iostream>

namespace wolf_sim
{
    template <typename PayloadType, int depth>
    class Register
    {
    private:
        std::queue<std::pair<long, PayloadType>> payloadQueue;
        std::queue<long> retiredQueue;
        async_simple::coro::Mutex mutex;
        async_simple::coro::Notifier getNotifier;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condNotEmpty;
        async_simple::coro::Notifier outputDoneNotifier;
        int nextOutIdx;
        int outPortNr;
        long regGetTimestamp;
        int outputCounter;
        // 禁止拷贝
        Register(const Register &r) = delete;
        Register &operator=(const Register &r) = delete;
        // 禁止移动
        Register(Register &&r) = delete;
        Register &operator=(Register &&r) = delete;

    public:
        explicit Register()
        {
            regGetTimestamp = 0;
            nextOutIdx = 0;
            outPortNr = 0;
            outputCounter = 0;
        };

        // 分配 block Idx 
        void addOutput() {
            outPortNr++;
        }

        async_simple::coro::Lazy<void> put(AlwaysBlock &b, PayloadType p){
            co_await mutex.coLock();
            while((!retiredQueue.empty()) && b.blockTimestamp >= retiredQueue.front()){
                retiredQueue.pop();
            }
            assert(payloadQueue.size() + retiredQueue.size() <= depth);
            if (payloadQueue.size() + retiredQueue.size() == depth)
            {
                if(retiredQueue.empty()){
                    mutex.unlock();
                    // 等待 get 操作完成
                    co_await getNotifier.wait();
                    co_await mutex.coLock();
                }
                b.blockTimestamp = retiredQueue.front();
                retiredQueue.pop();
            }
            payloadQueue.push(std::make_pair(b.blockTimestamp, p));
            condNotEmpty.notifyAll();
            mutex.unlock();
        };

        async_simple::coro::Lazy<PayloadType> get(AlwaysBlock &b){
            co_await mutex.coLock();
            // 检查队列是否为空
            if (payloadQueue.empty()){
                co_await condNotEmpty.wait(mutex, [&]{ return !payloadQueue.empty(); });
            }
            // 读取队列
            std::pair<long, PayloadType> tAndp = payloadQueue.front();

            // 协调时间戳
            // 1. regGetTimestamp 至少推进到 payloadTimestamp
            if (tAndp.first > regGetTimestamp){
                regGetTimestamp = tAndp.first;
            } 
            // 2.根据 blockTimestamp 推进
            if (b.blockTimestamp > regGetTimestamp) {
                regGetTimestamp = b.blockTimestamp;
            }
            // 当所有 block 读取均完成时，regGetTimestamp = max(regGetTimestamp, payloadTimestamp, blockTimestamp0, blockTimestamp1, ...)

            outputCounter += 1;
            if(outputCounter < outPortNr){
                mutex.unlock();
                co_await outputDoneNotifier.wait();
            } else {
               outputCounter = 0;
               payloadQueue.pop();
               retiredQueue.push(regGetTimestamp);
               getNotifier.notify();
               outputDoneNotifier.notify(); 
               mutex.unlock();
            }

            b.blockTimestamp = regGetTimestamp;
            co_return tAndp.second;
        };

        async_simple::coro::Lazy<bool> nonBlockPut(AlwaysBlock &b, PayloadType p){
            co_return false;
        };
        async_simple::coro::Lazy<bool> nonBlockGet(AlwaysBlock &b, PayloadType &pRet){
            co_return false;
        };
    };

    template <typename PayloadType, int depth>
    class RegRef {
        // AlwaysBlock 通过 RegRef 持有对 Register 的指针，RegRef 也持有对 AlwaysBlock 的指针
        // AlwaysBlock 通过 RegRef 调用 Register 的 put/get/nonBlockPut/nonBlockGet 方法

        private:
            Register<PayloadType, depth> *regPtr;
            AlwaysBlock *blockPtr;
        public:
            void asInput(AlwaysBlock* blockPtr_, Register<PayloadType, depth>* regPtr_){
                blockPtr = blockPtr_;
                regPtr = regPtr_;
                regPtr->addOutput();
            }
            void asOutput(AlwaysBlock* blockPtr_, Register<PayloadType, depth>* regPtr_){
                blockPtr = blockPtr_;
                regPtr = regPtr_;
            }
            async_simple::coro::Lazy<void> put(PayloadType p){
                co_await regPtr->put(*blockPtr, p);
            }
            async_simple::coro::Lazy<PayloadType> get(){
                co_return co_await regPtr->get(*blockPtr);
            }
            async_simple::coro::Lazy<bool> nonBlockPut(PayloadType p){
                co_return co_await regPtr->nonBlockPut(*blockPtr, p);
            }
            async_simple::coro::Lazy<bool> nonBlockGet(PayloadType &pRet){
                co_return co_await regPtr->nonBlockGet(*blockPtr, pRet);
            } 
    };
}
#endif // WOLF_SIM_REGISTER_H
