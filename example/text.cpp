#include <fstream>
#include <cassert>
int main(){
    std::ifstream file("/home/shishunchen/hdd0/my_projects/wolf-sim/ramulator/configs/DDR4-config.cfg");
    assert(file.good() && "Bad config file");
    return 0;
}