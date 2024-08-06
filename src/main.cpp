#include "ChildModuleRef.h"
#include "Module.h"
#include "StateRef.h"

class MyModule: wolf_sim::Module {
    public:
    wolf_sim::ChildModuleRef<MyModule> myChildModule = wolf_sim::ChildModuleRef<MyModule>(this);
    void wolfWolf(){
        myChildModule->wolfWolf();
    }
    private:
    wolf_sim::StateRef<std::vector<int>> myState = wolf_sim::StateRef<std::vector<int>>(this);
    wolf_sim::StateRef<int> myStateInt = wolf_sim::StateRef<int>(this);
    void updateStateOutput() {
        myStateInt.w() = 1;
        int a = *myStateInt;
        myStateInt = 2;
    }

};

int main() {
    
}