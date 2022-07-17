#pragma once
#include "headers.hpp"
#include "logger.hpp"
#define LOCALHOST "127.0.0.1"
#define BOOTSTRAP_IP LOCALHOST
#define BOOTSTRAP_PORT 9000
#define RING_SIZE 65535
#define MAX_PACKET_SZ 65535

using std::string;
using std::vector;
using std::unordered_map;
using std::pair;
using std::tuple;
using boost::shared_ptr;
using boost::bind;
namespace ba = boost::asio;
namespace bpt = boost::posix_time;

typedef boost::posix_time::ptime timestamp_t;
typedef shared_ptr<ba::ip::tcp::socket> shared_socket;
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
  static const int request_default_ttl = 3;
  static const uint32_t ring_size = 1 << 16;
  static const int MAX_PEER_SIZE = 16;
};

inline timestamp_t get_local_time() {
  return bpt::second_clock::local_time();
}

inline string time_to_string(timestamp_t t) {
  std::stringstream ss;
  ss << t.time_of_day().minutes() << ":" << t.time_of_day().seconds();
  return ss.str();
}

inline int md5_mod(string s, int mod = MyConst::ring_size) {
  using boost::uuids::detail::md5;
  md5 hash;
  md5::digest_type digest;

  hash.process_bytes(s.data(), s.size());
  hash.get_digest(digest);
  const int *p = (const int *) &digest;
  int last_32_bits = p[3];
  return last_32_bits % mod;
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

enum class MsgType{
    DUMMY_START = 100,
    JOINREQ, //char addr[6], int ring_id, long heartbeat
	  AD,      //list of nodes in the group
    GET,     //get value by key (key_t, ttl) -> (value_t, success?)
    GET_RESPONSE,
    SET,      //set value by key (key_t, value_t, ttl) -> success?
    SET_RESPONSE,
    DUMMY_END
};

struct MessageHdr {
  uint32_t size;
	enum MsgType msgType;
  char payload[0];
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
