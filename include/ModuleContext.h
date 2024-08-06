#ifndef WOLF_SIM_MODULE_CONTEXT_H
#define WOLF_SIM_MODULE_CONTEXT_H
#include "wolf_sim.h"
#include <mutex>
#include <atomic>
#include <iostream>

namespace wolf_sim {
class ModuleContext {
 public:
  void safeLog(const std::string& log);
  void setEnd(bool value);
  bool isEnd();
  private:
  std::mutex logMutex;
  std::atomic<bool> end;
};
}  // namespace wolf_sim

#endif