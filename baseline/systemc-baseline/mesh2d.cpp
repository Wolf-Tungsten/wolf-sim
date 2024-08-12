#include <atomic>
#include <deque>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>

#include "/Users/wolf/Develop/systemc-3.0.0/build/include/systemc.h"

// 定义 Packet 结构体
struct Packet {
  int payload;
  int dstX;
  int dstY;
  int srcX;
  int srcY;
  bool last;

  // 默认构造函数
  Packet() : payload(0), dstX(0), dstY(0), srcX(0), srcY(0), last(false) {}

  // 拷贝构造函数
  Packet(const Packet& other)
      : payload(other.payload),
        dstX(other.dstX),
        dstY(other.dstY),
        srcX(other.srcX),
        srcY(other.srcY),
        last(other.last) {}

  // 重载赋值运算符
  Packet& operator=(const Packet& other) {
    if (this != &other) {
      payload = other.payload;
      dstX = other.dstX;
      dstY = other.dstY;
      srcX = other.srcX;
      srcY = other.srcY;
      last = other.last;
    }
    return *this;
  }

  // 重载输出运算符，用于信号的写操作
  friend std::ostream& operator<<(std::ostream& os, const Packet& p) {
    os << "Packet(payload=" << p.payload << ", dstX=" << p.dstX
       << ", dstY=" << p.dstY << ", srcX=" << p.srcX << ", srcY=" << p.srcY
       << ", last=" << p.last << ")";
    return os;
  }

  // 重载输入运算符，用于信号的读操作
  friend std::istream& operator>>(std::istream& is, Packet& p) {
    is >> p.payload >> p.dstX >> p.dstY >> p.srcX >> p.srcY >> p.last;
    return is;
  }

  // 重载相等运算符
  bool operator==(const Packet& other) const {
    return payload == other.payload && dstX == other.dstX &&
           dstY == other.dstY && srcX == other.srcX && srcY == other.srcY &&
           last == other.last;
  }

  // 友元声明，用于波形跟踪
  friend void sc_trace(sc_trace_file* tf, const Packet& p,
                       const std::string& name);
};

// 实现 sc_trace 函数
void sc_trace(sc_trace_file* tf, const Packet& p, const std::string& name) {
  sc_trace(tf, p.payload, name + ".payload");
  sc_trace(tf, p.dstX, name + ".dstX");
  sc_trace(tf, p.dstY, name + ".dstY");
  sc_trace(tf, p.srcX, name + ".srcX");
  sc_trace(tf, p.srcY, name + ".srcY");
  sc_trace(tf, p.last, name + ".last");
}

SC_MODULE(MeshRouter) {
  sc_in<bool> clk;
  sc_in<bool> reset;

  sc_in<bool> northInValid;
  sc_in<Packet> northInPayload;
  sc_out<bool> northInReady;

  sc_in<bool> southInValid;
  sc_in<Packet> southInPayload;
  sc_out<bool> southInReady;

  sc_in<bool> eastInValid;
  sc_in<Packet> eastInPayload;
  sc_out<bool> eastInReady;

  sc_in<bool> westInValid;
  sc_in<Packet> westInPayload;
  sc_out<bool> westInReady;

  sc_in<bool> localInValid;
  sc_in<Packet> localInPayload;
  sc_out<bool> localInReady;

  // 输出端口
  sc_out<bool> northOutValid;
  sc_out<Packet> northOutPayload;
  sc_in<bool> northOutReady;

  sc_out<bool> southOutValid;
  sc_out<Packet> southOutPayload;
  sc_in<bool> southOutReady;

  sc_out<bool> eastOutValid;
  sc_out<Packet> eastOutPayload;
  sc_in<bool> eastOutReady;

  sc_out<bool> westOutValid;
  sc_out<Packet> westOutPayload;
  sc_in<bool> westOutReady;

  sc_out<bool> localOutValid;
  sc_out<Packet> localOutPayload;
  sc_in<bool> localOutReady;

  void route() {
    if (reset.read() == true) {
      init();
      return;
    }

    if (northOutReady.read() && northOutValid.read()) {
      northOutBuffer.pop_front();
    }
    if (southOutReady.read() && southOutValid.read()) {
      southOutBuffer.pop_front();
    }
    if (eastOutReady.read() && eastOutValid.read()) {
      eastOutBuffer.pop_front();
    }
    if (westOutReady.read() && westOutValid.read()) {
      westOutBuffer.pop_front();
    }
    if (localOutReady.read() && localOutValid.read()) {
      localOutBuffer.pop_front();
    }

    if (coordY > 0 && !northOutBuffer.empty()) {
      northOutValid.write(true);
      northOutPayload.write(northOutBuffer.front());
    } else {
      northOutValid.write(false);
    }

    if (coordY < ySize - 1 && !southOutBuffer.empty()) {
      southOutValid.write(true);
      southOutPayload.write(southOutBuffer.front());
    } else {
      southOutValid.write(false);
    }

    if (coordX < xSize - 1 && !eastOutBuffer.empty()) {
      eastOutValid.write(true);
      eastOutPayload.write(eastOutBuffer.front());
    } else {
      eastOutValid.write(false);
    }

    if (coordX > 0 && !westOutBuffer.empty()) {
      westOutValid.write(true);
      westOutPayload.write(westOutBuffer.front());
    } else {
      westOutValid.write(false);
    }

    if (!localOutBuffer.empty()) {
      localOutValid.write(true);
      localOutPayload.write(localOutBuffer.front());
    } else {
      localOutValid.write(false);
    }

    if (northInValid.read() && northInReady.read()) {
      routeIntoBuffer(northInPayload.read());
    }
    if (southInValid.read() && southInReady.read()) {
      routeIntoBuffer(southInPayload.read());
    }
    if (eastInValid.read() && eastInReady.read()) {
      routeIntoBuffer(eastInPayload.read());
    }
    if (westInValid.read() && westInReady.read()) {
      routeIntoBuffer(westInPayload.read());
    }
    if (localInValid.read() && localInReady.read()) {
      routeIntoBuffer(localInPayload.read());
    }
    updateReady();
  };

  void routeIntoBuffer(Packet p) {
    if (p.dstX < coordX) {
      westOutBuffer.push_back(p);
    } else if (p.dstX > coordX) {
      eastOutBuffer.push_back(p);
    } else if (p.dstX == coordX) {
      if (p.dstY < coordY) {
        northOutBuffer.push_back(p);
      } else if (p.dstY > coordY) {
        southOutBuffer.push_back(p);
      } else {
        localOutBuffer.push_back(p);
      }
    }
  }

  void updateReady() {
    bool sba = southOutBuffer.size() < bufferSize - 5;
    bool nba = northOutBuffer.size() < bufferSize - 5;
    bool eba = eastOutBuffer.size() < bufferSize - 5;
    bool wba = westOutBuffer.size() < bufferSize - 5;
    bool lba = localOutBuffer.size() < bufferSize - 5;
    northInReady.write((coordY > 0) && sba && lba);
    southInReady.write((coordY < ySize - 1) && nba && lba);
    eastInReady.write((coordX < xSize - 1) && nba && sba && wba && lba);
    westInReady.write((coordX > 0) && nba && sba && eba && lba);
    localInReady.write(nba && sba && eba && wba);
  }

  // 初始化函数实现
  void init() {
    northInReady.write(true);
    southInReady.write(true);
    eastInReady.write(true);
    westInReady.write(true);
    localInReady.write(true);
    northOutValid.write(false);
    southOutValid.write(false);
    eastOutValid.write(false);
    westOutValid.write(false);
    localOutValid.write(false);
  }

  MeshRouter(sc_module_name name, int x, int y, int xSize, int ySize,
             int buffer_size)
      : sc_module(name),
        coordX(x),
        coordY(y),
        xSize(xSize),
        ySize(ySize),
        bufferSize(buffer_size) {
    SC_METHOD(route);
    sensitive << clk.pos();
  }

 private:
  int coordX;
  int coordY;
  int xSize;
  int ySize;
  int bufferSize;
  std::deque<Packet> northOutBuffer;
  std::deque<Packet> southOutBuffer;
  std::deque<Packet> eastOutBuffer;
  std::deque<Packet> westOutBuffer;
  std::deque<Packet> localOutBuffer;
};

SC_MODULE(PE) {
  sc_in<bool> clk;
  sc_in<bool> reset;

  // 输入端口
  sc_in<bool> inValid;
  sc_in<Packet> inPayload;
  sc_out<bool> inReady;

  // 输出端口
  sc_out<bool> outValid;
  sc_out<Packet> outPayload;
  sc_in<bool> outReady;

  // 构造函数
  PE(sc_module_name name, int x, int y, int xSize, int ySize, int totalPacket)
      : sc_module(name),
        coordX(x),
        coordY(y),
        xSize(xSize),
        ySize(ySize),
        totalPacket(totalPacket) {
    gen = std::mt19937(rd());
    dstXDist = std::uniform_int_distribution<int>(0, xSize - 1);
    dstYDist = std::uniform_int_distribution<int>(0, ySize - 1);

    SC_METHOD(updateStateOutput);
    sensitive << clk.pos();
  }

  // 方法声明
  void genNextPacket();
  void init();
  void updateStateOutput();

 private:
  int coordX, coordY, xSize, ySize;
  int totalPacket;
  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<int> dstXDist;
  std::uniform_int_distribution<int> dstYDist;

  int packetCounter;
  Packet nextPacket;
  std::unordered_map<std::string, bool> packetDuplication;

  static std::atomic<int> lastPacketCounter;
};

std::atomic<int> PE::lastPacketCounter(0);

void PE::genNextPacket() {
  nextPacket.srcX = coordX;
  nextPacket.srcY = coordY;
  nextPacket.last = packetCounter == totalPacket - 1;
  nextPacket.payload = packetCounter;
  do {
    nextPacket.dstX = dstXDist(gen);
    nextPacket.dstY = dstYDist(gen);
  } while (nextPacket.dstX == coordX && nextPacket.dstY == coordY);
}

void PE::init() {
  inReady.write(true);
  packetCounter = 0;
  outValid.write(true);
  genNextPacket();
  outPayload.write(nextPacket);
}

void PE::updateStateOutput() {
  if (reset.read() == true) {
    init();
    return;
  }
  if (outReady.read() && outValid.read()) {
    outValid.write(false);
    packetCounter++;
  }

  if (packetCounter < totalPacket) {
    outValid.write(true);
    genNextPacket();
    outPayload.write(nextPacket);
  }

  if (inValid.read() && inReady.read()) {
    if (inPayload.read().last) {
      lastPacketCounter++;
      std::cout << "PE(" << coordX << ", " << coordY
                << ") received last packet from "
                << "PE(" << inPayload.read().srcX << ", "
                << inPayload.read().srcY << ")" << std::endl;
      if (lastPacketCounter == xSize * ySize) {
        std::cout << "All packets received" << std::endl;
        sc_stop();
      }
    }
  }
}

SC_MODULE(Mesh2D) {
  // 输入端口
  sc_in<bool> clk;
  sc_in<bool> reset;

  std::vector<std::shared_ptr<PE>> peVec;
  std::vector<std::shared_ptr<MeshRouter>> routerVec;
  std::vector<std::shared_ptr<sc_signal<bool>>> validSignals;
  std::vector<std::shared_ptr<sc_signal<bool>>> readySignals;
  std::vector<std::shared_ptr<sc_signal<Packet>>> payloadSignals;

  // 构造函数
  Mesh2D(sc_module_name name, int xSize, int ySize, int bufferSize, int totalPacket)
      : sc_module(name),
        xSize(xSize),
        ySize(ySize),
        totalPacket(totalPacket),
        bufferSize(bufferSize) {
    routerVec.clear();
    peVec.clear();
    // 创建 Router 和 PE 并建立连接
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < ySize; y++) {
        auto router = std::make_shared<MeshRouter>("router", x, y, xSize, ySize,
                                                   bufferSize);
        routerVec.push_back(router);
        auto pe = std::make_shared<PE>("pe", x, y, xSize, ySize, totalPacket);
        peVec.push_back(pe);
        router->clk(clk);
        router->reset(reset);
        pe->clk(clk);
        pe->reset(reset);
        std::shared_ptr<sc_signal<bool>> routerLocalInValid =
            std::make_shared<sc_signal<bool>>();
        std::shared_ptr<sc_signal<bool>> routerLocalInReady =
            std::make_shared<sc_signal<bool>>();
        std::shared_ptr<sc_signal<bool>> routerLocalOutValid =
            std::make_shared<sc_signal<bool>>();
        std::shared_ptr<sc_signal<bool>> routerLocalOutReady =
            std::make_shared<sc_signal<bool>>();
        std::shared_ptr<sc_signal<Packet>> routerLocalInPayload =
            std::make_shared<sc_signal<Packet>>();
        std::shared_ptr<sc_signal<Packet>> routerLocalOutPayload =
            std::make_shared<sc_signal<Packet>>();
        validSignals.push_back(routerLocalInValid);
        validSignals.push_back(routerLocalOutValid);
        readySignals.push_back(routerLocalInReady);
        readySignals.push_back(routerLocalOutReady);
        payloadSignals.push_back(routerLocalInPayload);
        payloadSignals.push_back(routerLocalOutPayload);
        router->localInValid(*routerLocalInValid);
        router->localInReady(*routerLocalInReady);
        router->localInPayload(*routerLocalInPayload);
        router->localOutValid(*routerLocalOutValid);
        router->localOutReady(*routerLocalOutReady);
        router->localOutPayload(*routerLocalOutPayload);
        pe->inValid(*routerLocalOutValid);
        pe->inReady(*routerLocalOutReady);
        pe->inPayload(*routerLocalOutPayload);
        pe->outValid(*routerLocalInValid);
        pe->outReady(*routerLocalInReady);
        pe->outPayload(*routerLocalInPayload);
      }
    }
    // 创建 router 之间的连接
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < ySize; y++) {
        auto router = routerVec[x * ySize + y];
        if (y > 0) {
          std::shared_ptr<sc_signal<bool>> valid =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<bool>> ready =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<Packet>> payload =
              std::make_shared<sc_signal<Packet>>();
          validSignals.push_back(valid);
          readySignals.push_back(ready);
          payloadSignals.push_back(payload);
          // 建立从 Router 到 north Router 的输出
          auto northRouter = routerVec[x * ySize + y - 1];
          northRouter->southInValid(*valid);
          router->northOutValid(*valid);
          northRouter->southInPayload(*payload);
          router->northOutPayload(*payload);
          router->northOutReady(*ready);
          northRouter->southInReady(*ready);
        } else {
          std::shared_ptr<sc_signal<bool>> valid =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<bool>> ready =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<Packet>> payload =
              std::make_shared<sc_signal<Packet>>();
          validSignals.push_back(valid);
          readySignals.push_back(ready);
          payloadSignals.push_back(payload);
          router->northOutReady(*ready);
          router->northOutPayload(*payload);
          router->northOutValid(*valid);
          router->northInValid(*valid);
          router->northInPayload(*payload);
          router->northInReady(*ready);
        }
        if (y < ySize - 1) {
          std::shared_ptr<sc_signal<bool>> valid =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<bool>> ready =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<Packet>> payload =
              std::make_shared<sc_signal<Packet>>();
          validSignals.push_back(valid);
          readySignals.push_back(ready);
          payloadSignals.push_back(payload);
          // 建立从 Router 到 south Router 的输出
          auto southRouter = routerVec[x * ySize + y + 1];
          southRouter->northInValid(*valid);
          router->southOutValid(*valid);
          southRouter->northInPayload(*payload);
          router->southOutPayload(*payload);
          router->southOutReady(*ready);
          southRouter->northInReady(*ready);
        } else {
          std::shared_ptr<sc_signal<bool>> valid =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<bool>> ready =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<Packet>> payload =
              std::make_shared<sc_signal<Packet>>();
          validSignals.push_back(valid);
          readySignals.push_back(ready);
          payloadSignals.push_back(payload);
          router->southOutValid(*valid);
          router->southOutPayload(*payload);
          router->southOutReady(*ready);
          router->southInReady(*ready);
          router->southInPayload(*payload);
          router->southInValid(*valid);
        }
        if (x > 0) {
          std::shared_ptr<sc_signal<bool>> valid =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<bool>> ready =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<Packet>> payload =
              std::make_shared<sc_signal<Packet>>();
          validSignals.push_back(valid);
          readySignals.push_back(ready);
          payloadSignals.push_back(payload);
          // 建立从 Router 到 west Router 的输出
          auto westRouter = routerVec[(x - 1) * ySize + y];
          westRouter->eastInValid(*valid);
          router->westOutValid(*valid);
          westRouter->eastInPayload(*payload);
          router->westOutPayload(*payload);
          router->westOutReady(*ready);
          westRouter->eastInReady(*ready);
        } else {
          std::shared_ptr<sc_signal<bool>> valid =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<bool>> ready =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<Packet>> payload =
              std::make_shared<sc_signal<Packet>>();
          validSignals.push_back(valid);
          readySignals.push_back(ready);
          payloadSignals.push_back(payload);
          router->westOutValid(*valid);
          router->westOutReady(*ready);
          router->westOutPayload(*payload);
          router->westInValid(*valid);
          router->westInReady(*ready);
          router->westInPayload(*payload);
        }
        if (x < xSize - 1) {
          std::shared_ptr<sc_signal<bool>> valid =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<bool>> ready =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<Packet>> payload =
              std::make_shared<sc_signal<Packet>>();
          validSignals.push_back(valid);
          readySignals.push_back(ready);
          payloadSignals.push_back(payload);
          // 建立从 Router 到 east Router 的输出
          auto eastRouter = routerVec[(x + 1) * ySize + y];
          eastRouter->westInValid(*valid);
          router->eastOutValid(*valid);
          eastRouter->westInPayload(*payload);
          router->eastOutPayload(*payload);
          router->eastOutReady(*ready);
          eastRouter->westInReady(*ready);
        } else {
          std::shared_ptr<sc_signal<bool>> valid =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<bool>> ready =
              std::make_shared<sc_signal<bool>>();
          std::shared_ptr<sc_signal<Packet>> payload =
              std::make_shared<sc_signal<Packet>>();
          validSignals.push_back(valid);
          readySignals.push_back(ready);
          payloadSignals.push_back(payload);
          router->eastOutValid(*valid);
          router->eastOutReady(*ready);
          router->eastOutPayload(*payload);
          router->eastInValid(*valid);
          router->eastInReady(*ready);
          router->eastInPayload(*payload);
        }
      }
    }
  }

 private:
  int xSize, ySize, totalPacket, bufferSize;
};
// 主函数
int sc_main(int argc, char* argv[]) {
  sc_signal<bool> clk;    // 时钟信号
  sc_signal<bool> reset;  // 输出信号

  Mesh2D mesh("mesh", 8, 8, 10, 1000000);  // 创建 Mesh2D 网络
  mesh.clk(clk);                        // 连接时钟信号
  mesh.reset(reset);                    // 连接输出信号

  // 生成时钟信号
  clk.write(true);
  reset.write(true);
  int reset_cycles = 10;
  sc_start(0, SC_NS);  // 初始化仿真时间
  while (!sc_end_of_simulation_invoked()) {
    if (reset_cycles > 0) {
      reset_cycles--;
    } else if (reset_cycles == 0) {
      reset.write(false);
      reset_cycles--;
    }
    clk.write(false);
    sc_start(0.5, SC_NS);  // 等待 10 纳秒
    clk.write(true);
    sc_start(0.5, SC_NS);  // 等待 10 纳秒
  }

  return 0;
}