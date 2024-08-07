#ifndef WOLF_SIM_MODULE_CONTEXT_H   
#define WOLF_SIM_MODULE_CONTEXT_H

#include "wolf_sim.h"

namespace wolf_sim {
 void ModuleContext::safeLog(const std::string& log) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << log;
  }

  void ModuleContext::setEnd(bool value) {
    end = value;
  }

  bool ModuleContext::isEnd() {
    return end;
  }
}  // namespace wolf_sim

#endif