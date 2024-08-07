#ifndef WOLF_SIM_CHILD_MODULE_REF_H
#define WOLF_SIM_CHILD_MODULE_REF_H

#include <memory>

#include "Module.h"

#define ChildModuleWithLabel(name, ChildModuleType, label) \
  wolf_sim::ChildModuleRef<ChildModuleType> name = \
      wolf_sim::ChildModuleRef<ChildModuleType>(this, label);

#define ChildModule(name, ChildModuleType) \
  wolf_sim::ChildModuleRef<ChildModuleType> name = \
      wolf_sim::ChildModuleRef<ChildModuleType>(this, #name);
      
namespace wolf_sim {
template <typename ChildModuleType>
class ChildModuleRef {
 public:
  ChildModuleRef(Module* mPtr, std::string label) {
    this->mPtr = mPtr;
    childModulePtr = std::make_shared<ChildModuleType>();
    childModulePtr -> setModuleLabel(label);
    mPtr->addChildModule(childModulePtr);
  }
  std::shared_ptr<ChildModuleType> operator->() { return childModulePtr; }

 private:
  Module* mPtr;
  std::shared_ptr<ChildModuleType> childModulePtr;
};
}  // namespace wolf_sim
#endif