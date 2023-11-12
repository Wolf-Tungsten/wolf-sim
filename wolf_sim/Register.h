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

namespace wolf_sim
{
    template <typename PayloadType, int depth>
    class Register
    {
    private:
        std::queue<std::pair<long, PayloadType>> internalQueue;
        async_simple::coro::Mutex mutex;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condNotFull;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condNotEmpty;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condNextOutputReady;
        uint64_t getFlag;             // 当 outPort 数量小于等于 64 时使用 bit 操作进行
        std::vector<bool> getFlagVec; // 当 outPort 数量超过 64 时退化成 vec 操作
        int nextOutIdx;
        int outPortNr;
        long regPutTimestamp;
        // 禁止拷贝构造
        Register(const Register &r) = delete;
        // 禁止赋值
        Register &operator=(const Register &r) = delete;

    public:
        explicit Register()
        {
            regPutTimestamp = 0;
            nextOutIdx = 0;
            getFlag = 0;
            outPortNr = 0;
            for (int i = 0; i < outPortNr; i++)
            {
                getFlagVec.push_back(false);
            }
        };

        // 分配 block Idx 
        int allocOutIdx() {
            outPortNr++;
            return nextOutIdx++;
        }

        async_simple::coro::Lazy<void> put(AlwaysBlock &b, PayloadType p){
            co_await mutex.coLock();
            if (internalQueue.size() >= depth)
            {
                co_await condNotFull.wait(mutex, [&]
                                          { return internalQueue.size() < depth; });
                if (regPutTimestamp > b.blockTimestamp)
                {
                    b.blockTimestamp = regPutTimestamp;
                }
            }
            internalQueue.push(std::make_pair(b.blockTimestamp, p));
            condNotEmpty.notifyAll();
            mutex.unlock();
        };

        async_simple::coro::Lazy<PayloadType> get(AlwaysBlock &b, int outIdx){
            co_await mutex.coLock();
            // 检查 getFlag 当前 block 是否在本轮访存中已读取过
            bool flag = outPortNr <= 64 ? (getFlag & (1 << outIdx)) : getFlagVec[outIdx];
            if (flag){
                co_await condNextOutputReady.wait(mutex, [&]{ 
                    flag = outPortNr <= 64 ? (getFlag & (1 << outIdx)) : getFlagVec[outIdx];
                    return !flag; });
            }
            // 检查队列是否为空
            if (internalQueue.empty()){
                co_await condNotEmpty.wait(mutex, [&]{ return !internalQueue.empty(); });
            }
            // 读取队列
            auto tAndp = internalQueue.front();
            // 协调时间戳

            if (tAndp.first > b.blockTimestamp){
                b.blockTimestamp = tAndp.first;
                regPutTimestamp = tAndp.first;
            } else {
                regPutTimestamp = b.blockTimestamp;
            }
            // 标记 getFlag
            if (outPortNr <= 64){
                getFlag |= (1 << outIdx);
            } else {
                getFlagVec[outIdx] = true;
            }
            // 检查是否所有 block 均完成读取
            if (outPortNr <= 64){
                if (getFlag == (1 << outPortNr) - 1){
                    getFlag = 0;
                    internalQueue.pop();
                    condNotFull.notifyAll();
                    condNextOutputReady.notifyAll();
                }
            } else {
                bool allFlag = true;
                for (int i = 0; i < outPortNr; i++){
                    allFlag &= getFlagVec[i];
                }
                if (allFlag){
                    for (int i = 0; i < outPortNr; i++){
                        getFlagVec[i] = false;
                    }
                    internalQueue.pop();
                    condNotFull.notifyAll();
                    condNextOutputReady.notifyAll();
                }
            }
            mutex.unlock();
            co_return tAndp.second;
        };
        async_simple::coro::Lazy<bool> nonBlockPut(AlwaysBlock &b, PayloadType p){
            co_await mutex.coLock();
            if (internalQueue.size() >= depth)
            {
                if (regPutTimestamp > b.blockTimestamp)
                {
                    b.blockTimestamp = regPutTimestamp;
                }
                mutex.unlock();
                co_return false;
            }
            internalQueue.push(std::make_pair(b.blockTimestamp, p));
            condNotEmpty.notifyAll();
            mutex.unlock(); 
            co_return true;
        };
        async_simple::coro::Lazy<bool> nonBlockGet(AlwaysBlock &b, int outIdx, PayloadType &pRet){
            co_await mutex.coLock();
            // 检查 getFlag 当前 block 是否在本轮访存中已读取过
            bool flag = outPortNr <= 64 ? (getFlag & (1 << outIdx)) : getFlagVec[outIdx];
            if (flag){
                co_await condNextOutputReady.wait(mutex, [&]{ 
                    flag = outPortNr <= 64 ? (getFlag & (1 << outIdx)) : getFlagVec[outIdx];
                    return !flag; });
            }
            // 检查队列是否为空
            if (internalQueue.empty()){
                mutex.unlock();
                co_return false;
            }
            // 读取队列
            auto tAndp = internalQueue.front();
            // 协调时间戳
            if (tAndp.first > b.blockTimestamp){
                b.blockTimestamp = tAndp.first;
                regPutTimestamp = tAndp.first;
            } else {
                regPutTimestamp = b.blockTimestamp;
            }
            // 标记 getFlag
            if (outPortNr <= 64){
                getFlag |= (1 << outIdx);
            } else {
                getFlagVec[outIdx] = true;
            }
            // 检查是否所有 block 均完成读取
            if (outPortNr <= 64){
                if (getFlag == (1 << outPortNr) - 1){
                    getFlag = 0;
                    internalQueue.pop();
                    condNotFull.notifyAll();
                    condNextOutputReady.notifyAll();
                }
            } else {
                bool allFlag = true;
                for (int i = 0; i < outPortNr; i++){
                    allFlag &= getFlagVec[i];
                }
                if (allFlag){
                    for (int i = 0; i < outPortNr; i++){
                        getFlagVec[i] = false;
                    }
                    internalQueue.pop();
                    condNotFull.notifyAll();
                    condNextOutputReady.notifyAll();
                }
            }
            mutex.unlock();
            pRet = tAndp.second;
            co_return true;
        };
    };

    template <typename PayloadType, int depth>
    class RegRef {
        // AlwaysBlock 通过 RegRef 持有对 Register 的指针，RegRef 也持有对 AlwaysBlock 的指针
        // AlwaysBlock 通过 RegRef 调用 Register 的 put/get/nonBlockPut/nonBlockGet 方法
        // RegRef 记录当前 AlwaysBlock 对所持有 Register 的 outIdx
        // RegRef 通过 bind 建立绑定关系

        private:
            Register<PayloadType, depth> *regPtr;
            AlwaysBlock *blockPtr;
            int outIdx;
        public:
            void bind(AlwaysBlock* blockPtr_, Register<PayloadType, depth>* regPtr_){
                blockPtr = blockPtr_;
                regPtr = regPtr_;
                outIdx = regPtr->allocOutIdx();
            }
            async_simple::coro::Lazy<void> put(PayloadType p){
                co_await regPtr->put(*blockPtr, p);
            }
            async_simple::coro::Lazy<PayloadType> get(){
                co_return co_await regPtr->get(*blockPtr, outIdx);
            }
            async_simple::coro::Lazy<bool> nonBlockPut(PayloadType p){
                co_return co_await regPtr->nonBlockPut(*blockPtr, p);
            }
            async_simple::coro::Lazy<bool> nonBlockGet(PayloadType &pRet){
                co_return co_await regPtr->nonBlockGet(*blockPtr, outIdx, pRet);
            } 
    };
}
#endif // WOLF_SIM_REGISTER_H
