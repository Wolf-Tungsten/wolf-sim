#ifndef WOLF_SIM_MODULE_CONTEXT_H
#define WOLF_SIM_MODULE_CONTEXT_H
#include <mutex>
#include <atomic>
#include <iostream>

namespace wolf_sim {
class ModuleContext {
 public:
  void safeLog(const std::string& log);
  private:
  std::mutex logMutex;
};
}  // namespace wolf_sim

#endif