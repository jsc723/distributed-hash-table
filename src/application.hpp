#pragma once 
#include "headers.hpp"
#include "utils.hpp"
#include "serializer.hpp"
#include "data_store.hpp"

using ba::ip::tcp;
using std::string;
using std::vector;
using boost::shared_ptr;

class application;
class packet_receiver;

/*
------------------------- Application ---------------------------------
*/

class application : public Loggable
{
public:
    typedef ba::deadline_timer timer_t;
    typedef boost::function<void()> task_callback_t;
    typedef boost::posix_time::time_duration duration_t;

    application(ba::io_context &io_context, int id, unsigned short port, int ring_id);

    void init();


    void introduce_self_to_group();
    void start_accept();
    void handle_accept(shared_ptr<packet_receiver> new_connection,
                       const boost::system::error_code &error);
    void dispatch_packet(shared_socket socket, MessageHdr *msg);

    void repeating_task_template(const boost::system::error_code& ec, int timer_idx,
                         task_callback_t callback, duration_t interval);

    void add_repeating_task(task_callback_t callback, duration_t interval);
    // --------- repeating tasks
    void heartbeat_loop();
    void ad_loop();
    void check_member_loop();
    // -------------------------

    bool add_node(const MemberInfo &member);
    void update_self();
    void update(const MemberInfo &info, bool ignore_timestamp = false);
    void update(const vector<MemberInfo> &info_list);


    vector<MemberInfo> getValidMembers();
    vector<Address> sampleNodes(int k);

    bool memberListEntryIsValid(const MemberInfo &e) {
		auto time_diff = get_local_time() - e.timestamp;
        return time_diff.total_seconds() < MyConst::timeoutFail;
	}

    bool memberListEntryShouldBeRemoved(const MemberInfo &e) {
        auto time_diff = get_local_time() - e.timestamp;
		return time_diff.total_seconds() >= MyConst::timeoutRemove;
	}

    MemberInfo &self_info() {
        return members[self_index];
    }
    ba::io_context & get_context() {
        return io_context;
    }
    Address get_bootstrap_address() {
        return bootstrap_address;
    }
    data_store &get_store() {
        return store;
    }
    MemberInfo &get_member(int idx) {
        if (idx >= members.size()) {
            critical("get_member idx out of range: %d", idx);
        }
        return members[idx];
    }
    int map_key_to_node_idx(const data_store::key_t &key);

    virtual void vlog(LogLevel level, const char *format, va_list args){
        sprintf(prefix_buf, "[id=%d][h=%d] ", self_info().id, self_info().heartbeat);
        logger.vlog(level, prefix_buf, format, args);
    }
private:
    ba::io_context &io_context;
    Address bootstrap_address;
    tcp::acceptor acceptor_;
    
    int self_index;
    vector<MemberInfo> members;
    vector<ba::deadline_timer> timers;
    data_store store;
    char prefix_buf[1024];
};
