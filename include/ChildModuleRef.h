#ifndef WOLF_SIM_CHILD_MODULE_REF_H
#define WOLF_SIM_CHILD_MODULE_REF_H

#include <memory>

#include "Module.h"

namespace wolf_sim {
template <typename ChildModuleType>
class ChildModuleRef {
 public:
  ChildModuleRef(Module* mPtr) {
    this->mPtr = mPtr;
    childModulePtr = std::shared_ptr<ChildModuleType>();
    mPtr->addChildModule(childModulePtr);
  }
  std::shared_ptr<ChildModuleType> operator->() { return childModulePtr; }

 private:
  Module* mPtr;
  std::shared_ptr<ChildModuleType> childModulePtr;
};
}  // namespace wolf_sim
#endif