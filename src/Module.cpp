#include "Module.h"
#include <sstream>
namespace wolf_sim {

void Module::reset() { mcPtr = nullptr; }

void Module::configRoutine(std::shared_ptr<ModuleContext> mcPtr) {
  childTickScheduler.setup(shared_from_this());
  this->mcPtr = mcPtr;
  /* 调用当前模块的 config 方法 */
  config();
  /* 递归调用子模块的 configRoutine */
  for (auto& child : childrenMap) {
    child.second->configRoutine(mcPtr);
  }
}

void Module::initRoutine() {
  /* 模块内部变量的初始化 */
  currentTime = 0;
  wakeUpTime = 0;
  /* 调用当前模块的 init 方法 */
  moduleStatus = ModuleStatus::init;
  init();
  moduleStatus = ModuleStatus::standBy;
  /* 递归调用子模块的 initRoutine */
  for (auto& child : childrenMap) {
    child.second->initRoutine();
  }
}

void Module::tickRoutine(Time_t currentTime) {
  /* 更新当前时间 */
  this->currentTime = currentTime;
  if (currentTime < wakeUpTime) {
    return;
  }
  /* 更新子模块输入 */
  moduleStatus = ModuleStatus::updateChildInput;
  updateChildInput();
  /* 递归调用子模块的 tickRoutine */
  moduleStatus = ModuleStatus::tickChildren;
  childTickScheduler.scheduledTick(currentTime);
  /* 更新当前模块状态输出 */
  moduleStatus = ModuleStatus::updateStateOutput;
  updateStateOutput();
  /* 将当前模块的 log 输出*/
  if(!logStream.str().empty()) {
    mcPtr->safeLog(logStream.str());
    logStream.str("");
    logStream.clear();
  }
  /* 状态恢复到 standby */
  moduleStatus = ModuleStatus::standBy;
  /* 推进 currentTime */
  ++currentTime;
}

void Module::sleepFor(Time_t time) { wakeUpTime = currentTime + time; }

bool Module::terminated() {
  if (mcPtr != nullptr) {
    return mcPtr->getTerminated();
  } else {
    return false;
  }
}

Time_t Module::whatTime() {
  if (mcPtr != nullptr) {
    return currentTime;
  } else {
    return 0;
  }
}

std::ostringstream& Module::logger() {
  if(moduleLabel.empty()) {
    logStream << "[*" << static_cast<void*>(this) << " @ " << whatTime() << "] " << std::endl;
  } else {
    logStream << "[" << getModuleLabel() << " @ " << whatTime() << "] " << std::endl;
  }
  return logStream;
}

void Module::terminate() {
  if (mcPtr != nullptr) {
    mcPtr->setTerminated(true);
  }
}


/* 顶层模块调用 tick、tickToEnd 方法 */
void Module::tick() {
  /* 首先判断 mcPtr 是否为空，如果为空则是模型第一次 tick */
  if (mcPtr == nullptr) {
    /* 创建 ModuleContext 对象 */
    mcPtr = std::make_shared<ModuleContext>();
    mcPtr->setTerminated(false);
    /* 调用 configRoutine 方法 */
    configRoutine(mcPtr);
    /* 调用 initRoutine 方法 */
    initRoutine();
  }
  /* 调用 tickRoutine 方法 */
  tickRoutine(currentTime);
}

void Module::tick(Time_t tickCount) {
  for (Time_t i = 0; i < tickCount && !terminated(); i++) {
    tick();
  }
}

void Module::tickToEnd() {
  while (!terminated()) {
    tick();
  }
}

}  // namespace wolf_sim