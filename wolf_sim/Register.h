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
        std::string regIdentifier;
        bool debugEnabled;
        std::queue<std::pair<long, PayloadType>> payloadQueue;
        std::queue<long> retiredQueue;
        async_simple::coro::Mutex mutex;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condRetiredQueueNotEmpty;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condPayloadQueueNotEmpty;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condNextOutput;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condConsumerBlockNotReady;
        std::vector<bool> signInSheet;
        int outPortNr;
        long regGetTimestamp;
        int outputCounter;
        bool consumerBlockReady;
        // 禁止拷贝
        Register(const Register &r) = delete;
        Register &operator=(const Register &r) = delete;
        // 禁止移动
        Register(Register &&r) = delete;
        Register &operator=(Register &&r) = delete;

    public:
        explicit Register()
        {
            debugEnabled = false;
            regGetTimestamp = 0;
            outPortNr = 0;
            outputCounter = 0;
            consumerBlockReady = false;
        };

        void enableDebug(std::string name){
            debugEnabled = true;
            regIdentifier = name;
        }

        void disableDebug(){
            debugEnabled = false;
        }

        // 分配 outIdx 
        int addOutput() {
            signInSheet.push_back(false);
            return outPortNr++;
        }

        async_simple::coro::Lazy<void> put(AlwaysBlock &b, PayloadType p){
            co_await mutex.coLock();

            if(debugEnabled){
                std::cout << "Register " << regIdentifier << " requested put " << p << " with blockTimestamp=" << b.blockTimestamp << std::endl;
                // print payload queue and retired queue , echo queue in 1 line
                std::cout << "Register " << regIdentifier << " payloadQueue: ";
                std::queue<std::pair<long, PayloadType>> payloadQueueCopy = payloadQueue;
                while(!payloadQueueCopy.empty()){
                    std::cout << "[t=" << payloadQueueCopy.front().first << ", p=" << payloadQueueCopy.front().second << "]";
                    payloadQueueCopy.pop();
                }
                std::cout << std::endl;
                std::cout << "Register " << regIdentifier << " retiredQueue: ";
                std::queue<long> retiredQueueCopy = retiredQueue;
                while(!retiredQueueCopy.empty()){
                    std::cout << "[t=" << retiredQueueCopy.front() << "]";
                    retiredQueueCopy.pop();
                }
                std::cout << std::endl;
            }

            while((!retiredQueue.empty()) && b.blockTimestamp >= retiredQueue.front()){
                if(debugEnabled){
                    std::cout << "Register " << regIdentifier << " pop from retiredQueue " << retiredQueue.front() << std::endl;
                }
                retiredQueue.pop();
            }
            
            assert(payloadQueue.size() + retiredQueue.size() <= depth);
            if (payloadQueue.size() + retiredQueue.size() == depth)
            {
                if(debugEnabled && retiredQueue.empty()){
                    std::cout << "Register " << regIdentifier << " wait for retiredQueue not empty" << std::endl;
                }
                co_await condRetiredQueueNotEmpty.wait(mutex, [&]{ return !retiredQueue.empty(); });
                
                if(debugEnabled){
                    std::cout << "Register " << regIdentifier << " update blockTimestamp to " << retiredQueue.front() << std::endl;
                }
                b.blockTimestamp = retiredQueue.front();
                retiredQueue.pop();
            }

            if(debugEnabled){
                std::cout << "Register " << regIdentifier << " push to payloadQueue: " << p << std::endl;
            }
            payloadQueue.push(std::make_pair(b.blockTimestamp, p));
            condPayloadQueueNotEmpty.notifyAll();
            mutex.unlock();
        };

        async_simple::coro::Lazy<PayloadType> get(AlwaysBlock &b, int outIdx){
            co_await mutex.coLock();

            if(debugEnabled){
                std::cout << "Register " << regIdentifier << " requested get by " << b.blockIdentifier << " from port " << outIdx << " at blockTimestamp=" << b.blockTimestamp << std::endl;
                // print payload queue and retired queue , echo queue in 1 line
                std::cout << "Register " << regIdentifier << " payloadQueue: ";
                std::queue<std::pair<long, PayloadType>> payloadQueueCopy = payloadQueue;
                while(!payloadQueueCopy.empty()){
                    std::cout << "[t=" << payloadQueueCopy.front().first << ", p=" << payloadQueueCopy.front().second << "]";
                    payloadQueueCopy.pop();
                }
                std::cout << std::endl;
                std::cout << "Register " << regIdentifier << " retiredQueue: ";
                std::queue<long> retiredQueueCopy = retiredQueue;
                while(!retiredQueueCopy.empty()){
                    std::cout << "[t=" << retiredQueueCopy.front() << "]";
                    retiredQueueCopy.pop();
                }
                std::cout << std::endl;
            }

            co_await condNextOutput.wait(mutex, [&]{ return !signInSheet[outIdx]; });
            signInSheet[outIdx] = true; 

            // 检查队列是否为空
            co_await condPayloadQueueNotEmpty.wait(mutex, [&]{ return !payloadQueue.empty(); });
            
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

            if(outPortNr == 1){
                // 只有一个 consumer，不需要等待
                b.blockTimestamp = regGetTimestamp;
                payloadQueue.pop(); // 将 payloadQueue 出队
                retiredQueue.push(regGetTimestamp); // 将 regGetTimestamp 入队
                condRetiredQueueNotEmpty.notifyAll(); // 通知 Producer 有新的 retired
                signInSheet[outIdx] = false;  
            } else {
                outputCounter += 1;
                if(outputCounter < outPortNr){
                    // 等待所有 consumer 都准备好提取数据
                    co_await condConsumerBlockNotReady.wait(mutex, [&]{ return consumerBlockReady; });
                    // 互斥执行，防止多个 consumer 同时减少 outputCounter
                    outputCounter--;
                    b.blockTimestamp = regGetTimestamp;
                    if(outputCounter == 0){ // 最后一个被唤醒的 consumer 负责将 consumerBlockReady 置为 false
                        consumerBlockReady = false;
                        signInSheet.assign(signInSheet.size(), false);
                        payloadQueue.pop(); // 将 payloadQueue 出队
                        retiredQueue.push(regGetTimestamp); // 将 regGetTimestamp 入队
                        condRetiredQueueNotEmpty.notifyAll(); // 通知 Producer 有新的 retired
                        condNextOutput.notifyAll(); // 通知所有 consumer 有新的 output
                    }
                } else {
                    // 最后一个请求的 consumer 负责将 consumerBlockReady 置为 true
                    consumerBlockReady = true;
                    outputCounter--;
                    b.blockTimestamp = regGetTimestamp;
                    condConsumerBlockNotReady.notifyAll();
                }
            }
            
            mutex.unlock();
            co_return tAndp.second;
        };

    };

    template <typename PayloadType, int depth>
    class RegRef {
        // AlwaysBlock 通过 RegRef 持有对 Register 的指针，RegRef 也持有对 AlwaysBlock 的指针
        // AlwaysBlock 通过 RegRef 调用 Register 的 put/get/nonBlockPut/nonBlockGet 方法

        private:
            std::shared_ptr<Register<PayloadType, depth>> regPtr;
            AlwaysBlock *blockPtr;
            int outIdx;
        public:
            void asInput(AlwaysBlock* blockPtr_, std::shared_ptr<Register<PayloadType, depth>> regPtr_){
                blockPtr = blockPtr_;
                regPtr = regPtr_;
                outIdx = regPtr->addOutput();
            }
            void asOutput(AlwaysBlock* blockPtr_, std::shared_ptr<Register<PayloadType, depth>> regPtr_){
                blockPtr = blockPtr_;
                regPtr = regPtr_;
            }
            async_simple::coro::Lazy<void> put(PayloadType p){
                return regPtr->put(*blockPtr, p);
            }
            async_simple::coro::Lazy<PayloadType> get(){
                return regPtr->get(*blockPtr, outIdx);
            }
            async_simple::coro::Lazy<void> asyncPut(PayloadType p){
                // 进行 put，但是不按照 put 过程更新 block 的时间戳
                // 在仿真语义上，asyncPut 不推进时间戳
                long t = blockPtr->blockTimestamp;
                co_await regPtr->put(*blockPtr, p);
                blockPtr->blockTimestamp = t;
                co_return;
            }
            async_simple::coro::Lazy<PayloadType> asyncGet(){
                long t = blockPtr->blockTimestamp;
                auto payload = co_await regPtr->get(*blockPtr, outIdx);
                blockPtr->blockTimestamp = t;
                co_return payload;
            }
    };
}
#endif // WOLF_SIM_REGISTER_H
