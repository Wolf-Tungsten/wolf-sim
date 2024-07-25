//
// Created by gaoruihao on 6/23/23.
//

#include "Environment.h"
#include "async_simple/executors/SimpleExecutor.h"
#include "async_simple/coro/SyncAwait.h"
#include "async_simple/coro/Collect.h"

namespace wolf_sim
{
    Environment::Environment(int _threadNum) : threadNum(_threadNum) {};
   

    void Environment::addBlock(std::shared_ptr<AlwaysBlock> blockPtr)
    {
        alwaysBlockPtrVec.push_back(blockPtr);
        for (const auto &internalBlockPair : blockPtr->internalAlwaysBlockMap)
        {
            addBlock(internalBlockPair.second);
        }
    }

    async_simple::coro::Lazy<void> Environment::coroStart()
    {
        std::vector<async_simple::coro::Lazy<void>> alwaysCoroVec;
        for (auto alwaysBlockPtr : alwaysBlockPtrVec)
        {
            alwaysCoroVec.push_back(alwaysBlockPtr->simulationLoop());
        }
        co_await async_simple::coro::collectAllPara(std::move(alwaysCoroVec));
        co_return;
    }

    void wolf_sim::Environment::run()
    {
        async_simple::executors::SimpleExecutor executor(threadNum);
        syncAwait(coroStart().via(&executor));
    }
};

