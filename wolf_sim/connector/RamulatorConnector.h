#ifndef WOLF_SIM_RAMULATOR_CONNECTOR_H
#define WOLF_SIM_RAMULATOR_CONNECTOR_H
#include <cmath>
#include "Processor.h"
#include "Config.h"
#include "Controller.h"
#include "SpeedyController.h"
#include "Memory.h"
#include "DRAM.h"
#include "Statistics.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <functional>
#include <map>
#include <type_traits>
#include "Gem5Wrapper.h"
#include "DDR3.h"
#include "DDR4.h"
#include "DSARP.h"
#include "GDDR5.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "WideIO.h"
#include "WideIO2.h"
#include "HBM.h"
#include "SALP.h"
#include "ALDRAM.h"
#include "TLDRAM.h"
#include "STTMRAM.h"
#include "PCM.h"

// using namespace ramulator;
namespace wolf_sim{

class RamulatorConnector: public wolf_sim::AlwaysBlock {

    public:

        RamulatorConnector(std::string configname_,int timescale_)
        : configname(configname_),timescale(timescale_){
            //init the timestamp
            // std::cout<< configname_ <<std::endl;
            blockTimestamp = 0;
        };

        // template <typename T>
        wolf_sim::ReturnNothing always() override {
        //     //modified from main从一个文件获得configs
            ramulator::Config configs(configname);
            const std::string& standard = configs["standard"];
            assert(standard != "" || "DRAM standard should be specified.");

            void* ret = initMemory(standard,configs);

            if (standard == "DDR3") {
               ramulator::DDR3* spec = static_cast<ramulator::DDR3*>(ret);
              co_await after <ramulator::DDR3>(configs,spec);
            }
            else if (standard == "DDR4") {
              //auto spec = static_cast<ramulator::DDR4*> (ret);
              ramulator::DDR4* spec = static_cast<ramulator::DDR4*>(ret);
              co_await after <ramulator::DDR4>(configs,spec);
            } else if (standard == "SALP-MASA") {
		          //auto spec = static_cast<ramulator::SALP*> (ret);
              ramulator::SALP* spec = static_cast<ramulator::SALP*>(ret);
              co_await after <ramulator::SALP>(configs,spec);
            } else if (standard == "LPDDR3") {
              ramulator::LPDDR3* spec = static_cast<ramulator::LPDDR3*>(ret);
              co_await after <ramulator::LPDDR3>(configs,spec);
            } else if (standard == "LPDDR4") {
              ramulator::LPDDR4* spec = static_cast<ramulator::LPDDR4*>(ret);
              co_await after <ramulator::LPDDR4>(configs,spec);
            } else if (standard == "GDDR5") {
              ramulator::GDDR5* spec = static_cast<ramulator::GDDR5*>(ret);
              co_await after <ramulator::GDDR5>(configs,spec);
            } else if (standard == "HBM") {
              ramulator::HBM* spec = static_cast<ramulator::HBM*>(ret);
              co_await after <ramulator::HBM>(configs,spec);
            } else if (standard == "WideIO") {
              ramulator::WideIO* spec = static_cast<ramulator::WideIO*>(ret);
              co_await after <ramulator::WideIO>(configs,spec);
            } else if (standard == "WideIO2") {
              ramulator::WideIO2* spec = static_cast<ramulator::WideIO2*>(ret);
              co_await after <ramulator::WideIO2>(configs,spec);
            } else if (standard == "STTMRAM") {
              ramulator::STTMRAM* spec = static_cast<ramulator::STTMRAM*>(ret);
              co_await after <ramulator::STTMRAM>(configs,spec);
            } else if (standard == "PCM") {
              ramulator::PCM* spec = static_cast<ramulator::PCM*>(ret);
              co_await after <ramulator::PCM>(configs,spec);
            } else if (standard == "ALDRAM") {
              ramulator::ALDRAM* spec = static_cast<ramulator::ALDRAM*>(ret);
              co_await after <ramulator::ALDRAM>(configs,spec);
            } else if (standard == "TLDRAM") {
              ramulator::TLDRAM* spec = static_cast<ramulator::TLDRAM*>(ret);
              co_await after <ramulator::TLDRAM>(configs,spec);
            }

            
        }

        template<typename T>
        wolf_sim::ReturnNothing after(ramulator::Config &configs, T* spec){
            // modified from start_run
            // initiate controller and memory
            int memoryFreq = spec->speed_entry.freq; //in MHz
            float temp = 1000000.0 / (memoryFreq * timescale) + 0.5;
            memoryTimescale = int(temp);
            std::cout << "memory timestamp (use ceil): "<<memoryTimescale<<std::endl; 

            int C = configs.get_channels(), R = configs.get_ranks();
            // Check and Set channel, rank number
            spec->set_channel_number(C);
            spec->set_rank_number(R);
            std::vector<ramulator::Controller<T>*> ctrls;
            for (int c = 0 ; c < C ; c++) {
                ramulator::DRAM<T>* channel = new ramulator::DRAM<T>(spec, T::Level::Channel);
                channel->id = c;
                channel->regStats("");
                ramulator::Controller<T>* ctrl = new ramulator::Controller<T>(configs, channel);
                ctrls.push_back(ctrl);
            }
            ramulator::Memory<T, ramulator::Controller> memory(configs, ctrls);
            
            long getAddr = 0;
            ramulator::Request::Type getType = ramulator::Request::Type::READ;
            map<int, int> latencies; //kv对，k&v are both int
            auto read_complete = [&latencies](ramulator::Request& r){latencies[r.depart - r.arrive]++;}; 
            ramulator::Request req(getAddr, getType, read_complete);

            while (1) {
              //get req in reg               
                // auto inTrace = co_await reg_in.get();
                // addr = inTrace.first;
                // type = inTrace.second;
                getAddr = 0x7876af80;
              //通过tick推进时间，同时记录时间戳
                memory.send(req);
              //结束了要put回去(async_put)
                // auto payload = co_await reg_out.asyncPut();
                memory.tick();
                blockTimestamp ++;
                // auto payload = co_await reg.get();
                if(blockTimestamp=10) break;
            }
        }
        

        // template <typename T>
        // void memoryOp(ramulator::Memory<T, ramulator::Controller>& memory, std::pair<long, ramulator::Request::Type> instr){
        //     map<int, int> latencies; //kv对，k&v are both int
        //     auto read_complete = [&latencies](ramulator::Request& r){latencies[r.depart - r.arrive]++;};
        //     long addr = 0;
        //     ramulator::Request::Type type = ramulator::Request::Type::READ;
        //     ramulator::Request req(addr, type, read_complete);

        //     if (!end || memory.pending_requests()){ //如果没有停止
        //         if (!end){
        //             req.addr = instr.first;
        //             req.type = instr.second;
        //             stall = !memory.send(req); //送进去，看成不成功
        //         }
        //         else {
        //             memory.set_high_writeq_watermark(0.0f); // make sure that all write requests in the
        //                                                     // write queue are drained
        //         }
        //         memory.tick(); //代码里没有循环，就是一次++
        //         blockTimestamp ++;
        //     }
        // }


        ~RamulatorConnector() {///ramulator::Memory<T, ramulator::Controller>& memory
            // memory.finish();//finish the memory 析构函数不能有参数，那我怎么知道它要释放那些空间呢
            std::cout << "clk cycle: "<< blockTimestamp << std::endl;
            std::cout << "RamulatorConnector destructed" << std::endl;
        }
        
    
    private:// define private 

        // wolf_sim::RegRef<std::pair<long, ramulator::Request::Type>, 10> reg_in;

        /* vars needs by running simulation */
        std::string configname;
        int timescale; //以ps 为单位
        int memoryTimescale ;
        bool stall = false, end = false;
        // int delay;
        
        void* initMemory(const std::string& standard, ramulator::Config& configs){
            if (standard == "DDR3") {
              ramulator::DDR3* ddr3 = new ramulator::DDR3(configs["org"], configs["speed"]);
                return  ddr3 ;
            } else if (standard == "DDR4") {
              ramulator::DDR4* ddr4 = new ramulator::DDR4(configs["org"], configs["speed"]);
                return  ddr4 ;
            } else if (standard == "SALP-MASA") {
              ramulator::SALP* salp8 = new ramulator::SALP(configs["org"], configs["speed"], "SALP-MASA", configs.get_subarrays());
                return  salp8 ;
            } else if (standard == "LPDDR3") {
              ramulator::LPDDR3* lpddr3 = new ramulator::LPDDR3(configs["org"], configs["speed"]);
                return  lpddr3 ;
            } else if (standard == "LPDDR4") {
              // total cap: 2GB, 1/2 of others
              ramulator::LPDDR4* lpddr4 = new ramulator::LPDDR4(configs["org"], configs["speed"]);
                return  lpddr4 ;
            } else if (standard == "GDDR5") {
              ramulator::GDDR5* gddr5 = new ramulator::GDDR5(configs["org"], configs["speed"]);
                return  gddr5 ;
            } else if (standard == "HBM") {
              ramulator::HBM* hbm = new ramulator::HBM(configs["org"], configs["speed"]);
                return  hbm ;
            } else if (standard == "WideIO") {
              // total cap: 1GB, 1/4 of others
              ramulator::WideIO* wio = new ramulator::WideIO(configs["org"], configs["speed"]);
                return  wio ;
            } else if (standard == "WideIO2") {
              // total cap: 2GB, 1/2 of others
              ramulator::WideIO2* wio2 = new ramulator::WideIO2(configs["org"], configs["speed"], configs.get_channels());
              wio2->channel_width *= 2;
                return  wio2 ;
            } else if (standard == "STTMRAM") {
              ramulator::STTMRAM* sttmram = new ramulator::STTMRAM(configs["org"], configs["speed"]);
                return  sttmram ;
            } else if (standard == "PCM") {
              ramulator::PCM* pcm = new ramulator::PCM(configs["org"], configs["speed"]);
                return  pcm ;
            }
            // Various refresh mechanisms
             else if (standard == "ALDRAM") {
              ramulator::ALDRAM* aldram = new ramulator::ALDRAM(configs["org"], configs["speed"]);
                return  aldram ;
            } else if (standard == "TLDRAM") {
              ramulator::TLDRAM* tldram = new ramulator::TLDRAM(configs["org"], configs["speed"], configs.get_subarrays());
                return  tldram ;
            }
        }
};
}
#endif

//初始化的内容全部放在always（）里面，作为一个局部变量
//
