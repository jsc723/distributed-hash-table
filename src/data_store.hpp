#pragma once
#include "headers.hpp"
#include "utils.hpp"

class data_store {
public:
    typedef string key_t;
    struct value_t {
        string value;
        uint64_t version;
        bool operator==(const value_t &rhs) {
            return value == rhs.value && version == rhs.version;
        }
        value_t() {}
        value_t(string value, uint64_t version) : value(value), version(version) {}
        value_t(const dh_message::VersionedValue &vv): value(vv.value()), version(vv.version()) {}
    };
    pair<value_t, bool> get(const key_t &key);
    bool set(const key_t &key, const value_t& val);
    bool lock(const key_t &key);
    bool check_lock(const key_t &key);
    void release(const key_t &key);
    vector<pair<key_t, value_t>> get_multiple_lower_bound(const key_t &key, int count);
private:
    map<key_t, value_t> m;
    unordered_set<key_t> locks;
};