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

    application(ba::io_context &io_context, int id, string ip, unsigned short port, int ring_id,
                string bootstrap_ip, unsigned short bootstrap_port);

    void init();


    void join_cluster();
    void start_accept();
    void handle_accept(shared_ptr<packet_receiver> new_connection,
                       const boost::system::error_code &error);
    void dispatch_packet(shared_socket socket, shared_msg msg);

    void repeating_task_template(const boost::system::error_code& ec, int timer_idx,
                         task_callback_t callback, duration_t interval);

    void add_repeating_task(task_callback_t callback, duration_t interval);
    // --------- repeating tasks
    void heartbeat_loop();
    void ad_loop();
    void check_member_loop();
    void push_loop();
    // -------------------------

    bool add_node(const MemberInfo &member);
    void update_self();
    void update(const MemberInfo &info, bool ignore_timestamp = false);
    void update(const vector<MemberInfo> &info_list);


    vector<MemberInfo> getValidMembers();
    vector<Address> sampleNodes(int k);
    MemberInfo prevNode() {
        if (members.size() == 0) {
            critical("prevNode: member size is 0");
            exit(1);
        }
        return members[(self_index + (members.size() - 1)) % members.size()];
    }
    MemberInfo nextNode() {
        if (members.size() == 0) {
            critical("nextNode: member size is 0");
            exit(1);
        }
        return members[(self_index + 1) % members.size()];
    }


    bool self_is_responsible_for_key(const string &key);

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
    vector<MemberInfo> get_group_starting_at(int idx, int max_size);

    virtual void vlog(LogLevel level, const char *format, va_list args){
        sprintf(prefix_buf, "[id=%d][h=%d] ", self_info().id, self_info().heartbeat);
        logger.vlog(level, prefix_buf, format, args);
    }
private:
    ba::io_context &io_context;
    Address bootstrap_address;
    tcp::acceptor acceptor_;

    bool is_bootstrap_server;
    
    int self_index;
    vector<MemberInfo> members;
    vector<ba::deadline_timer> timers;
    data_store store;
    char prefix_buf[1024];
};
