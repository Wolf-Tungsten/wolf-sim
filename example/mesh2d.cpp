#include <atomic>
#include <deque>
#include <random>

#include "wolf_sim.h"

const int MESH_SIZE = 16;       // 16*16 的网格
const int BUFFER_SIZE = 10;     // 缓冲区大小
const int PACKET_COUNT = 1000;  // 每个PE发送的包数量

std::atomic<int> finalPacketCount = 0;

struct packet {
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

 private:
  int xCoord;
  int yCoord;
  int meshXSize;
  int meshYSize;
  bool hasNorth;
  bool hasSouth;
  bool hasEast;
  bool hasWest;

  int bufferSize;

  std::deque<packet> northOutBuffer;
  std::deque<packet> southOutBuffer;
  std::deque<packet> eastOutBuffer;
  std::deque<packet> westOutBuffer;
  std::deque<packet> localOutBuffer;

  int packetDropCnt = 0;

  void fire() {
    packet p;
    if (hasNorth && northInPort >> p) {
      routeIntoBuffer(p);
    }
    if (hasSouth && southInPort >> p) {
      routeIntoBuffer(p);
    }
    if (hasEast && eastInPort >> p) {
      routeIntoBuffer(p);
    }
    if (hasWest && westInPort >> p) {
      routeIntoBuffer(p);
    }
    if (localInPort >> p) {
      routeIntoBuffer(p);
    }

    if (!northOutBuffer.empty() && northOutReadyPort.valid()) {
      northOutPort << northOutBuffer.front();
      northOutBuffer.pop_front();
    }

    if (!southOutBuffer.empty() && southOutReadyPort.valid()) {
      southOutPort << southOutBuffer.front();
      southOutBuffer.pop_front();
    }

    if (!eastOutBuffer.empty() && eastOutReadyPort.valid()) {
      eastOutPort << eastOutBuffer.front();
      eastOutBuffer.pop_front();
    }

    if (!westOutBuffer.empty() && westOutReadyPort.valid()) {
      westOutPort << westOutBuffer.front();
      westOutBuffer.pop_front();
    }

    if (!localOutBuffer.empty() && localOutReadyPort.valid()) {
      localOutPort << localOutBuffer.front();
      localOutBuffer.pop_front();
    }

    bool northOutBufferFull = northOutBuffer.size() >= bufferSize;
    bool southOutBufferFull = southOutBuffer.size() >= bufferSize;
    bool eastOutBufferFull = eastOutBuffer.size() >= bufferSize;
    bool westOutBufferFull = westOutBuffer.size() >= bufferSize;
    bool localOutBufferFull = localOutBuffer.size() >= bufferSize;
    bool routerReady =
        !(northOutBufferFull || southOutBufferFull || eastOutBufferFull ||
          westOutBufferFull || localOutBufferFull);
    localInReadyPort << routerReady;
    northInReadyPort << routerReady;
    southInReadyPort << routerReady;
    eastInReadyPort << routerReady;
    westInReadyPort << routerReady;
    planWakeUp(1);
  }

  void routeIntoBuffer(const packet& p) {
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

  void setupPE() {
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

  void fire() {
    packet inP;
    if (inPort >> inP) {
      if (inP.dataNo == PACKET_COUNT - 1) {
        finalPacketCount++;
        if (finalPacketCount == PACKET_COUNT * MESH_SIZE * MESH_SIZE) {
          terminateSimulation();
        }
      }
    }

    outPort << nextPacket;

    if (whatTime() == 0 || outReadyPort.valid()) {
      ++nextPacket.dataNo;
      nextPacket.dstX = disX(gen);
      nextPacket.dstY = disY(gen);
    }

    inReadyPort << true;
    planWakeUp(1);
  }
};