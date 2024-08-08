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

#define ChildModuleArray(name, ChildModuleType, size) \
  std::vector<wolf_sim::ChildModuleRef<ChildModuleType>> name = \
      std::vector<wolf_sim::ChildModuleRef<ChildModuleType>>(size, wolf_sim::ChildModuleRef<ChildModuleType>(this, #name));

#define ChildModuleArrayWithLabel(name, ChildModuleType, size, label) \
  std::vector<wolf_sim::ChildModuleRef<ChildModuleType>> name = \
      std::vector<wolf_sim::ChildModuleRef<ChildModuleType>>(size, wolf_sim::ChildModuleRef<ChildModuleType>(this, label)); 
      
namespace wolf_sim {
template <typename ChildModuleType>
class ChildModuleRef {
 public:
  ChildModuleRef(Module* mPtr, std::string label) {
    this->mPtr = mPtr;
    childModulePtr = std::make_shared<ChildModuleType>();
    childModulePtr -> setModuleLabel(label);
    // 静态生成的模块先加入到父模块的静态子模块列表中
    mPtr->staticChildren.push_back(childModulePtr);
  }
  std::shared_ptr<ChildModuleType> operator->() { return childModulePtr; }

 private:
  Module* mPtr;
  std::shared_ptr<ChildModuleType> childModulePtr;
};
}  // namespace wolf_sim
#endif