#pragma once
#include "headers.hpp"
#include "utils.hpp"


struct Serializer {
	static void addressToIpPort(const Address &addr, int &id, unsigned short &port);
	static Address ipPortToAddress(int id, unsigned short port);
    static char *encodeAddress(char *mem, const Address& e);
	static char *decodeAddress(char *mem, Address &e);
	
	struct Message {
		static MessageHdr *allocEncodeJOINREQ(const Address &addr, int id, int ring_id, long heartbeat, size_t &msgSize);
		static void decodeJOINREQ(MessageHdr *msg, Address &addr, int &id, int &ring_id, long &heartbeat);
		// static MessageHdr *allocEncodeAD(const vector<MemberInfo> &lst, size_t &msgSize);
		// static void decodeAD(MessageHdr *msg, vector<MemberInfo> &lst);
		static void dealloc(MessageHdr *msg) {
			free(msg);
		}
	};
	// static char *encodeMemberInfo(char *mem, const MemberInfo& e);
	// static char *decodeMemberInfo(char *mem, MemberInfo &e);
	// static char *encodeMemberList(char *mem, const vector<MemberInfo> &lst);
	// static char *decodeMemberList(char *mem, vector<MemberInfo> &lst);
};

struct Mem {
	template<typename T>
	static char *write(char *dst, T val) {
		memcpy(dst, &val, sizeof(val));
		return dst + sizeof(val);
	}
	template<typename T>
	static char *read(char *src, T &val) {
		memcpy(&val, src, sizeof(val));
		return src + sizeof(val);
	}
};
