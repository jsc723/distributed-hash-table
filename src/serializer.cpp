#include "serializer.hpp"


Logger logger;
//----------------------------------------------------------------//
/* ------------------------- Serializer --------------------------*/
//----------------------------------------------------------------//

void Serializer::addressToIpPort(const Address &addr, int &id, unsigned short &port) {
    id = addr.ip.to_uint();
	port = addr.port;
}
Address Serializer::ipPortToAddress(int ip, unsigned short port) {
    Address addr;
    addr.ip = boost::asio::ip::make_address_v4(ip);
    addr.port = port;
    return addr;
}

shared_msg Serializer::allocEncodeAD(const vector<MemberInfo> &lst) {
    dh_message::Advertisement ad;
    for(auto &e: lst) {
        *ad.add_member() = e.toProto();
        logger.log(LogLevel::DEBUG, "[Serializer]", "encode id=%d, hb=%d", e.id, e.heartbeat);
    }
    logger.log(LogLevel::DEBUG, "[Serializer]", "encoded member sz =%d", ad.member_size());
    return allocEncode(MsgType::AD, ad);
}
void Serializer::decodeAD(shared_msg msg, vector<MemberInfo> &lst) {
    dh_message::Advertisement ad;
    ad.ParseFromArray(msg->payload, msg->payload_size());
    logger.log(LogLevel::DEBUG, "[Serializer]", "decode ad, member_sz=%d", ad.member_size());
    for(int i = 0; i < ad.member_size(); i++) {
        lst.emplace_back(ad.member(i));
        logger.log(LogLevel::DEBUG, "[Serializer]", "decode id=%d, hb=%d", lst.back().id, lst.back().heartbeat);
    }
}

