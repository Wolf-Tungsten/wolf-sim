#include "Module.h"

namespace wolf_sim {

void Module::setNameAndParent(std::string _name,
                              std::weak_ptr<Module> _parentPtr) {
  name = _name;
  parentPtr = _parentPtr;
}

int Module::assignInput(std::shared_ptr<Register> reg) {
  int id = nextInputId++;
  inputRegisterMap[id] = reg;
  reg->connectAsInput(shared_from_this());
  return id;
}

int Module::assignOutput(std::shared_ptr<Register> reg) {
  int id = nextOutputId++;
  outputRegisterMap[id] = reg;
  reg->connectAsOutput(shared_from_this());
  return id;
}

std::shared_ptr<Register> Module::ejectInput(int id){
  if (!inputRegisterMap.contains(id)) {
    throw std::runtime_error("input register not found");
  }
  auto regPtr = inputRegisterMap[id];
  inputRegisterMap.erase(id);
  return regPtr;
}

std::shared_ptr<Register> Module::ejectOutput(int id){
  if (!outputRegisterMap.contains(id)) {
    throw std::runtime_error("output register not found");
  }
  auto regPtr = outputRegisterMap[id];
  outputRegisterMap.erase(id);
  return regPtr;
}

std::shared_ptr<Register> Module::createRegister(std::string name) {
  if (name == "") {
    name =
        std::string("anonymous_reg_") + std::to_string(childRegisterMap.size());
  }
  if (childRegisterMap.contains(name)) {
    throw std::runtime_error("create Register name conflict!");
  }
  auto p = std::make_shared<Register>();
  p->setName(name);
  childRegisterMap[name] = p;
  return p;
}

void Module::planWakeUp(Time_t delay, std::any wakeUpPayload) {
  if (delay <= 0) {
    throw std::runtime_error("delay should be positive");
  }
  wakeUpSchedule.push(std::make_pair(internalFireTime + delay, wakeUpPayload));
}

void Module::sleepFor(Time_t delay) {
  planWakeUp(delay);
  isSleeping = true;
  sleepWakeUpTime = internalFireTime + delay;
} 

void Module::writeRegister(int id, std::any writePayload, Time_t delay) {
  if (delay < 0) {
    throw std::runtime_error("delay should not be negetive");
  } else if (delay == 0) {
    pendingRegisterWrite[id] = writePayload;
  } else {
    registerWriteSchedule.push(
        std::make_tuple(internalFireTime + delay, id, writePayload));
  }
}

void Module::terminateSimulation() {
  terminationNotify(); // 向上向下通知所有模块准备终止
  // 额外抛出异常
  throw SimulationTerminateException();
}

void Module::terminateModuleSimulation() {
  // 只终止当前模块
  throw SimulationTerminateException();
}

void Module::terminationNotify() {
  // 如果当前模块已终止则忽略
  if (!hasTerminated) {
    // 将自己标记为终止
    hasTerminated = true;
    // 通知所有输出寄存器终止
    for (const auto& regPair : outputRegisterMap) {
      regPair.second->terminationNotify();
    }
    // 如果有父 Module，通知父 Module
    if (!parentPtr.expired()) {
      auto parent = parentPtr.lock();
      parent->terminationNotify();
    }
    // 如果有子 Module，通知子 Module
    for (const auto& childPair : childModuleMap) {
      childPair.second->terminationNotify();
    }
  }
}

void Module::simulationLoop() {
  try {
    /** 在 wakeUpScheduler 中放一个 0 时刻的启动 */
    wakeUpSchedule.push(std::make_pair(0, std::any()));
    int loopCount = 0;  // 调试信息
    // !inputRegisterMap.empty() ||
    // !wakeUpSchedule.empty() ||
    // !registerWriteSchedule.empty()
    while (!hasTerminated) {
      /* 计算最小唤醒时间 */
      register Time_t minTime = MAX_TIME;
#if OPT_OPTIMISTIC_READ
      inputRegLockedOptimistic.clear();
#endif
      MODULE_LOG("loop " + std::to_string(loopCount) + " 0.开始计算 minTime");
      if (!wakeUpSchedule.empty()) {
        Time_t wakeUpTime = wakeUpSchedule.top().first;
        if (wakeUpTime < minTime) {
          minTime = wakeUpTime;
        }
      }
      MODULE_LOG("loop " + std::to_string(loopCount) +
                 " 1.计划唤醒的 minTime= " +
                 (minTime == MAX_TIME ? "MAX_TIME" : std::to_string(minTime)));
      for (const auto& inputRegPair : inputRegisterMap) {
        if (minTime == 0) {
          MODULE_LOG("loop " + std::to_string(loopCount) +
                     " 1.1.申请寄存器锁被冷启动跳过");
          break;
        }
        int regId = inputRegPair.first;
        auto regPtr = inputRegPair.second;
#if OPT_OPTIMISTIC_READ
        if (inputRegActiveTimeOptimistic.contains(regId) &&
            inputRegActiveTimeOptimistic[regId] > minTime) {
          continue;
        }
#endif
        MODULE_LOG("loop " + std::to_string(loopCount) + " 1.1.申请锁定寄存器" +
                   regPtr->getName());
        regPtr->acquireRead();
        register Time_t regActiveTime = regPtr->getActiveTime();
        MODULE_LOG("loop " + std::to_string(loopCount) + " 1.2.寄存器 " +
                   regPtr->getName() + " 锁定成功，最近激活时间 " + std::to_string(regActiveTime));
#if OPT_OPTIMISTIC_READ
        inputRegLockedOptimistic[regId] = true;
        inputRegActiveTimeOptimistic[regId] = regActiveTime;
#endif
        if (regActiveTime < minTime) {
          minTime = regActiveTime;
        }
      }
      MODULE_LOG("loop " + std::to_string(loopCount) +
                 " 2.输入寄存器的 minTime= " +
                 (minTime == MAX_TIME ? "MAX_TIME" : std::to_string(minTime)));
      if (!registerWriteSchedule.empty()) {
        register Time_t writeTime = std::get<0>(registerWriteSchedule.top());
        if (writeTime < minTime) {
          minTime = writeTime;
        }
      }
      MODULE_LOG("loop " + std::to_string(loopCount) +
                 " 3.计划写入的（最终） minTime= " +
                 (minTime == MAX_TIME ? "MAX_TIME" : std::to_string(minTime)));
      MODULE_LOG("loop " + std::to_string(loopCount) + " 4.读取寄存器");
      /* 构造传给 fire 的 payload 列表 */
      inputRegPayload.clear();
      for (const auto& inputRegPair : inputRegisterMap) {
        if (minTime == 0) {
          break;
        }
#if OPT_OPTIMISTIC_READ
        int regId = inputRegPair.first;
        if (!inputRegLockedOptimistic.contains(regId)) {
          continue;
        }
#endif
        auto regPtr = inputRegPair.second;
        Time_t regActiveTime = regPtr->getActiveTime();
        std::any payload = regPtr->read();
        MODULE_LOG("loop " + std::to_string(loopCount) + " 4.1.读取寄存器 " +
                   regPtr->getName() + " 读取时间 " +
                   std::to_string(regActiveTime) + " minTime " + std::to_string(minTime));
        if (regActiveTime == minTime) {
          MODULE_LOG("loop " + std::to_string(loopCount) +
                     " 4.2.读取并弹出寄存器 " + regPtr->getName() + " 读取时间 " + std::to_string(regActiveTime));
          if (payload.has_value()) {
            inputRegPayload[inputRegPair.first] = payload;
          }
          regPtr->pop();
        }
        regPtr->releaseRead();
        MODULE_LOG("loop " + std::to_string(loopCount) + " 4.2.释放寄存器 " +
                   regPtr->getName());
      }
      MODULE_LOG("loop " + std::to_string(loopCount) + " 5.查找计划唤醒任务");
      wakeUpPayload.clear();
      while (!wakeUpSchedule.empty() && wakeUpSchedule.top().first == minTime) {
        MODULE_LOG("loop " + std::to_string(loopCount) +
                   " 5.1.找到计划唤醒任务");
        wakeUpPayload.push_back(wakeUpSchedule.top().second);
        wakeUpSchedule.pop();
      }
      /* 将寄存器写请求构造出来 */
      MODULE_LOG("loop " + std::to_string(loopCount) + " 6.查找寄存器写入任务");
      pendingRegisterWrite.clear();
      while (!registerWriteSchedule.empty() &&
             std::get<0>(registerWriteSchedule.top()) == minTime) {
        const auto& regWriteTuple = registerWriteSchedule.top();
        int id = std::get<1>(regWriteTuple);
        std::any payload = std::get<2>(regWriteTuple);
        pendingRegisterWrite[id] = payload;
        registerWriteSchedule.pop();
        MODULE_LOG("loop " + std::to_string(loopCount) + " 6.1.将写入寄存器 " +
                   outputRegisterMap[id]->getName());
      }
      if (minTime == MAX_TIME) {
        MODULE_LOG("loop " + std::to_string(loopCount) +
                   " 7.没有找到有效的 minTime，模块仿真结束");
        throw SimulationTerminateException();
      } else if (internalFireTime > 0 && minTime <= internalFireTime) {
        MODULE_LOG("loop " + std::to_string(loopCount) +
                   " 7.找到有效的 minTime " + std::to_string(minTime) +
                   " 但是早于上次 fire 时间 " +
                   std::to_string(internalFireTime));
        throw std::runtime_error("time sequence error");
      }
      /* 记录（可能的）fire 时间 */
      internalFireTime = minTime;
      /* 如果有 payload 就 fire */
      if (!inputRegPayload.empty() || !wakeUpPayload.empty()) {
        if(!isSleeping){
          MODULE_LOG("loop " + std::to_string(loopCount) +
                   " 7.执行 fire， 本次启动时间 " +
                   std::to_string(internalFireTime));
          fire();
        } else if (minTime == sleepWakeUpTime) {
          MODULE_LOG("loop " + std::to_string(loopCount) +
                   " 7.执行 fire， 本次启动时间 " +
                   std::to_string(internalFireTime));
          fire();
          isSleeping = false;
        }
      }
      /* 将 fire 产生和之前计划的寄存器写动作写出 */
      for (const auto& outputRegPair : outputRegisterMap) {
        int id = outputRegPair.first;
        if (pendingRegisterWrite.contains(id)) {
          std::any payload = pendingRegisterWrite[id];
          MODULE_LOG("loop " + std::to_string(loopCount) + " 8.写输出寄存器 " +
                     outputRegPair.second->getName() + " 写入时间 " +
                     std::to_string(internalFireTime + 1));
          outputRegPair.second->write(internalFireTime + 1, payload);
        } else {
          /* 向所有没有写入的寄存器打入一个空的 time packet，
          这里并不检查这些寄存器的 activeTime，交给 register
          中的逻辑自动丢弃较小的 time packet */
          MODULE_LOG("loop " + std::to_string(loopCount) +
                     " 8.写输出寄存器 timepacket " +
                     outputRegPair.second->getName() + " 写入时间 " +
                     std::to_string(internalFireTime + 1));
          outputRegPair.second->write(internalFireTime + 1, std::any());
        }
      }
      loopCount++;
    }
  } catch (const SimulationTerminateException& e) {
    MODULE_LOG("SimulationLoop terminated");
    if(!hasTerminated){
      /* 如果是自然结束或者模块终止的情况会执行到这里，将所有输出寄存器设置为终止 */
      for(const auto& outputRegPair: outputRegisterMap){
        outputRegPair.second->terminationNotify();
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Exception caught in Simulation: " << e.what() << std::endl;
    exit(1);
  }
  finalStop();
}
}  // namespace wolf_sim