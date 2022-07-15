#pragma once
#include "headers.hpp"
#define LOCALHOST "172.0.0.1"
#define BOOTSTRAP_IP LOCALHOST
#define BOOTSTRAP_PORT 9000
#define RING_SIZE 65535

using std::string;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using boost::shared_ptr;

struct Address {
  boost::asio::ip::address_v4 ip;
  unsigned short port;

  bool operator==(const Address &rhs) const {
    return ip == rhs.ip && port == rhs.port;
  }
};

struct MemberInfo {
  Address address;
  int id;
  int ring_id;
  int heartbeat;
  bool isAlive;

  MemberInfo(Address &addr, int id, int ring_id)
  : address(addr), id(id), ring_id(ring_id), heartbeat(0), isAlive(false) {
  }

  bool operator==(const MemberInfo &rhs) const {
    return id == rhs.id && address == rhs.address;
  }
};

enum MsgTypes{
    JOINREQ, //char addr[6], int ring_id, long heartbeat
	  AD,      //list of nodes in the group
    DUMMYLASTMSGTYPE
};

typedef struct MessageHdr {
	enum MsgTypes msgType;
}MessageHdr;

inline std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}
