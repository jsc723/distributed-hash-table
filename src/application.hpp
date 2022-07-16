#pragma once 
#include "headers.hpp"
#include "utils.hpp"
#include "serializer.hpp"

using boost::asio::ip::tcp;
using std::string;
using std::vector;
using boost::shared_ptr;

class application;
class packet_receiver;

/*
------------------------- Application ---------------------------------
*/

class application
{
public:
    typedef boost::asio::deadline_timer timer_t;
    typedef boost::function<void()> task_callback_t;
    typedef boost::posix_time::time_duration duration_t;

    application(boost::asio::io_context &io_context, int id, unsigned short port, int ring_id);


    void introduce_self_to_group();
    void start_accept();
    void handle_accept(shared_ptr<packet_receiver> new_connection,
                       const boost::system::error_code &error);

    void repeating_task_template(const boost::system::error_code& ec, int timer_idx,
                         task_callback_t callback, duration_t interval);

    void heartbeat_loop();

    void add_repeating_task(task_callback_t callback, duration_t interval);
    void ad_loop();

    bool add_node(const MemberInfo &member);
    void update_self();
    void update(const MemberInfo &info);
    void update(const vector<MemberInfo> &info_list);


    vector<MemberInfo> getValidMembers();
    vector<Address> sampleNodes(int k);

    bool memberListEntryIsValid(const MemberInfo &e) {
		auto time_diff = get_local_time() - e.timestamp;
        return time_diff.total_seconds() < MyConst::timeoutFail;
	}

    MemberInfo &self_info() {
        return members[0];
    }
    boost::asio::io_context & get_context() {
        return io_context;
    }
    Address get_bootstrap_address() {
        return bootstrap_address;
    }
private:
    boost::asio::io_context &io_context;
    Address bootstrap_address;
    tcp::acceptor acceptor_;
    
    vector<MemberInfo> members;
    vector<boost::asio::deadline_timer> timers;
};
