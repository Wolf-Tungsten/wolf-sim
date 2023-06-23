//
// Created by gaoruihao on 6/23/23.
//

#include "Environment.h"
#include "async_simple/executors/SimpleExecutor.h"
#include "async_simple/coro/SyncAwait.h"
#include "async_simple/coro/Collect.h"

wolf_sim::Environment::Environment(int _threadNum) : threadNum(_threadNum){
    running = false;
}

void wolf_sim::Environment::addAlwaysBlock(AlwaysBlock &alwaysBlock) {
    if(running){
        throw std::runtime_error("Not allowed to add always block when simulation is running");
    }
    alwaysBlockVec.push_back(alwaysBlock.always());
}

async_simple::coro::Lazy<void> wolf_sim::Environment::coroStart() {
    co_await collectAllPara(std::move(alwaysBlockVec));
    co_return;
}

void wolf_sim::Environment::run() {
    async_simple::executors::SimpleExecutor executor(threadNum);
    syncAwait(coroStart().via(&executor));
}