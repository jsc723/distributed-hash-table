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
    };
    pair<value_t, bool> get(const key_t &key);
    bool set(const key_t &key, const value_t& val);
private:
    unordered_map<key_t, value_t> m;
};