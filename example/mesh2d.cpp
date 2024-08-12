#include <atomic>
#include <deque>
#include <iostream>
#include <memory>
#include <random>

#include "wolf_sim.h"

std::atomic<int> lastPacketCounter;
std::map<std::string, bool> packetDuplication;

struct Packet {
  int payload;
  int dstX;
  int dstY;
  int srcX;
  int srcY;
  bool last;
};

class MeshRouter : public wolf_sim::Module {
 public:
  Input(northInValid, bool);
  Input(northInPayload, Packet);
  Output(northInReady, bool);
  Input(southInValid, bool);
  Input(southInPayload, Packet);
  Output(southInReady, bool);
  Input(eastInValid, bool);
  Input(eastInPayload, Packet);
  Output(eastInReady, bool);
  Input(westInValid, bool);
  Input(westInPayload, Packet);
  Output(westInReady, bool);
  Input(localInValid, bool);
  Input(localInPayload, Packet);
  Output(localInReady, bool);

  Output(northOutValid, bool);
  Output(northOutPayload, Packet);
  Input(northOutReady, bool);
  Output(southOutValid, bool);
  Output(southOutPayload, Packet);
  Input(southOutReady, bool);
  Output(eastOutValid, bool);
  Output(eastOutPayload, Packet);
  Input(eastOutReady, bool);
  Output(westOutValid, bool);
  Output(westOutPayload, Packet);
  Input(westOutReady, bool);
  Output(localOutValid, bool);
  Output(localOutPayload, Packet);
  Input(localOutReady, bool);

  int coordX, coordY;

  MeshRouter(int x, int y, int bufferSize)
      : coordX(x), coordY(y), bufferSize(bufferSize) {};

 private:
  int bufferSize;

  Reg(northOutBuffer, std::deque<Packet>);
  Reg(southOutBuffer, std::deque<Packet>);
  Reg(eastOutBuffer, std::deque<Packet>);
  Reg(westOutBuffer, std::deque<Packet>);
  Reg(localOutBuffer, std::deque<Packet>);

  void init() {
    northInReady = true;
    southInReady = true;
    eastInReady = true;
    westInReady = true;
    localInReady = true;
    northOutValid = false;
    southOutValid = false;
    eastOutValid = false;
    westOutValid = false;
    localOutValid = false;
  }

  void updateStateOutput() {
    if (northOutReady && northOutValid) {
      northOutBuffer.w().pop_front();
    }
    if (southOutReady && southOutValid) {
      southOutBuffer.w().pop_front();
    }
    if (eastOutReady && eastOutValid) {
      eastOutBuffer.w().pop_front();
    }
    if (westOutReady && westOutValid) {
      westOutBuffer.w().pop_front();
    }
    if (localOutReady && localOutValid) {
      localOutBuffer.w().pop_front();
    }

    if (northOutBuffer.r().size() > 0) {
      northOutValid = true;
      northOutPayload = northOutBuffer.r().front();
    } else {
      northOutValid = false;
    }

    if (southOutBuffer.r().size() > 0) {
      southOutValid = true;
      southOutPayload = southOutBuffer.r().front();
    } else {
      southOutValid = false;
    }

    if (eastOutBuffer.r().size() > 0) {
      eastOutValid = true;
      eastOutPayload = eastOutBuffer.r().front();
    } else {
      eastOutValid = false;
    }

    if (westOutBuffer.r().size() > 0) {
      westOutValid = true;
      westOutPayload = westOutBuffer.r().front();
    } else {
      westOutValid = false;
    }

    if (localOutBuffer.r().size() > 0) {
      localOutValid = true;
      localOutPayload = localOutBuffer.r().front();
    } else {
      localOutValid = false;
    }

    if (northInValid && northInReady) {
      routeIntoBuffer(northInPayload);
    }
    if (southInValid && southInReady) {
      routeIntoBuffer(southInPayload);
    }
    if (eastInValid && eastInReady) {
      routeIntoBuffer(eastInPayload);
    }
    if (westInValid && westInReady) {
      routeIntoBuffer(westInPayload);
    }
    if (localInValid && localInReady) {
      routeIntoBuffer(localInPayload);
    }

    updateReady();
  }

  void routeIntoBuffer(Packet p) {
    if (p.payload < 0) {
      logger() << "illegal packet" << std::endl;
    }
    if (p.dstX < coordX) {
      westOutBuffer.w().push_back(p);
    } else if (p.dstX > coordX) {
      eastOutBuffer.w().push_back(p);
    } else if (p.dstX == coordX) {
      if (p.dstY < coordY) {
        northOutBuffer.w().push_back(p);
      } else if (p.dstY > coordY) {
        southOutBuffer.w().push_back(p);
      } else {
        localOutBuffer.w().push_back(p);
      }
    }
  }

  void updateReady() {
    bool sba = southOutBuffer.r().size() < bufferSize - 5;
    bool nba = northOutBuffer.r().size() < bufferSize - 5;
    bool eba = eastOutBuffer.r().size() < bufferSize - 5;
    bool wba = westOutBuffer.r().size() < bufferSize - 5;
    bool lba = localOutBuffer.r().size() < bufferSize - 5;
    northInReady = sba && lba;
    southInReady = nba && lba;
    eastInReady = nba && sba && wba && lba;
    westInReady = nba && sba && eba && lba;
    localInReady = nba && sba && eba && wba;
  }
};

class PE : public wolf_sim::Module {
 public:
  Input(inValid, bool);
  Input(inPayload, Packet);
  Output(inReady, bool);

  Output(outValid, bool);
  Output(outPayload, Packet);
  Input(outReady, bool);

  PE(int x, int y, int xSize, int ySize, int totalPacket)
      : coordX(x),
        coordY(y),
        xSize(xSize),
        ySize(ySize),
        totalPacket(totalPacket) {
    gen = std::mt19937(rd());
    dstXDist = std::uniform_int_distribution<int>(0, xSize - 1);
    dstYDist = std::uniform_int_distribution<int>(0, ySize - 1);
  };

  // static std::atomic<int> lastPacketCounter;

 private:
  int coordX, coordY, xSize, ySize;
  int totalPacket;
  std::random_device rd;
  std::mt19937 gen;
  std::uniform_int_distribution<int> dstXDist;
  std::uniform_int_distribution<int> dstYDist;

  Reg(packetCounter, int);
  Reg(nextPacket, Packet);

  void genNextPacket() {
    nextPacket.w().srcX = coordX;
    nextPacket.w().srcY = coordY;
    nextPacket.w().last = packetCounter == totalPacket - 1;
    nextPacket.w().payload = packetCounter;
    do {
      nextPacket.w().dstX = dstXDist(gen);
      nextPacket.w().dstY = dstYDist(gen);
    } while (nextPacket.r().dstX == coordX && nextPacket.r().dstY == coordY);
  }
  void init() {
    inReady = true;
    packetCounter = 0;
    outValid = true;
    genNextPacket();
    outPayload = nextPacket;
  }

  void updateStateOutput() {
    if (outReady && outValid) {
      // logger() << "PE(" << coordX << ", " << coordY << ") sent packet "
      //          << outPayload.r().payload << " to "
      //          << "PE(" << outPayload.r().dstX << ", " << outPayload.r().dstY
      //          << ")" << std::endl;
      outValid = false;
      packetCounter.w()++;
    }

    if (packetCounter < totalPacket) {
      outValid = true;
      genNextPacket();
      outPayload = nextPacket;
    }

    if (inValid && inReady) {
      if (inPayload.r().last) {
        lastPacketCounter++;
        logger() << "PE(" << coordX << ", " << coordY
                 << ") received last packet from "
                 << "PE(" << inPayload.r().srcX << ", " << inPayload.r().srcY
                 << ")" << std::endl;
        if (lastPacketCounter == xSize * ySize) {
          logger() << "All packets received" << std::endl;
          terminate();
        }
      } else {
        // logger() << "PE(" << coordX << ", " << coordY << ") received packet "
        //          << "PE(" << inPayload.r().srcX << ", " << inPayload.r().srcY
        //          << ")~" << inPayload.r().payload
        //           << std::endl;
      }
    }
  }
};

class Mesh2D : public wolf_sim::Module {
 public:
  Mesh2D(int xSize, int ySize, int bufferSize, int totalPacket)
      : xSize(xSize),
        ySize(ySize),
        bufferSize(bufferSize),
        totalPacket(totalPacket) {};

 private:
  int xSize, ySize, bufferSize, totalPacket;
  std::vector<std::shared_ptr<MeshRouter>> routerVec;
  std::vector<std::shared_ptr<PE>> peVec;

  void construct() {
    routerVec.clear();
    peVec.clear();
    // 创建路由器和处理器
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < ySize; y++) {
        auto router = std::make_shared<MeshRouter>(x, y, bufferSize);
        router->setModuleLabel("Router(" + std::to_string(x) + ", " +
                               std::to_string(y) + ")");
        routerVec.push_back(router);
        addChildModule(router);
        auto pe = std::make_shared<PE>(x, y, xSize, ySize, totalPacket);
        pe->setModuleLabel("PE(" + std::to_string(x) + ", " +
                           std::to_string(y) + ")");
        peVec.push_back(pe);
        addChildModule(pe);
      }
    }
  }

  void updateChildInput() {
    for (int x = 0; x < xSize; x++) {
      for (int y = 0; y < ySize; y++) {
        auto router = routerVec[x * ySize + y];
        auto pe = peVec[x * ySize + y];
        // 建立 PE 和 Router 之间的连接
        router->localInValid = pe->outValid;
        router->localInPayload = pe->outPayload;
        pe->outReady = router->localInReady;
        pe->inValid = router->localOutValid;
        pe->inPayload = router->localOutPayload;
        router->localOutReady = pe->inReady;
        if (y > 0) {
          // 建立从 Router 到 north Router 的输出
          auto northRouter = routerVec[x * ySize + y - 1];
          northRouter->southInValid = router->northOutValid;
          northRouter->southInPayload = router->northOutPayload;
          router->northOutReady = northRouter->southInReady;
        }
        if (y < ySize - 1) {
          // 建立从 Router 到 south Router 的输出
          auto southRouter = routerVec[x * ySize + y + 1];
          southRouter->northInValid = router->southOutValid;
          southRouter->northInPayload = router->southOutPayload;
          router->southOutReady = southRouter->northInReady;
        }
        if (x > 0) {
          // 建立从 Router 到 west Router 的输出
          auto westRouter = routerVec[(x - 1) * ySize + y];
          westRouter->eastInValid = router->westOutValid;
          westRouter->eastInPayload = router->westOutPayload;
          router->westOutReady = westRouter->eastInReady;
        }
        if (x < xSize - 1) {
          // 建立从 Router 到 east Router 的输出
          auto eastRouter = routerVec[(x + 1) * ySize + y];
          eastRouter->westInValid = router->eastOutValid;
          eastRouter->westInPayload = router->eastOutPayload;
          router->eastOutReady = eastRouter->westInReady;
        }
      }
    }
  }
};

int main() {
  for (int test = 0; test < 1; test++) {
    packetDuplication.clear();
    lastPacketCounter = 0;
    Mesh2D mesh(8, 8, 10, 1000000);
    std::cout << "Test " << test << std::endl;
    mesh.tickToTermination();
  }
  return 0;
}