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
  void setTerminated(bool value);
  bool getTerminated();
  private:
  std::mutex logMutex;
  std::atomic<bool> terminated;
};
}  // namespace wolf_sim

#endif