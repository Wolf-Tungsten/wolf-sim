#include "wolf_sim.h"

class Producer : public wolf_sim::Module {
    public:
    OPort(payloadOut, int);
    IPort(readyOut, int);

    private:
    int payload = 0;
    void fire() override {
        if(readyOut.valid()){
            payload++;
        }
        payloadOut << payload;
    }
};

class FIFO : public wolf_sim::Module {
    public:
    IPort(payloadIn, int);
    OPort(readyIn, bool);
    OPort(payloadOut, int);
    IPort(readyOut, bool);

    private:
    int fifoSize = 10;
    std::deque<int> buffer;
    bool bufferFull = false;
    void fire() {
        if(readyOut.valid() && !buffer.empty()){
            buffer.pop_front();
        }

        int p;
        if(payloadIn >> p && !bufferFull){
            buffer.push_back(p);
        }
        
        if(!buffer.empty()) {
            payloadOut << buffer.front();
        }

        bufferFull = buffer.size() >= fifoSize;
        readyIn << !bufferFull;

        planWakeUp(1);
    }
};