#include <deque>

#include "wolf_sim/wolf_sim.h"
struct packet {
  int srcX;
  int srcY;
  int dstX;
  int dstY;
};

/* 左上角坐标为 0，0 */
class Mesh2DRouter : public wolf_sim::Module {
 public:
  IPort(northInPort, packet);
  OPort(northInAckPort, bool);
  IPort(southInPort, packet);
  OPort(southInAckPort, bool);
  IPort(eastInPort, packet);
  OPort(eastInAckPort, bool);
  IPort(westInPort, packet);
  OPort(westInAckPort, bool);
  IPort(localInPort, packet);
  OPort(localInAckPort, bool);

  OPort(northOutPort, packet);
  IPort(northOutAckPort, bool);
  OPort(southOutPort, packet);
  IPort(southOutAckPort, bool);
  OPort(eastOutPort, packet);
  IPort(eastOutAckPort, bool);
  OPort(westOutPort, packet);
  IPort(westOutAckPort, bool);

  OPort(localOutPort, packet);
  IPort(localOutAckPort, bool);

  int xCoord;
  int yCoord;
  int meshXSize;
  int meshYSize;

  int bufferSize;

 private:
  std::deque<packet> northOutBuffer;
  std::deque<packet> southOutBuffer;
  std::deque<packet> eastOutBuffer;
  std::deque<packet> westOutBuffer;
  std::deque<packet> localOutBuffer;

  int packetDropCnt = 0;

  void fire() {
    bool northOccupied = false;
    bool southOccupied = false;
    bool eastOccupied = false;
    bool westOccupied = false;
    bool localOccupied = false;

    if (!northOutBuffer.empty()) {
      northOccupied = true;
      northOutPort << northOutBuffer.front();
      northOutBuffer.pop_front();
    }

    if (!southOutBuffer.empty()) {
      southOccupied = true;
      southOutPort << southOutBuffer.front();
      southOutBuffer.pop_front();
    }

    if (!eastOutBuffer.empty()) {
      eastOccupied = true;
      eastOutPort << eastOutBuffer.front();
      eastOutBuffer.pop_front();
    }

    if (!westOutBuffer.empty()) {
      westOccupied = true;
      westOutPort << westOutBuffer.front();
      westOutBuffer.pop_front();
    }

    if (!localOutBuffer.empty()) {
      localOccupied = true;
      localOutPort << localOutBuffer.front();
      localOutBuffer.pop_front();
    }

    packet payload;
    if (xCoord > 0 && westInPort >> payload) {
      // 西侧有接口
      route(payload, northOccupied, southOccupied, eastOccupied, westOccupied,
            localOccupied);
    }
    if (yCoord > 0 && northInPort >> payload) {
      // 北侧有接口
      route(payload, northOccupied, southOccupied, eastOccupied, westOccupied,
            localOccupied);
    }
    if (xCoord < meshXSize - 1 && eastInPort >> payload) {
      // 东侧有接口
      route(payload, northOccupied, southOccupied, eastOccupied, westOccupied,
            localOccupied);
    }
    if (yCoord < meshYSize - 1 && southInPort >> payload) {
      // 南侧有接口
      route(payload, northOccupied, southOccupied, eastOccupied, westOccupied,
            localOccupied);
    }
    if (localInPort >> payload) {
      // 本地一定有接口
      route(payload, northOccupied, southOccupied, eastOccupied, westOccupied,
            localOccupied);
    }

    if (!northOutBuffer.empty() || !southOutBuffer.empty() ||
        !eastOutBuffer.empty() || !westOutBuffer.empty() ||
        !localOutBuffer.empty()) {
      // 如果缓冲区有数据，则下一拍要唤醒
      planWakeUp(1);
    }
  }

  void route(const packet& p, bool& northOccupied, bool& southOccupied,
             bool& eastOccupied, bool& westOccupied, bool& localOccupied) {
    // 首先判断包的去向
    // 如果包的去向已经被占用，那么检查对应缓冲区是否有空间
    // 如果有空间则放入缓冲区，否则丢弃，计为丢包
    // 如果包的去向未被占用，直接发送

    if (p.dstX == xCoord && p.dstY == yCoord) {
      if (localOccupied) {
        if (localOutBuffer.size() < bufferSize) {
          localOutBuffer.push_back(p);
        } else {
          packetDropCnt++;
        }
      } else {
        localOccupied = true;
        localOutPort << p;
      }
    } else if (p.dstX == xCoord && p.dstY < yCoord) {
      if (northOccupied) {
        if (northOutBuffer.size() < bufferSize) {
          northOutBuffer.push_back(p);
        } else {
          packetDropCnt++;
        }
      } else {
        northOccupied = true;
        northOutPort << p;
      }
    } else if (p.dstX == xCoord && p.dstY > yCoord) {
      if (southOccupied) {
        if (southOutBuffer.size() < bufferSize) {
          southOutBuffer.push_back(p);
        } else {
          packetDropCnt++;
        }
      } else {
        southOccupied = true;
        southOutPort << p;
      }
    } else if (p.dstX > xCoord) {
      if (eastOccupied) {
        if (eastOutBuffer.size() < bufferSize) {
          eastOutBuffer.push_back(p);
        } else {
          packetDropCnt++;
        }
      } else {
        eastOccupied = true;
        eastOutPort << p;
      }
    } else if (p.dstX < xCoord) {
      if (westOccupied) {
        if (westOutBuffer.size() < bufferSize) {
          westOutBuffer.push_back(p);
        } else {
          packetDropCnt++;
        }
      } else {
        westOccupied = true;
        westOutPort << p;
      }
    } else {
      // 无法到达的包，丢弃
      throw std::runtime_error("Unreachable packet");
    }
  }
};