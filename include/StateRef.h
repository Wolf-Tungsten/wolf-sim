#ifndef WOLF_SIM_STATE_REF_H
#define WOLF_SIM_STATE_REF_H
#include <exception>

#include "Module.h"

namespace wolf_sim {
template <typename T>
class StateRef {
 public:
  StateRef(Module* mPtr) { this->mPtr = mPtr; }

  void operator=(T newValue) {
    modifyGuard();
    value = newValue;
  }

  const T& operator*() { return value; }

  const T& r() { return value; }

  T& w() {
    modifyGuard();
    return value;
  }

 private:
  Module* mPtr;
  T value;
  virtual void modifyGuard() {
    if (mPtr->moduleStatus != ModuleStatus::computeStateOutput &&
        mPtr->moduleStatus != ModuleStatus::init) {
      throw std::runtime_error(
          "Module state can only be updated in init or updateStateOutput "
          "phase.");
    }
  }
};

template <typename T>
class InputRef : public StateRef<T> {
 public:
  InputRef(Module* mPtr) : StateRef<T>(mPtr) {}
  void modifyGuard() override {
    if (mPtr->moduleStatus != ModuleStatus::standBy) {
      throw std::runtime_error(
          "Module input illegal update.");
    }
  }
};

template <typename T>
class OutputRef : public StateRef<T> {
 public:
  OutputRef(Module* mPtr) : StateRef<T>(mPtr) {}
  void modifyGuard() override {
    if (mPtr->moduleStatus != ModuleStatus::updateStateOutput) {
      throw std::runtime_error(
          "Module output can only be updated in updateStateOutput phase.");
    }
  }
};


}  // namespace wolf_sim
#endif