#include "ChildTickScheduler.h"

#include <chrono>
#include <thread>

namespace wolf_sim {
void ChildTickScheduler::setup(std::weak_ptr<Module> module) {
  this->module = module;
  reset();
}

void ChildTickScheduler::reset() {
  mode = Mode::pending;
  benchmarkCount = 0;
  serialOverhead = 0;
  parallelOverhead = 0;
}

void ChildTickScheduler::forceSerial() { mode = Mode::serial; }

void ChildTickScheduler::forceParallel() { mode = Mode::parallel; }

bool ChildTickScheduler::determined() {
  return mode != Mode::parallel || mode != Mode::serial;
}

void ChildTickScheduler::scheduledTick(Time_t currentTime) {
  if (mode == Mode::pending) {
    /* 判断子模块是不是全都确定了 */
    if (allChildrenDetermined()) {
      mode = Mode::benchmarkSerial;
    }
    serialTick(currentTime);
  } else if (mode == Mode::benchmarkSerial) {
    auto start = std::chrono::high_resolution_clock::now();
    serialTick(currentTime);
    auto end = std::chrono::high_resolution_clock::now();
    serialOverhead +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
    ++benchmarkCount;
    if (benchmarkCount >= MAX_BENCHMARK_COUNT) {
      benchmarkCount = 0;
      mode = Mode::benchmarkParallel;
    }
  } else if (mode == Mode::benchmarkParallel) {
    auto start = std::chrono::high_resolution_clock::now();
    parallelTick(currentTime);
    auto end = std::chrono::high_resolution_clock::now();
    parallelOverhead +=
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
    ++benchmarkCount;
    if (benchmarkCount >= MAX_BENCHMARK_COUNT) {
      if (parallelOverhead < serialOverhead) {
        mode = Mode::parallel;
      } else {
        mode = Mode::serial;
      }
    }
  } else if (mode == Mode::serial) {
    serialTick(currentTime);
  } else if (mode == Mode::parallel) {
    parallelTick(currentTime);
  }
}

bool ChildTickScheduler::allChildrenDetermined() {
  for (auto& p : module.lock()->childrenMap) {
    auto child = p.second;
    if (!child->childTickScheduler.determined()) {
      return false;
    }
  }
  return true;
}

void ChildTickScheduler::serialTick(Time_t currentTime) {
  for (auto& p : module.lock()->childrenMap) {
    auto child = p.second;
    child->tickRoutine(currentTime);
  }
}

void ChildTickScheduler::parallelTick(Time_t currentTime) {
  std::vector<std::thread> threads;
  for (auto& p : module.lock()->childrenMap) {
    auto child = p.second;
    threads.push_back(std::thread(
        [child, currentTime]() { child->tickRoutine(currentTime); }));
  }
  for (auto& t : threads) {
    t.join();
  }
}

}  // namespace wolf_sim