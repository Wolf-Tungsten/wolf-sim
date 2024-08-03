#include <atomic>
#include <deque>
#include <random>

#include "wolf_sim.h"

const int MESH_SIZE = 2;        // 16*16 的网格
const int BUFFER_SIZE = 10;     // 缓冲区大小
const int PACKET_COUNT = 100;  // 每个PE发送的包数量

std::atomic<int> finalPacketCount = 0;

struct packet {
  int srcX;
  int srcY;
  int dstX;
  int dstY;
  int dataNo;
};

/* 左上角坐标为 0，0 */
class Mesh2DRouter : public wolf_sim::Module {
 public:
  IPort(northInPort, packet);
  OPort(northInReadyPort, bool);
  IPort(southInPort, packet);
  OPort(southInReadyPort, bool);
  IPort(eastInPort, packet);
  OPort(eastInReadyPort, bool);
  IPort(westInPort, packet);
  OPort(westInReadyPort, bool);
  IPort(localInPort, packet);
  OPort(localInReadyPort, bool);

  OPort(northOutPort, packet);
  IPort(northOutReadyPort, bool);
  OPort(southOutPort, packet);
  IPort(southOutReadyPort, bool);
  OPort(eastOutPort, packet);
  IPort(eastOutReadyPort, bool);
  OPort(westOutPort, packet);
  IPort(westOutReadyPort, bool);

  OPort(localOutPort, packet);
  IPort(localOutReadyPort, bool);

  bool hasNorth;
  bool hasSouth;
  bool hasEast;
  bool hasWest;

 private:
  int xCoord;
  int yCoord;
  int meshXSize;
  int meshYSize;

  int bufferSize;

  std::deque<packet> northOutBuffer;
  std::deque<packet> southOutBuffer;
  std::deque<packet> eastOutBuffer;
  std::deque<packet> westOutBuffer;
  std::deque<packet> localOutBuffer;

  bool bufferFull = false;

  void fire() {

    // 清空已经发送的包
    if (!northOutBuffer.empty() && northOutReadyPort.valid()) {
      northOutBuffer.pop_front();
    }
    if (!southOutBuffer.empty() && southOutReadyPort.valid()) {
      southOutBuffer.pop_front();
    }
    if (!eastOutBuffer.empty() && eastOutReadyPort.valid()) {
      eastOutBuffer.pop_front();
    }
    if (!westOutBuffer.empty() && westOutReadyPort.valid()) {
      westOutBuffer.pop_front();
    }
    if (!localOutBuffer.empty() && localOutReadyPort.valid()) {
      localOutBuffer.pop_front();
    }

    // 转载新的包，但注意判断条件不能被清空过程修改
    packet p;
    if (!bufferFull && hasNorth && (northInPort >> p)) {
      routeIntoBuffer(p);
    }
    if (!bufferFull && hasSouth && (southInPort >> p)) {
      routeIntoBuffer(p);
    }
    if (!bufferFull && hasEast && (eastInPort >> p)) {
      routeIntoBuffer(p);
    }
    if (!bufferFull && hasWest && (westInPort >> p)) {
      routeIntoBuffer(p);
    }
    if (!bufferFull && localInPort >> p) {
      routeIntoBuffer(p);
    }

    // 输出下一个包
    if (!northOutBuffer.empty()) {
      northOutPort << northOutBuffer.front();
    }
    if (!southOutBuffer.empty()) {
      southOutPort << southOutBuffer.front();
    }
    if (!eastOutBuffer.empty()) {
      eastOutPort << eastOutBuffer.front();
    }
    if (!westOutBuffer.empty()) {
      westOutPort << westOutBuffer.front();
    }
    if (!localOutBuffer.empty()) {
      localOutPort << localOutBuffer.front();
    }

    // 判断空满状态
    bool northOutBufferFull = (northOutBuffer.size() >= bufferSize);
    bool southOutBufferFull = (southOutBuffer.size() >= bufferSize);
    bool eastOutBufferFull = (eastOutBuffer.size() >= bufferSize);
    bool westOutBufferFull = (westOutBuffer.size() >= bufferSize);
    bool localOutBufferFull = (localOutBuffer.size() >= bufferSize);
    bufferFull = northOutBufferFull || southOutBufferFull ||
                 eastOutBufferFull || westOutBufferFull || localOutBufferFull;
    bool routerReady = !bufferFull;
    localInReadyPort << routerReady;
    if (hasNorth) {
      northInReadyPort << routerReady;
    }
    if (hasSouth) {
      southInReadyPort << routerReady;
    }
    if (hasEast) {
      eastInReadyPort << routerReady;
    }
    if (hasWest) {
      westInReadyPort << routerReady;
    }
    planWakeUp(1);
  }

  void routeIntoBuffer(const packet& p) {
    if (p.dataNo == PACKET_COUNT - 1) {
      std::cout << "Router " << xCoord << "_" << yCoord << " received packet "
                << p.dataNo << " from " << p.srcX << "_" << p.srcY << " to "
                << p.dstX << "_" << p.dstY << " at " << whatTime() << std::endl;
    }
    if (p.dstX == xCoord && p.dstY == yCoord) {
      localOutBuffer.push_back(p);
    } else if (p.dstX == xCoord && p.dstY < yCoord) {
      northOutBuffer.push_back(p);
    } else if (p.dstX == xCoord && p.dstY > yCoord) {
      southOutBuffer.push_back(p);
    } else if (p.dstX > xCoord) {
      eastOutBuffer.push_back(p);
    } else if (p.dstX < xCoord) {
      westOutBuffer.push_back(p);
    } else {
      // 无法到达的包，丢弃
      throw std::runtime_error("Unreachable packet");
    }
  }

 public:
  void setupRouter(int x, int y, int meshX, int meshY, int bufferSize) {
    xCoord = x;
    yCoord = y;
    meshXSize = meshX;
    meshYSize = meshY;
    this->bufferSize = bufferSize;
    // 左上角为 0，0
    hasNorth = yCoord > 0;
    hasSouth = yCoord < meshYSize - 1;
    hasEast = xCoord < meshXSize - 1;
    hasWest = xCoord > 0;
  }
};

// EmuPE：模拟器向其他 PE 发送 packet 并接收 packet
// 一共发送 PACKET_COUNT 个包
// 每个包的 dstX 和 dstY 随机生成合法地址
// 同时接收其他 PE 发送的包，当收到最后一个包的时候更新 finalPacketCount
// 每个包的 dataNo 为包的序号，最后一个包 dataNo == PACKET_COUNT - 1
// 当 finalPacketCount == PACKET_COUNT 时，结束模拟
// finalPacketCount 是一个原子变量，可以在多线程中安全访问
class EmuPE : public wolf_sim::Module {
 public:
  OPort(outPort, packet);
  IPort(outReadyPort, bool);

  IPort(inPort, packet);
  OPort(inReadyPort, bool);

  void setupPE(int x, int y) {
    xCoord = x;
    yCoord = y;
    nextPacket.srcX = xCoord;
    nextPacket.srcY = yCoord;
    gen.seed(rd());
    disX = std::uniform_int_distribution<int>(0, MESH_SIZE - 1);
    disY = std::uniform_int_distribution<int>(0, MESH_SIZE - 1);
    nextPacket.dataNo = 0;
  }

 private:
  packet nextPacket;
  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<int> disX;
  std::uniform_int_distribution<int> disY;
  int xCoord;
  int yCoord;

  void fire() {
    packet inP;
    if (inPort >> inP) {
      if (inP.dataNo == PACKET_COUNT - 1) {
        finalPacketCount++;
        if (finalPacketCount == MESH_SIZE * MESH_SIZE) {
          std::cout << "ahhahahahahahhahah" << std::endl;
          terminateSimulation();
        }
      }
    }

    

    if (outReadyPort.valid()) {
      if (nextPacket.dataNo == PACKET_COUNT - 1) {
        std::cout << "PE " << xCoord << "_" << yCoord << " send packet "
                  << nextPacket.dataNo << " to " << nextPacket.dstX << "_"
                  << nextPacket.dstY << " at " << whatTime() << std::endl;
      }
      ++nextPacket.dataNo;
      nextPacket.dstX = disX(gen);
      nextPacket.dstY = disY(gen);
    }

    outPort << nextPacket;
    inReadyPort << true;
    planWakeUp(1);
  }
};

class Mesh2DTop : public wolf_sim::Module {
  void construct() {
    std::vector<std::shared_ptr<Mesh2DRouter>> routerVec;
    // 创建 Router 和 PE，并建立 Router 和 PE 的连接
    for (int x = 0; x < MESH_SIZE; ++x) {
      for (int y = 0; y < MESH_SIZE; ++y) {
        auto router = createChildModule<Mesh2DRouter>(
            "router" + std::to_string(x) + "_" + std::to_string(y));
        router->setupRouter(x, y, MESH_SIZE, MESH_SIZE, BUFFER_SIZE);
        routerVec.push_back(router);

        auto pe = createChildModule<EmuPE>("pe" + std::to_string(x) + "_" +
                                           std::to_string(y));
        pe->setupPE(x, y);

        connect(pe->outPort, router->localInPort, "PE" + std::to_string(x) +
                                  "_" + std::to_string(y) + "_to_Router" +
                                  std::to_string(x) + "_" + std::to_string(y));
        connect(pe->outReadyPort, router->localInReadyPort, "PE" +
                                      std::to_string(x) + "_" + std::to_string(y) +
                                      "_to_Router" + std::to_string(x) + "_" +
                                      std::to_string(y) + "_ready");
        connect(router->localOutPort, pe->inPort, "Router" + std::to_string(x) +
                                           "_" + std::to_string(y) + "_to_PE" +
                                           std::to_string(x) + "_" +
                                           std::to_string(y));
        connect(router->localOutReadyPort, pe->inReadyPort, "Router" +
                                               std::to_string(x) + "_" +
                                               std::to_string(y) + "_to_PE" +
                                               std::to_string(x) + "_" +
                                               std::to_string(y) + "_ready");
      }
    }
    // 建立 Router 之间的连接
    for (int x = 0; x < MESH_SIZE; ++x) {
      for (int y = 0; y < MESH_SIZE; ++y) {
        auto router = routerVec[x * MESH_SIZE + y];
        if (router->hasNorth) {
          // 创建北向连接
          auto northRouter = routerVec[x * MESH_SIZE + y - 1];
          connect(router->northOutPort, northRouter->southInPort, "Router" +
                                              std::to_string(x) + "_" +
                                              std::to_string(y) + "_to_Router" +
                                              std::to_string(x) + "_" +
                                              std::to_string(y - 1));
          connect(router->northOutReadyPort, northRouter->southInReadyPort, "Router" +
                                                  std::to_string(x) + "_" +
                                                  std::to_string(y) + "_to_Router" +
                                                  std::to_string(x) + "_" +
                                                  std::to_string(y - 1) + "_ready");
        }
        if (router->hasSouth) {
          // 创建南向连接
          auto southRouter = routerVec[x * MESH_SIZE + y + 1];
          connect(router->southOutPort, southRouter->northInPort, "Router" +
                                              std::to_string(x) + "_" +
                                              std::to_string(y) + "_to_Router" +
                                              std::to_string(x) + "_" +
                                              std::to_string(y + 1));
          connect(router->southOutReadyPort, southRouter->northInReadyPort, "Router" +
                                                  std::to_string(x) + "_" +
                                                  std::to_string(y) + "_to_Router" +
                                                  std::to_string(x) + "_" +
                                                  std::to_string(y + 1) + "_ready");
        }
        if (router->hasWest) {
          // 创建西向连接
          auto westRouter = routerVec[(x - 1) * MESH_SIZE + y];
          connect(router->westOutPort, westRouter->eastInPort, "Router" +
                                          std::to_string(x) + "_" + std::to_string(y) +
                                          "_to_Router" + std::to_string(x - 1) + "_" +
                                          std::to_string(y));
          connect(router->westOutReadyPort, westRouter->eastInReadyPort, "Router" +
                                              std::to_string(x) + "_" + std::to_string(y) +
                                              "_to_Router" + std::to_string(x - 1) + "_" +
                                              std::to_string(y) + "_ready");
        }
        if (router->hasEast) {
          // 创建东向连接
          auto eastRouter = routerVec[(x + 1) * MESH_SIZE + y];
          connect(router->eastOutPort, eastRouter->westInPort, "Router" +
                                          std::to_string(x) + "_" + std::to_string(y) +
                                          "_to_Router" + std::to_string(x + 1) + "_" +
                                          std::to_string(y));
          connect(router->eastOutReadyPort, eastRouter->westInReadyPort, "Router" +
                                              std::to_string(x) + "_" + std::to_string(y) +
                                              "_to_Router" + std::to_string(x + 1) + "_" +
                                              std::to_string(y) + "_ready");
        }
      }
    }
  }
};

int main() {
  auto env = std::make_shared<wolf_sim::Environment>();
  auto top = std::make_shared<Mesh2DTop>();
  env->addTopModule(top);
  env->run();
  return 0;
}