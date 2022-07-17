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

bool data_store::lock(const key_t &key) {
    if (locks.count(key)) return false;
    locks.emplace(key);
    return true;
}
bool data_store::check_lock(const key_t &key) {
    return locks.count(key) > 0;
}
void data_store::release(const key_t &key) {
    locks.erase(key);
}