#ifndef WOLF_SIM_STATE_REF_H
#define WOLF_SIM_STATE_REF_H
#include <exception>

#include "Module.h"

#define Input(name, type) \
  wolf_sim::InputRef<type> name = wolf_sim::InputRef<type>(this);
#define Output(name, type) \
  wolf_sim::OutputRef<type> name = wolf_sim::OutputRef<type>(this);
#define Reg(name, type) \
  wolf_sim::StateRef<type> name = wolf_sim::StateRef<type>(this);

namespace wolf_sim {
template <typename T>
class StateRef {
 public:
  StateRef(Module* mPtr) { this->mPtr = mPtr; }

  StateRef& operator=(T newValue) {
    modifyGuard();
    value = newValue;
    return *this;
  }

  operator const T&() { return value; }

  const T& operator*() { return value; }

  const T& r() { return value; }

  T& w() {
    modifyGuard();
    return value;
  }

 protected:
  Module* mPtr;
  T value;
  virtual void modifyGuard() {
    if (mPtr->modulePhase != Module::Phase::updateStateOutputPhase &&
        mPtr->modulePhase != Module::Phase::initPhase) {
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
  InputRef& operator=(T newValue) {
    this->modifyGuard();
    this->value = newValue;
    return *this;
  }

 protected:
  void modifyGuard() override {
    if (this->mPtr->modulePhase != Module::Phase::standByPhase) {
      throw std::runtime_error("Module input illegal update.");
    }
  }
};

template <typename T>
class OutputRef : public StateRef<T> {
 public:
  OutputRef(Module* mPtr) : StateRef<T>(mPtr) {}
  OutputRef& operator=(T newValue) {
    this->modifyGuard();
    this->value = newValue;
    return *this;
  }
  protected:
  void modifyGuard() override {
    if (this->mPtr->modulePhase != Module::Phase::updateStateOutputPhase &&
        this->mPtr->modulePhase != Module::Phase::initPhase) {
      throw std::runtime_error(
          "Module output can only be updated in init or updateStateOutput "
          "phase.");
    }
  }
};

}  // namespace wolf_sim
#endif