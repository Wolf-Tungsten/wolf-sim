#include <systemc>
#include <random>
#include <thread>
#include <chrono>
#include <string>

int generateRandomNumber(int min, int max) {
    std::random_device rd;  // 用于获取随机数的种子
    std::mt19937 gen(rd()); // 使用 Mersenne Twister 算法生成随机数
    std::uniform_int_distribution<> dis(min, max); // 定义随机数的范围

    return dis(gen);
}

class Station : public sc_core::sc_module {
public:
    sc_core::sc_in_clk clk;
    sc_core::sc_fifo_in<int> in_port;
    sc_core::sc_fifo_out<int> out_port;

    Station(sc_core::sc_module_name _name, int stationId_, int stationAmount_, int delay_, int transCount_ );
    

    void stationThread() {
        out_port.write(std::rand() % stationAmount);
        while(1) {
            wait(1, sc_core::SC_NS);
            int dst = in_port.read();
            if(dst != stationId) {
                // forward to next station
                out_port.write(dst);
                wait(1, sc_core::SC_NS);
            } 
                // process it and spwan a new request
                int nextDst = std::rand() % stationAmount;
                std::cout << "Station " << stationId << " request to " << nextDst << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            
        }
    }
private:
    int stationId;
    int delay;
    int transCount;
    int stationAmount;
};

SC_HAS_PROCESS(Station);
    Station::Station(sc_core::sc_module_name _name, int stationId_, int stationAmount_, int delay_, int transCount_ ) : 
    sc_core::sc_module(_name), 
    stationId(stationId_), 
    delay(delay_), 
    transCount(transCount_), 
    stationAmount(stationAmount_) {
        SC_THREAD(stationThread);
        sensitive << clk.pos();
    }

int sc_main(int argc, char *argv[]) {

    int stationAmount = strtol(argv[1], NULL, 10);
    int transCount = strtol(argv[2], NULL, 10);
    int delay = strtol(argv[3], NULL, 10);
    
    sc_core::sc_clock clk("clk", 1, sc_core::SC_NS);

    std::vector<std::unique_ptr<sc_core::sc_fifo<int>>> fifoVec;
    std::vector<std::unique_ptr<Station>> stationVec;

    for(int i = 0; i < stationAmount; i++) {
        fifoVec.emplace_back(std::make_unique<sc_core::sc_fifo<int>>(1));
        stationVec.emplace_back(std::make_unique<Station>((std::string("station")+std::to_string(i)).c_str(), i, stationAmount, delay, transCount));
    }
    for(int i = 0; i < stationAmount; i++) {
        stationVec[i]->clk(clk);
        stationVec[i]->in_port(*fifoVec[i]);
        stationVec[i]->out_port(*fifoVec[(i+1)%stationAmount]);
    }
    sc_core::sc_start(10, sc_core::SC_NS);
    return 0;
}