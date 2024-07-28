#include "wolf_sim/wolf_sim.h"

class ChildProducer : public wolf_sim::Module {
    public:
    OPort(payloadOPort, int);

    private:
    int payload = 0;
    void fire() {
        payloadOPort << payload;
        payload++;
        if(payload < 10) {
            planWakeUp(5);
        }
    }
};

class ParentProducer: public wolf_sim::Module {
    public:
    OPort(payloadOPort, int);
    private:
    void construct() {
        auto childProducer = createChildModule<ChildProducer>("childProducer");
        childProducer->payloadOPort >>= payloadOPort;
    }
};

class ChildConsumer : public wolf_sim::Module {
    public:
    IPort(payloadIPort, int);
    private:
    void fire() {
        int payload;
        if (payloadIPort >> payload) {
            std::cout << "ChildConsumer got payload " << payload << " at " << whatTime() << std::endl;
        }
    }
};

class ParentConsumer : public wolf_sim::Module {
    public:
    IPort(payloadIPort, int);
    private:
    void construct() {
        auto childConsumer = createChildModule<ChildConsumer>("childConsumer");
        payloadIPort >>= childConsumer->payloadIPort;
    }
};

class Top : public wolf_sim::Module {
    public:
    private:
    void construct() {
        auto parentProducer = createChildModule<ParentProducer>("parentProducer");
        auto parentConsumer = createChildModule<ParentConsumer>("parentConsumer");
        auto reg = createRegister("reg");
        parentProducer->payloadOPort >>= reg;
        parentConsumer->payloadIPort <<= reg;
    }
    void finalStop() {
        std::cout << "Top terminated at " << whatTime() << std::endl;
    }
};

int main() {
    auto env = std::make_shared<wolf_sim::Environment>();
    auto topModule = std::make_shared<Top>();
    env->addTopModule(topModule);
    env->run();
    return 0;
}