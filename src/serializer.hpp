#pragma once
#include "headers.hpp"
#include "utils.hpp"

struct Serializer {
	static shared_msg allocEncodeAD(const vector<MemberInfo> &lst);
	static void decodeAD(shared_msg msg, vector<MemberInfo> &lst);

	template<typename T>
	static shared_msg allocEncode(MsgType type, T &req) {
		int data_sz = (int)req.ByteSizeLong();
		uint32_t msg_sz = sizeof(MsgHdr) + data_sz;
		shared_msg msg = MsgHdr::create_shared(msg_sz);
		msg->msgType = type;
		char *payload = (char*)(msg.get()+1);
		req.SerializeToArray(payload, data_sz);
		return msg;
	}

private:
	static void addressToIpPort(const Address &addr, int &id, unsigned short &port);
	static Address ipPortToAddress(int id, unsigned short port);

};