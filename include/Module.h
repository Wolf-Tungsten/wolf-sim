
//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_MODULE_H
#define WOLF_SIM_MODULE_H

#include <any>
#include <map>
#include <memory>
#include <string>

#include "ModuleContext.h"

namespace wolf_sim {

class Module : public std::enable_shared_from_this<Module> {
 public:
  void tick();
  void tick(Time_t tickCount);
  void tickToEnd();
  bool end();
  void reset();
  friend class Module;
  Time_t whatTime() { return currentTime; }
  void setModuleLabel(std::string label) { moduleLabel = label; }
  std::string getModuleLabel() { return moduleLabel; }

 protected:
  
  void sleepFor(Time_t time);
  void setModuleLabel(std::string label);
  void terminate();
  std::ostringstream& logger();

 private:

  std::string moduleLabel;

  std::map<int, std::shared_ptr<Module>> childrenMap;
  int nextChildrenId;
  int addChildModule(std::shared_ptr<Module> childModulePtr);

  std::shared_ptr<ModuleContext> mcPtr;
  enum ModuleStatus {
    init,
    standBy,
    updateChildInput,
    tickChildren,
    updateStateOutput
  } moduleStatus;
  Time_t currentTime;
  Time_t wakeUpTime;

  std::ostringstream logStream;

  void tickRoutine(Time_t currentTime);
  void configRoutine(std::shared_ptr<ModuleContext> mcPtr);
  void initRoutine();
  void tickChildren(Time_t currentTime);

  /* 留给子类重载的方法 */
  virtual void config(){};
  virtual void init(){};
  virtual void updateChildInput(){};
  virtual void updateStateOutput(){};
};

}  // namespace wolf_sim

#endif  // WOLF_SIM_MODULE_H