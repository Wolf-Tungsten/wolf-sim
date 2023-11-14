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
#include <mutex>
#include <condition_variable>

namespace wolf_sim
{
    template <typename PayloadType, int depth>
    class Register
    {
    private:
        std::queue<std::pair<long, PayloadType>> internalQueue;
        std::mutex mutex;
        std::condition_variable condNotFull;
        std::condition_variable condNotEmpty;
        std::condition_variable condNextOutputReady;
        uint64_t getFlag;             // 当 outPort 数量小于等于 64 时使用 bit 操作进行
        std::vector<bool> getFlagVec; // 当 outPort 数量超过 64 时退化成 vec 操作
        int nextOutIdx;
        int outPortNr;
        long regPutTimestamp;
        // 禁止拷贝构造
        Register(const Register &r) = delete;
        // 禁止赋值
        Register &operator=(const Register &r) = delete;
        // 移动构造
        Register(Register &&r) noexcept
        {
            internalQueue = std::move(r.internalQueue);
            mutex = std::move(r.mutex);
            condNotFull = std::move(r.condNotFull);
            condNotEmpty = std::move(r.condNotEmpty);
            condNextOutputReady = std::move(r.condNextOutputReady);
            getFlag = r.getFlag;
            getFlagVec = std::move(r.getFlagVec);
            nextOutIdx = r.nextOutIdx;
            outPortNr = r.outPortNr;
            regPutTimestamp = r.regPutTimestamp;
            r.outPortNr = 0;
        }
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

        void put(AlwaysBlock &b, PayloadType p){
            std::unique_lock<std::mutex> lock(mutex);
            if (internalQueue.size() >= depth)
            {
                while(internalQueue.size() >= depth){
                    condNotFull.wait(lock);
                }
                if (regPutTimestamp > b.blockTimestamp)
                {
                    b.blockTimestamp = regPutTimestamp;
                }
            }
            internalQueue.push(std::make_pair(b.blockTimestamp, p));
            condNotEmpty.notify_all();
        };

        PayloadType get(AlwaysBlock &b, int outIdx){
            std::unique_lock<std::mutex> lock(mutex);
            // 检查 getFlag 当前 block 是否在本轮访存中已读取过
            bool flag = outPortNr <= 64 ? (getFlag & (1 << outIdx)) : getFlagVec[outIdx];
            while(flag){
                condNextOutputReady.wait(lock);
                flag = outPortNr <= 64 ? (getFlag & (1 << outIdx)) : getFlagVec[outIdx];
            }
                
            // 检查队列是否为空
            while(internalQueue.empty()){
                condNotEmpty.wait(lock);
            }
            // 读取队列
            auto tAndp = internalQueue.front();
            std::cout << "get " << tAndp.second << std::endl;
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
                    condNotFull.notify_all();
                    condNextOutputReady.notify_all();
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
                    condNotFull.notify_all();
                    condNextOutputReady.notify_all();
                }
            }
            return tAndp.second;
        };

        bool nonBlockPut(AlwaysBlock &b, PayloadType p){
            std::unique_lock<std::mutex> lock(mutex);
            if (internalQueue.size() >= depth)
            {
                if (regPutTimestamp > b.blockTimestamp)
                {
                    b.blockTimestamp = regPutTimestamp;
                }
                return false;
            }
            internalQueue.push(std::make_pair(b.blockTimestamp, p));
            condNotEmpty.notify_all();
            return true;
        };

        bool nonBlockGet(AlwaysBlock &b, int outIdx, PayloadType &pRet){
            std::unique_lock<std::mutex> lock(mutex);
            // 检查 getFlag 当前 block 是否在本轮访存中已读取过
            bool flag = outPortNr <= 64 ? (getFlag & (1 << outIdx)) : getFlagVec[outIdx];
            while(flag){
                condNextOutputReady.wait(lock);
                flag = outPortNr <= 64 ? (getFlag & (1 << outIdx)) : getFlagVec[outIdx];
            }
            // 检查队列是否为空，如果为空的话直接解锁返回
            if (internalQueue.empty()){
                return false;
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
                    condNotFull.notify_all();
                    condNextOutputReady.notify_all();
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
                    condNotFull.notify_all();
                    condNextOutputReady.notify_all();
                }
            }
            mutex.unlock();
            pRet = tAndp.second;
            return true;
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
            void asInput(AlwaysBlock* blockPtr_, Register<PayloadType, depth>* regPtr_){
                blockPtr = blockPtr_;
                regPtr = regPtr_;
                outIdx = regPtr->allocOutIdx();
            }
            void asOutput(AlwaysBlock* blockPtr_, Register<PayloadType, depth>* regPtr_){
                blockPtr = blockPtr_;
                regPtr = regPtr_;
            }
            void put(PayloadType p){
                regPtr->put(*blockPtr, p);
            }
            PayloadType get(){
                return regPtr->get(*blockPtr, outIdx);
            }
            bool nonBlockPut(PayloadType p){
                return regPtr->nonBlockPut(*blockPtr, p);
            }
            bool nonBlockGet(PayloadType &pRet){
                return regPtr->nonBlockGet(*blockPtr, outIdx, pRet);
            } 
    };
}
#endif // WOLF_SIM_REGISTER_H
