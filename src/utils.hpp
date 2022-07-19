#pragma once
#include "headers.hpp"
#include "logger.hpp"
#define LOCALHOST "127.0.0.1"
#define BOOTSTRAP_IP "127.0.0.1"
#define BOOTSTRAP_PORT 9000
#define MSG_HEAD 0xbeefcade

using std::string;
using std::vector;
using std::unordered_map;
using std::unordered_set;
using std::map;
using std::pair;
using std::tuple;
using boost::shared_ptr;
using boost::bind;
namespace ba = boost::asio;
namespace bpt = boost::posix_time;

class MsgHdr;
typedef boost::posix_time::ptime timestamp_t;
typedef shared_ptr<ba::ip::tcp::socket> shared_socket;
typedef shared_ptr<MsgHdr> shared_msg;
struct DHConst {
	static const int HeartBeatInterval = 1;
	static const int ResendTimeout = 3;
	static const int GossipInterval = 3;
	static const int TimeoutFail = 15;
	static const int TimeoutRemove = 30;
	static const int GossipFan = 3;
  static const int CheckMemberInterval = 5;
  static const int JoinRetryMax = 5;
  static const int JoinRetryFactor = 5;
  static const int RequestDefaultTTL = 3;
  static const uint32_t RingSize = 1 << 16;
  static const int ReplicaSize = 3;
  static const int MaxPacketSize = 65535;
  static const int PushMaxCount = 5;
  static const int PushInterval = 5;
  static const int PushRandomStringLen = 3;
  static const int CommunicationTimeout = 5;
};

inline timestamp_t get_local_time() {
  return bpt::second_clock::local_time();
}

inline string time_to_string(timestamp_t t) {
  std::stringstream ss;
  ss << t.time_of_day().minutes() << ":" << t.time_of_day().seconds();
  return ss.str();
}

inline int md5_mod(string s, int mod = DHConst::RingSize) {
  using boost::uuids::detail::md5;
  md5 hash;
  md5::digest_type digest;

  hash.process_bytes(s.data(), s.size());
  hash.get_digest(digest);
  const int *p = (const int *) &digest;
  int result = p[3] % mod;
  if (result < 0) result += mod;
  return result % mod;
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
  MemberInfo(const dh_message::MemberInfo &proto_info)
  : id(proto_info.id()),
    ring_id(proto_info.ring_id()),
    heartbeat(proto_info.heartbeat()),
    isAlive(false), 
    timestamp(get_local_time()) 
  {
     address.ip = boost::asio::ip::make_address_v4(proto_info.address().ip());
     address.port = (unsigned short)proto_info.address().port();
  }

  bool operator==(const MemberInfo &rhs) const {
    return id == rhs.id && address == rhs.address;
  }

  bool isValid() {
    auto time_diff = get_local_time() - timestamp;
    return time_diff.total_seconds() < DHConst::TimeoutFail;
  }

  bool shouldRemoved() {
    auto time_diff = get_local_time() - timestamp;
    return time_diff.total_seconds() >= DHConst::TimeoutRemove;
  }

  dh_message::MemberInfo toProto() const {
    dh_message::MemberInfo proto_info;
    proto_info.mutable_address()->set_ip(address.ip.to_uint());
    proto_info.mutable_address()->set_port(address.port);
    proto_info.set_id(id);
    proto_info.set_ring_id(ring_id);
    proto_info.set_heartbeat(heartbeat);
    return proto_info;
  }
};

enum class MsgType{
    DUMMY_START = 100,
    JOIN, //char addr[6], int ring_id, long heartbeat
	  AD,      //list of nodes in the group
    GET,     //get value by key (key_t, ttl) -> (value_t, success?)
    GET_RESPONSE,
    SET,      //set value by key (key_t, value_t, ttl) -> success?
    SET_RESPONSE,
    PUSH,     //list of key value pairs
    DUMMY_END
};

struct MsgHdr {
  uint32_t HEAD;
  uint32_t size;
	enum MsgType msgType;
  char payload[0];
  auto buffer() -> decltype(ba::buffer(this, size)) {
    return ba::buffer(this, size);
  }
  auto payload_buffer() -> decltype(ba::buffer(this, size)) {
    return ba::buffer(payload, size - sizeof(*this));
  }
  static shared_msg create_shared(uint32_t size) {
    auto ptr = shared_msg((MsgHdr*)malloc(size), free);
    ptr->HEAD = MSG_HEAD;
    ptr->size = size;
    ptr->msgType = MsgType::DUMMY_START;
    return ptr;
  }
  size_t payload_size() {
    return size - sizeof(*this);
  }
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

inline std::string random_string(const int len) {
    static const char alphanum[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    
    return tmp_s;
}
