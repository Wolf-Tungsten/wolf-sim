//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_REGISTER_H
#define WOLF_SIM_REGISTER_H
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Mutex.h"
#include "async_simple/coro/ConditionVariable.h"
#include <vector>

namespace wolf_sim {
    template<typename PayloadType>
    class Register {
    private:
        PayloadType payload;
        async_simple::coro::Mutex mutex;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condNotFull;
        async_simple::coro::ConditionVariable<async_simple::coro::Mutex> condNotEmpty;
        std::vector<bool> fullStatus;
        double timestamp;
        bool allCleared();
        void coordinateTimestamp(double& extTimestamp);
    public:
        explicit Register(int size);
        async_simple::coro::Lazy<void> put(PayloadType p, double& extTimestamp);
        async_simple::coro::Lazy<PayloadType> get(int index, double& extTimestamp);
        async_simple::coro::Lazy<void> nonBlockPut(PayloadType p, double& extTimestamp);
        async_simple::coro::Lazy<bool> nonBlockGet(int index, double& extTimestamp, PayloadType &p);
    };

    template<typename PayloadType>
    async_simple::coro::Lazy<void> Register<PayloadType>::put(PayloadType p, double &extTimestamp) {
        co_await mutex.coLock();
        co_await condNotFull.wait(mutex, [&] {
            return this->allCleared();
        });
        this->coordinateTimestamp(extTimestamp);
        for (auto && status : fullStatus) {
            status = true;
        }
        payload = p;
        condNotEmpty.notifyAll();
        mutex.unlock();
    }

    template<typename PayloadType>
    Register<PayloadType>::Register(int size) {
        for (int i = 0; i < size; i++) {
            fullStatus.push_back(false);
        }
        timestamp = 0;
    }

    template<typename PayloadType>
    bool Register<PayloadType>::allCleared() {
        bool res = true;
        for (bool status: fullStatus) {
            res = res && !status;
        }
        return res;
    }

    template<typename PayloadType>
    void Register<PayloadType>::coordinateTimestamp(double &extTimestamp) {
        if (extTimestamp > timestamp) {
            timestamp = extTimestamp;
        } else {
            extTimestamp = timestamp;
        }
    }

    template<typename PayloadType>
    async_simple::coro::Lazy<void> Register<PayloadType>::nonBlockPut(PayloadType p, double &extTimestamp) {
        co_await mutex.coLock();
        coordinateTimestamp(extTimestamp);
        bool justDoIt = allCleared();
        if (justDoIt) {
            for (int i = 0; i < fullStatus.size(); i++) {
                fullStatus[i] = true;
            }
            payload = p;
            condNotEmpty.notifyAll();
        }
        mutex.unlock();
        co_return justDoIt;
    }

    template<typename PayloadType>
    async_simple::coro::Lazy<bool>
    Register<PayloadType>::nonBlockGet(int index, double &extTimestamp, PayloadType &p) {
        co_await mutex.coLock();
        coordinateTimestamp(extTimestamp);
        bool posFullStatus = this->fullStatus[index];
        if (posFullStatus) {
            p = payload;
            this->fullStatus[index] = false;
        }
        if (allCleared()) {
            condNotFull.notifyAll();
        }
        mutex.unlock();
        co_return posFullStatus;
    }

    template<typename PayloadType>
    async_simple::coro::Lazy<PayloadType> Register<PayloadType>::get(int index, double &extTimestamp) {
        co_await mutex.coLock();
        co_await condNotEmpty.wait(mutex, [&] {
            return fullStatus[index];
        });
        coordinateTimestamp(extTimestamp);
        PayloadType p = payload;
        fullStatus[index] = false;
        if (allCleared()) {
            condNotFull.notifyAll();
        }
        mutex.unlock();
        co_return p;
    }
}
#endif //WOLF_SIM_REGISTER_H
