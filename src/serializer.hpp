#pragma once
#include "headers.hpp"
#include "utils.hpp"

namespace pojo 
{
	struct get_t {
		int transaction_id;
		int sender_id;
		int ttl;
		string key;
	};
	struct get_response_t {
		int transaction_id;
		int sender_id;
		int responder_id;
		uint64_t version;
		string value;
	};
	struct set_t {
		int transaction_id;
		int sender_id;
		int ttl;
		uint64_t version;
		string key;
		string value;
	};
}

struct Serializer {
	static void addressToIpPort(const Address &addr, int &id, unsigned short &port);
	static Address ipPortToAddress(int id, unsigned short port);
    static char *encodeAddress(char *mem, const Address& e);
	static char *decodeAddress(char *mem, Address &e);
	static char *encodeString(char *mem, string s);
	static char *decodeString(char *mem, string &s);
	
	struct Message {
		static MessageHdr *allocEncodeJOINREQ(const Address &addr, int id, int ring_id, int heartbeat, uint32_t &msgSize);
		static void decodeJOINREQ(MessageHdr *msg, Address &addr, int &id, int &ring_id, int &heartbeat);
		static MessageHdr *allocEncodeAD(const vector<MemberInfo> &lst);
		static void decodeAD(MessageHdr *msg, vector<MemberInfo> &lst);
		static MessageHdr *allocEncodeGET(pojo::get_t data);
		static void decodeGET(MessageHdr *msg, pojo::get_t &data);
		static void dealloc(MessageHdr *msg) {
			free(msg);
		}
	};
	static char *encodeMemberInfo(char *mem, const MemberInfo& e);
	static char *decodeMemberInfo(char *mem, MemberInfo &e);
	static char *encodeMemberList(char *mem, const vector<MemberInfo> &lst);
	static char *decodeMemberList(char *mem, vector<MemberInfo> &lst);
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
