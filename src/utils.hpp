#pragma once
#include "headers.hpp"
#define LOCALHOST "127.0.0.1"
#define BOOTSTRAP_IP LOCALHOST
#define BOOTSTRAP_PORT 9000
#define RING_SIZE 65535
#define MAX_PACKET_SZ 65535

using std::string;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using boost::shared_ptr;
using boost::bind;
namespace ba = boost::asio;
namespace bpt = boost::posix_time;

typedef boost::posix_time::ptime timestamp_t;

struct MyConst {
	static const int heartbeatInterval = 1;
	static const int resendTimeout = 3;
	static const int GossipInterval = 3;
	static const int timeoutFail = 15;
	static const int timeoutRemove = 30;
	static const int GossipFan = 3;
  static const int check_memebr_interval = 5;
  static const int joinreq_retry_max = 5;
  static const int joinreq_retry_factor = 5;
};

inline timestamp_t get_local_time() {
  return bpt::second_clock::local_time();
}

inline string time_to_string(timestamp_t t) {
  std::stringstream ss;
  ss << t.time_of_day().minutes() << ":" << t.time_of_day().seconds();
  return ss.str();
}

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
  timestamp_t timestamp;

  MemberInfo(): timestamp(get_local_time()) {}
  MemberInfo(Address &addr, int id, int ring_id)
  : address(addr), id(id), ring_id(ring_id), heartbeat(0), isAlive(false), timestamp(get_local_time()) {
  }

  bool operator==(const MemberInfo &rhs) const {
    return id == rhs.id && address == rhs.address;
  }
};

enum MsgTypes{
    DUMMYLASTMSGTYPE = 100,
    JOINREQ, //char addr[6], int ring_id, long heartbeat
	  AD      //list of nodes in the group
};

struct MessageHdr {
  uint32_t size;
	enum MsgTypes msgType;
};

inline std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

inline void print_bytes(void *ptr, int size) 
{
    unsigned char *p = (unsigned char *) ptr;
    int i;
    for (i=0; i<size; i++) {
        if(i > 0 && i%4 == 0) {
          printf("  ");
        }
        printf("%02hhX ", p[i]);
    }
    printf("\n");
}
