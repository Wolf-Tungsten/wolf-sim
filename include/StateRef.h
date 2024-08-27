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
  template <typename U>
  friend class StateRef;
  template <typename U>
  friend class InputRef;
  template <typename U>
  friend class OutputRef;

 public:
  StateRef(Module* mPtr) : value(T()) { this->mPtr = mPtr; }

  StateRef& operator=(const T& newValue) {
    modifyGuard();
    value = newValue;
    return *this;
  }
  StateRef& operator=(const StateRef<T>& stateRef) {
    modifyGuard();
    value = stateRef.value;
    return *this;
  }

  operator const T&() { return value; }

  const T& r() { return value; }

  T& w() {
    modifyGuard();
    return value;
  }

 protected:
  Module* mPtr;
  T value;
  virtual void modifyGuard() {
    if (mPtr->modulePhase != Module::Phase::updateChildInputPhase &&
        mPtr->modulePhase != Module::Phase::updateStateOutputPhase &&
        mPtr->modulePhase != Module::Phase::initPhase &&
        mPtr->modulePhase != Module::Phase::constructPhase) {
      throw std::runtime_error(
          "Module state illegal update.");
    }
  }
};

template <typename T>
class InputRef : public StateRef<T> {
 public:
  InputRef(Module* mPtr) : StateRef<T>(mPtr) {}
  InputRef& operator=(const T& newValue) {
    this->modifyGuard();
    this->value = newValue;
    return *this;
  }
  InputRef& operator=(const StateRef<T>& stateRef) {
    this->modifyGuard();
    this->value = stateRef.value;
    return *this;
  }
  InputRef& operator=(const InputRef<T>& inputRef) {
    this->modifyGuard();
    this->value = inputRef.value;
    return *this;
  }

 protected:
  void modifyGuard() override {
    if (this->mPtr->modulePhase != Module::Phase::standByPhase &&
        this->mPtr->modulePhase != Module::Phase::constructPhase) {
      throw std::runtime_error("Module input illegal update.");
    }
  }
};

template <typename T>
class OutputRef : public StateRef<T> {
 public:
  OutputRef(Module* mPtr) : StateRef<T>(mPtr) {}
  OutputRef& operator=(const T& newValue) {
    this->modifyGuard();
    this->value = newValue;
    return *this;
  }
  OutputRef& operator=(const StateRef<T>& stateRef) {
    this->modifyGuard();
    this->value = stateRef.value;
    return *this;
  }
  OutputRef& operator=(const OutputRef<T>& outputRef) {
    this->modifyGuard();
    this->value = outputRef.value;
    return *this;
  }

 protected:
  void modifyGuard() override {
    if (this->mPtr->modulePhase != Module::Phase::updateChildInputPhase &&
        this->mPtr->modulePhase != Module::Phase::updateStateOutputPhase &&
        this->mPtr->modulePhase != Module::Phase::initPhase &&
        this->mPtr->modulePhase != Module::Phase::standByPhase &&
        this->mPtr->modulePhase != Module::Phase::constructPhase) {
      throw std::runtime_error(
          "Module output illegal update.");
    }
  }
};

}  // namespace wolf_sim
#endif