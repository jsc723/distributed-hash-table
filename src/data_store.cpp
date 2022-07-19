#include "data_store.hpp"



pair<data_store::value_t, bool> data_store::get(const data_store::key_t& key) {
    if (m.count(key) == 0) {
        return std::make_pair(data_store::value_t{}, false);
    }
    return std::make_pair(m[key], true);
}

bool data_store::set(const data_store::key_t &key, const data_store::value_t& val) {
    logger.log(LogLevel::DEBUG, "data_storen", "key=%s, version=%ld", key.c_str(), val.version);
    if (m.count(key) == 0 || m[key].version < val.version || val.version == 0) {
        m[key] = val;
        logger.log(LogLevel::DEBUG, "data_store ", "setted");
        return true;
    }
    if (m[key] == val) {
        logger.log(LogLevel::DEBUG, "data_store ", "setted2");
        return true;
    }
    logger.log(LogLevel::DEBUG, "data_store ", "not setted");
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

vector<pair<data_store::key_t, data_store::value_t>> data_store::get_multiple_lower_bound(const key_t &key, int count) {
    vector<pair<data_store::key_t, data_store::value_t>> res;
    auto it = m.lower_bound(key);
    bool around = false;
    while(count > 0) {
        if (it == m.end()) {
            if (!around) {
                it = m.begin();
                around = true;
                continue;
            } else {
                break;
            }
        }

        res.emplace_back(*it);
        count--;
        it++;
    }
    return res;
}