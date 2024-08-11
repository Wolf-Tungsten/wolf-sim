#include "Module.h"

#include <sstream>
namespace wolf_sim {

int Module::addChildModule(std::shared_ptr<Module> childModulePtr) {
  if(modulePhase != Phase::constructPhase) {
    throw std::runtime_error("Module can only add child in construct phase.");
  }
  childrenMap[nextChildrenId] = childModulePtr;
  return nextChildrenId++;
}

void Module::constructRoutine(std::shared_ptr<ModuleContext> mcPtr) {
  modulePhase = Phase::constructPhase;  
  nextChildrenId = 0;
  childrenMap.clear();
  this->mcPtr = mcPtr;
  /* 调用当前模块的 construct 方法 */
  construct();
  for(auto &child : staticChildren) {
    addChildModule(child);
  }
  /* construct 方法中允许会动态添加模块 */
  childTickSchedulerPtr = std::make_shared<ChildTickScheduler>();
  childTickSchedulerPtr->setup(this);
  if(deterministic){
    childTickSchedulerPtr->forceSerial();
  }
  /* 递归调用子模块的 constructRoutine */
  for (auto& child : childrenMap) {
    child.second->setDeterministic(deterministic);
    child.second->constructRoutine(mcPtr);
  }
}

void Module::initRoutine() {
  /* 递归调用子模块的 initRoutine */
  for (auto& child : childrenMap) {
    child.second->initRoutine();
  }
  /* 模块内部变量的初始化 */
  currentTime = 0;
  wakeUpTime = 0;
  /* 调用当前模块的 init 方法 */
  modulePhase = Phase::initPhase;
  init();
  modulePhase = Phase::standByPhase;
}

void Module::tickRoutine(SimTime_t currentTime) {
  /* 更新当前时间 */
  this->currentTime = currentTime;
  if (currentTime >= wakeUpTime) {
    /* 更新子模块输入 */
    modulePhase = Phase::updateChildInputPhase;
    updateChildInput();
  }
  /* 递归调用子模块的 tickRoutine */
  modulePhase = Phase::tickChildrenPhase;
  childTickSchedulerPtr->scheduledTick(currentTime);
  if (currentTime >= wakeUpTime) {
    /* 更新当前模块状态输出 */
    modulePhase = Phase::updateStateOutputPhase;
    updateStateOutput();
    /* 将当前模块的 log 输出*/
    if (!logStream.str().empty()) {
      mcPtr->safeLog(logStream.str());
      logStream.str("");
      logStream.clear();
    }
    bool childTerminated = false;
    for (auto& child : childrenMap) {
      childTerminated = child.second->terminated() || childTerminated; 
    }
    terminatedFlag = childTerminated || terminatedFlag;
    /* 状态恢复到 standby */
    modulePhase = Phase::standByPhase;
  }
}

void Module::sleepFor(SimTime_t time) {
  if (modulePhase != updateStateOutputPhase) {
    throw std::runtime_error(
        "Module can only plan sleep in updateStateOutput phase.");
  }
  wakeUpTime = currentTime + time;
}

bool Module::terminated() {
  return terminatedFlag;
}


SimTime_t Module::whatTime() {
  if (mcPtr != nullptr) {
    return currentTime;
  } else {
    return 0;
  }
}

std::ostringstream& Module::logger() {
  if (moduleLabel.empty()) {
    logStream << "[*" << static_cast<void*>(this) << " @ " << whatTime()
              << "] ";
  } else {
    logStream << "[" << getModuleLabel() << " @ " << whatTime() << "] ";
  }
  return logStream;
}

void Module::terminate() {
  terminatedFlag = true;
}

/* 顶层模块调用 tick、tickToEnd 方法 */
void Module::tick() {
  /* 首先判断 mcPtr 是否为空，如果为空则是模型第一次 tick */
  if (mcPtr == nullptr) {
    /* 创建 ModuleContext 对象 */
    mcPtr = std::make_shared<ModuleContext>();
    /* 调用 constructRoutine 方法 */
    constructRoutine(mcPtr);
    /* 调用 initRoutine 方法 */
    initRoutine();
  }
  if(terminated()) {
    throw std::runtime_error("Module has been terminated");
  }
  /* 调用 tickRoutine 方法 */
  tickRoutine(currentTime);
  currentTime++;
}

void Module::tick(SimTime_t tickCount) {
  for (SimTime_t i = 0; i < tickCount && !terminated(); i++) {
    tick();
  }
}

void Module::tickToTermination() {
  while (!terminated()) {
    tick();
  }
}

}  // namespace wolf_sim