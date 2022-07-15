#include "serializer.hpp"

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

char *Serializer::encodeAddress(char *mem, const Address& addr) {
    int ip; unsigned short port;
    addressToIpPort(addr, ip, port);
    mem = Mem::write(mem, ip);
    return Mem::write(mem, port);
}
char *Serializer::decodeAddress(char *mem, Address &addr) {
    int ip; unsigned short port;
    mem = Mem::read(mem, ip);
    mem = Mem::read(mem, port);
    addr = ipPortToAddress(ip, port);
    return mem;
}
	
MessageHdr *Serializer::Message::allocEncodeJOINREQ(const Address &myaddr, int id, int ring_id, int heartbeat, uint32_t &msgSize) {
    uint32_t addr_sz = sizeof(int) + sizeof(short);
    msgSize = sizeof(MessageHdr) + addr_sz + sizeof(id) + sizeof(ring_id) + sizeof(heartbeat);
    MessageHdr *msg = (MessageHdr *) malloc(msgSize);
    msg->msgType = JOINREQ;
    auto p = (char *)(msg+1);
    p = encodeAddress(p, myaddr);
    p = Mem::write(p, id);
    p = Mem::write(p, ring_id);
    p = Mem::write(p, heartbeat);
    return msg;
}
void Serializer::Message::decodeJOINREQ(MessageHdr *msg, Address &addr, int &id, int &ring_id, int &heartbeat) {
    auto p = (char *)(msg+1);
    p = decodeAddress(p, addr);
    p = Mem::read(p, id);
    p = Mem::read(p, ring_id);
    p = Mem::read(p, heartbeat);
}
// MessageHdr *Serializer::Message::allocEncodeAD(const vector<MemberInfo> &lst, uint32_t &msgSize) {
//     const uint32_t entrySize = sizeof(MemberInfo::id) 
//                             + sizeof(MemberInfo::port)
//                             + sizeof(MemberInfo::heartbeat) 
//                             + sizeof(MemberInfo::timestamp);//4,2,8,8
//     uint32_t lstSize = sizeof(uint32_t) + lst.size() * entrySize;
//     msgSize = sizeof(MessageHdr) + lstSize;
//     MessageHdr *msg = (MessageHdr *) malloc(msgSize);
//     msg->msgType = AD;
//     encodeMemberList((char *)(msg+1), lst);

//     return msg;
// }
// void Serializer::Message::decodeAD(MessageHdr *msg, vector<MemberInfo> &lst) {
//     decodeMemberList((char *)(msg+1), lst);
// }
// char *Serializer::encodeMemberInfo(char *mem, const MemberInfo& e) {
//     auto p = mem;
//     p = Mem::write(p, e.id);
//     p = Mem::write(p, e.port);
//     p = Mem::write(p, e.heartbeat);
//     p = Mem::write(p, e.timestamp);
//     return p;
// }
// char *Serializer::decodeMemberInfo(char *mem, MemberInfo &e) {
//     auto p = mem;
//     p = Mem::read(p, e.id);
//     p = Mem::read(p, e.port);
//     p = Mem::read(p, e.heartbeat);
//     p = Mem::read(p, e.timestamp);
//     return p;
// }
// char *Serializer::encodeMemberList(char *mem, const vector<MemberInfo> &lst) {
//     uint32_t sz = lst.size();
//     auto p = mem;
//     p = Mem::write(p, sz);
//     for(auto &e: lst) {
//         p = encodeMemberInfo(p, e);
//     }
//     return p;
// }
// char *Serializer::decodeMemberList(char *mem, vector<MemberInfo> &lst) {
//     uint32_t sz;
//     auto p = Mem::read(mem, sz);
//     for(uint32_t i = 0; i < sz; i++) {
//         MemberInfo e;
//         p = decodeMemberInfo(p, e);
//         lst.emplace_back(move(e));
//     }
//     return p;
// }
