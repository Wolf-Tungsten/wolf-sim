#include "wolf_sim.h"

namespace wolf_sim {
 void ModuleContext::safeLog(const std::string& log) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << log;
  }

  void ModuleContext::setTerminated(bool value) {
    terminated = value;
  }

  bool ModuleContext::getTerminated() {
    return terminated;
  }
}  // namespace wolf_sim

