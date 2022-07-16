#include "data_store.hpp"



pair<data_store::value_t, bool> data_store::get(const data_store::key_t& key) {
    if (m.count(key) == 0) {
        return std::make_pair(data_store::value_t{}, false);
    }
    return std::make_pair(m[key], true);
}

bool data_store::set(const data_store::key_t &key, const data_store::value_t& val) {
    if (m.count(key) == 0 || m[key].version < val.version) {
        m[key] = val;
        return true;
    }
    if (m[key] == val) {
        return true;
    }
    return false;
}