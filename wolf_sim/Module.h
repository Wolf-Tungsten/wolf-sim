
//
// Created by gaoruihao on 6/23/23.
//

#ifndef WOLF_SIM_MODULE_H
#define WOLF_SIM_MODULE_H
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "wolf_sim.h"

#if ENABLE_MODULE_LOG
#define MODULE_LOG(msg) moduleLog(msg);
#else
#define MODULE_LOG(msg)
#endif

#define IPort(portName, type) \
  wolf_sim::RegReadRef<type> portName = wolf_sim::RegReadRef<type>(this);

#define IPortArray(portName, type, arraySize)        \
  std::vector<wolf_sim::RegReadRef<type>> portName = \
      std::vector<wolf_sim::RegReadRef<type>>(       \
          arraySize, wolf_sim::RegReadRef<type>(this));

#define OPort(portName, type) \
  wolf_sim::RegWriteRef<type> portName = wolf_sim::RegWriteRef<type>(this);

#define OPortArray(portName, type, arraySize)         \
  std::vector<wolf_sim::RegWriteRef<type>> portName = \
      std::vector<wolf_sim::RegWriteRef<type>>(       \
          arraySize, wolf_sim::RegWriteRef<type>(this));

#define FromChildPort(portName, type) IPort(portName, type)

#define FromChildPortArray(portName, type, arraySize) \
  IPortArray(portName, type, arraySize)

#define ToChildPort(portName, type) OPort(portName, type)

#define ToChildPortArray(portName, type, arraySize) \
  OPortArray(portName, type, arraySize)

namespace wolf_sim {

template <typename T>
class RegReadRef {
 public:
  RegReadRef(Module* _mPtr) : mPtr(_mPtr), id(-1) {};
  void connect(std::shared_ptr<Register> regPtr) {
    id = mPtr->assignInput(regPtr);
  }
  void operator <<= (std::shared_ptr<Register> regPtr) {
    connect(regPtr);
  }
  bool valid() {
    if (id == -1) {
      throw std::runtime_error("port not connected");
    }
    return mPtr->inputRegPayload.contains(id);
  }
  T read() {
    if (id == -1) {
      throw std::runtime_error("port not connected");
    }
    return std::any_cast<T>(mPtr->inputRegPayload[id]);
  }

  bool operator>>(T& value) {
    if (id == -1) {
      throw std::runtime_error("port not connected");
    }
    if(mPtr->inputRegPayload.contains(id)){
        value = std::any_cast<T>(mPtr->inputRegPayload[id]);
        return true;
    }
    return false;
  }

 private:
  Module* mPtr;
  int id;
};

template <typename T>
class RegWriteRef {
 public:
  RegWriteRef(Module* _mPtr) : mPtr(_mPtr), id(-1) {};
  void connect(std::shared_ptr<Register> regPtr) {
    id = mPtr->assignOutput(regPtr);
  }
  std::shared_ptr<Register>& operator >>= (std::shared_ptr<Register>& regPtr) {
    connect(regPtr);
    return regPtr;
  }
  void write(T writePayload, Time_t delay = 0) {
    if (id == -1) {
      throw std::runtime_error("port not connected");
    }
    mPtr->writeRegister(id, writePayload, delay, false);
  }

  void terminate(Time_t delay = 0) {
    if (id == -1) {
      throw std::runtime_error("port not connected");
    }
    mPtr->writeRegister(id, std::any(), delay, true);
  }

  void operator<<(const T& value) { write(value); }

 private:
  Module* mPtr;
  int id;
};

class Module : public std::enable_shared_from_this<Module> {
 public:
  virtual void construct() {};
  void setNameAndParent(std::string _name, std::weak_ptr<Module> _parentPtr);
  friend class Environment;
  template <typename T>
  friend class RegReadRef;
  template <typename T>
  friend class RegWriteRef;

 protected:
  std::string name;
  std::weak_ptr<Module> parentPtr;
  std::map<int, std::shared_ptr<Register>> inputRegisterMap;
  std::map<int, std::shared_ptr<Register>> outputRegisterMap;

  Time_t whatTime();
  std::map<int, std::any> inputRegPayload;
  std::vector<std::any> wakeUpPayload;
  virtual void fire() {};

  template <typename ModuleDerivedType>
  std::shared_ptr<ModuleDerivedType> createChildModule(std::string name = "") {
    static_assert(std::is_base_of<Module, ModuleDerivedType>::value,
                  "child module must be derived from Module class");
    if (name == "") {
      name = std::string("anonymous_block_") +
             std::to_string(childModuleMap.size());
    }
    if (childModuleMap.contains(name)) {
      throw std::runtime_error("create Module name conflict!");
    }
    auto p = std::make_shared<ModuleDerivedType>();
    childModuleMap[name] = p;
    p->construct();
    p->setNameAndParent(name, shared_from_this());
    return p;
  };
  std::shared_ptr<Register> createRegister(std::string name = "");
  void planWakeUp(Time_t delay, std::any wakeUpPayload = std::any());
  void writeRegister(int id, std::any writePayload, Time_t delay = 1,
                     bool terminate = false);

 private:
  void moduleLog(std::string msg) {
    std::cout << "[" << name << "] " << msg << std::endl;
  }
  Time_t internalFireTime = 0;
#if OPT_OPTIMISTIC_READ
  std::map<int, Time_t> inputRegActiveTimeOptimistic;
  std::map<int, bool> inputRegLockedOptimistic;
#endif
  std::vector<int> terminatedInputRegNote;
  std::map<int, bool> terminatedOutputRegNote;
  std::map<std::string, std::shared_ptr<Register>> childRegisterMap;
  std::map<std::string, std::shared_ptr<Module>> childModuleMap;
  std::map<int, std::pair<std::any, bool>> pendingRegisterWrite;
  struct EarliestWakeUpComparator {
    bool operator()(const std::pair<Time_t, std::any>& p1,
                    const std::pair<Time_t, std::any>& p2) {
      return p1.first > p2.first;
    }
  };
  std::priority_queue<std::pair<Time_t, std::any>,
                      std::vector<std::pair<Time_t, std::any>>,
                      EarliestWakeUpComparator>
      wakeUpSchedule;
  struct EarliestRegisterWriteComparator {
    bool operator()(const std::tuple<Time_t, int, std::any, bool>& p1,
                    const std::tuple<Time_t, int, std::any, bool>& p2) {
      return std::get<0>(p1) > std::get<0>(p2);
    }
  };
  std::priority_queue<std::tuple<Time_t, int, std::any, bool>,
                      std::vector<std::tuple<Time_t, int, std::any, bool>>,
                      EarliestRegisterWriteComparator>
      registerWriteSchedule;
  std::map<int, Time_t> pendingRegisterTerminate;
  int assignInput(std::shared_ptr<Register> regPtr);
  int assignOutput(std::shared_ptr<Register> regPtr);
  void simulationLoop();
};

}  // namespace wolf_sim

#endif  // WOLF_SIM_MODULE_H