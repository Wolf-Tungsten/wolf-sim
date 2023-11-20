#include "wolf_sim/Environment.h"
#include "wolf_sim/AlwaysBlock.h"
#include "wolf_sim/Register.h"
#include <iostream>
#include <memory>

class BSFullAdder : public wolf_sim::AlwaysBlock {
    private:
        wolf_sim::RegRef<bool, 1> xIn;
        wolf_sim::RegRef<bool, 1> yIn;
        wolf_sim::RegRef<bool, 1> sOut;
        int mulWidth;
        int bitCount;
        int idx;
        bool d;

    public:
        BSFullAdder(std::shared_ptr<wolf_sim::Register<bool, 1>> xIn_, 
        std::shared_ptr<wolf_sim::Register<bool, 1>> yIn_, 
        std::shared_ptr<wolf_sim::Register<bool, 1>> sOut_, 
        int mulWidth_,
        int idx_) : d(false), idx(idx_), mulWidth(mulWidth_) {
            xIn.asInput(this, xIn_);
            yIn.asInput(this, yIn_);
            sOut.asOutput(this, sOut_);
            bitCount = 0;
        }

        wolf_sim::ReturnNothing always() override {
            bool s = 0;
            while(1) {
                co_await sOut.put(s);

                bool x = co_await xIn.get();
                bool y;
                if(idx == 0){
                    y = s;
                } else {
                    y = co_await yIn.get();
                }
                s = x ^ y ^ d;
                d = x & y | x & d | y & d;
                
                //std::cout << "full adder i=" << idx << " x=" << x << " y=" << y << " s=" << s << " d=" << d << std::endl;
                blockTimestamp += 1;
                // if((++bitCount) > 2*mulWidth) {
                //     d = 0;
                //     prevS = 0;
                //     bitCount = 0;
                // }
            }
        }
};

class BSInputDriver : public wolf_sim::AlwaysBlock {
    private:
        std::vector<int> aVec;
        std::vector<int> xVec;
        std::vector<wolf_sim::RegRef<bool, 1>> bsXVec;
        int mulWidth;

    public:
        BSInputDriver(std::vector<int> aVec_, std::vector<int> xVec_, std::vector<std::shared_ptr<wolf_sim::Register<bool, 1>>> bsXVec_, int mulWidth_) : aVec(aVec_), xVec(xVec_), mulWidth(mulWidth_) {
            for(int i = 0; i < mulWidth; i++) {
                bsXVec.emplace_back();
                bsXVec[i].asOutput(this, bsXVec_[i]);
            }
            assert(aVec.size() == xVec.size());
        }

        wolf_sim::ReturnNothing always() override {
            while(!aVec.empty()){
                int a = aVec.back();
                int x = xVec.back();
                aVec.pop_back();
                xVec.pop_back();
                for(int i = 0; i < mulWidth; i++) {
                    bool xBits = x & 1;
                    x >>= 1;
                    for(int j = 0; j < mulWidth; j++) {
                        co_await bsXVec[j].put(((a >> (mulWidth - j - 1)) & 1) & xBits);
                    }
                    blockTimestamp += 1;
                }
                for(int i = 0; i < mulWidth-1; i++) {
                    for(int j = 0; j < mulWidth; j++) {
                        co_await bsXVec[j].put(0);
                    }
                    blockTimestamp += 1;
                }
            }
        }
};

class BSOutputMonitor : public wolf_sim::AlwaysBlock {
    private:
    std::vector<int> aVec;
    std::vector<int> xVec;
    int mulWidth;
    wolf_sim::RegRef<bool, 1> yReg;

    public:
    BSOutputMonitor(std::vector<int> aVec_, std::vector<int> xVec_, std::shared_ptr<wolf_sim::Register<bool, 1>> y_, int mulWidth_) : aVec(aVec_), xVec(xVec_), mulWidth(mulWidth_) {
        yReg.asInput(this, y_);
    }

    wolf_sim::ReturnNothing always() override {
        while(!aVec.empty()) {
            int a = aVec.back();
            int x = xVec.back();
            aVec.pop_back();
            xVec.pop_back();
            int y = 0;
            for(int i = 0; i < mulWidth * 2-1; i++) {
                bool yBit = co_await yReg.get();
                if(i > 0){
                    y |= yBit << (i-1);
                }
                blockTimestamp += 1;
            }
            std::cout << "a: " << a << " x: " << x << " y: " << y << std::endl;
            assert(a * x == y);
        }
        std::cout << "Simulation successfully finished at:" << blockTimestamp << std::endl;
        exit(0);
    }
};

class BitSerialMultiplier {
    private:
        std::vector<int> aVec;
        std::vector<int> xVec;
        std::vector<std::shared_ptr<BSFullAdder>> bsFullAdderVec;
        std::shared_ptr<BSInputDriver> bsInputDriver;
        std::shared_ptr<BSOutputMonitor> bsOutputMonitor;
        std::vector<std::shared_ptr<wolf_sim::Register<bool,1>>> bsXVecReg;
        std::vector<std::shared_ptr<wolf_sim::Register<bool,1>>> bsYVecReg;
        wolf_sim::Environment& env;
        int mulWidth;
    
    public:
    BitSerialMultiplier(std::vector<int> aVec_, std::vector<int> xVec_, wolf_sim::Environment& env_, int mulWidth_) : aVec(aVec_), xVec(xVec_), env(env_), mulWidth(mulWidth_) {
        for(int i = 0; i < mulWidth; i++) {
            bsXVecReg.emplace_back(std::make_shared<wolf_sim::Register<bool, 1>>());
            bsYVecReg.emplace_back(std::make_shared<wolf_sim::Register<bool, 1>>());
        }
        std::shared_ptr<wolf_sim::Register<bool, 1>> nullReg = std::make_shared<wolf_sim::Register<bool, 1>>();
        for(int i = 0; i < mulWidth; i++) {
            if(i == 0){
                bsFullAdderVec.emplace_back(std::make_shared<BSFullAdder>(bsXVecReg[i], nullReg, bsYVecReg[i], mulWidth, i));
            } else {
                bsFullAdderVec.emplace_back(std::make_shared<BSFullAdder>(bsXVecReg[i], bsYVecReg[i-1], bsYVecReg[i], mulWidth, i));
            }
            env.addAlwaysBlock(bsFullAdderVec[i]);
        }
        bsInputDriver = std::make_shared<BSInputDriver>(aVec, xVec, bsXVecReg, mulWidth);
        env.addAlwaysBlock(bsInputDriver);
        bsOutputMonitor = std::make_shared<BSOutputMonitor>(aVec, xVec, bsYVecReg[mulWidth-1], mulWidth);
        env.addAlwaysBlock(bsOutputMonitor);
    }
};

int main() {
    wolf_sim::Environment env(10);
    std::vector<int> aVec;
    std::vector<int> xVec;
    for(int i = 0; i < 1024; i++) {
        aVec.push_back(rand() % (1<<10));
        xVec.push_back(rand() % (1<<10));
    }
    BitSerialMultiplier bsm(aVec, xVec, env, 16);
    env.run();
    return 0;
}