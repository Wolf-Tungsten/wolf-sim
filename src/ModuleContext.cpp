#include "wolf_sim.h"

namespace wolf_sim {
 void ModuleContext::safeLog(const std::string& log) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << log;
  }
}  // namespace wolf_sim

