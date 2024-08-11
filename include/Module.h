
//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_MODULE_H
#define WOLF_SIM_MODULE_H

#include <any>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "ChildTickScheduler.h"
#include "ModuleContext.h"
#include "SimTime.h"

namespace wolf_sim {
class ChildTickScheduler;
class Module : public std::enable_shared_from_this<Module> {
  friend class Module;
  friend class ChildTickScheduler;
  template <typename ChildModuleType>
  friend class ChildModuleRef;
  template <typename StateType>
  friend class StateRef;
  template <typename T>
  friend class InputRef;
  template <typename T>
  friend class OutputRef;


 public:
  void tick();
  void tick(SimTime_t tickCount);
  void tickToTermination();
  bool terminated();
  void reset();
  SimTime_t whatTime();
  void setModuleLabel(std::string label) { moduleLabel = label; }
  std::string getModuleLabel() { return moduleLabel; }
  void setDeterministic(bool v) { deterministic = v; }

 protected:
  void sleepFor(SimTime_t time);
  void terminate();
  std::ostringstream& logger();
  int addChildModule(std::shared_ptr<Module> childModulePtr);

 private:
  std::string moduleLabel;

  std::vector<std::shared_ptr<Module>> staticChildren;
  std::map<int, std::shared_ptr<Module>> childrenMap;
  int nextChildrenId = 0;

  std::shared_ptr<ModuleContext> mcPtr;
  enum Phase {
    constructPhase,
    initPhase,
    standByPhase,
    updateChildInputPhase,
    tickChildrenPhase,
    updateStateOutputPhase
  } modulePhase = constructPhase;
  SimTime_t currentTime = 0;
  SimTime_t wakeUpTime = 0;

  std::ostringstream logStream;

  std::shared_ptr<ChildTickScheduler> childTickSchedulerPtr;

  bool deterministic = false;

  void tickRoutine(SimTime_t currentTime);
  void constructRoutine(std::shared_ptr<ModuleContext> mcPtr);
  void initRoutine();

  /* 留给子类重载的方法 */
  virtual void construct(){};
  virtual void init(){};
  virtual void updateChildInput(){};
  virtual void updateStateOutput(){};
};

}  // namespace wolf_sim

#endif  // WOLF_SIM_MODULE_H