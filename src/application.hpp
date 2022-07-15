#pragma once 
#include "headers.hpp"
#include "utils.hpp"
#include "serializer.hpp"

using boost::asio::ip::tcp;
using std::string;
using std::vector;
using boost::shared_ptr;

class application;

class packet_receiver : public boost::enable_shared_from_this<packet_receiver> {
public:
    typedef boost::shared_ptr<packet_receiver> pointer;
    static pointer create(boost::asio::io_context &io_context, application &app)
    {
        auto conn = new packet_receiver(io_context, app);
        return pointer(conn);
    }

    void start(); //read packet size
    shared_ptr<tcp::socket> get_socket() {
        return socket;
    }
private:
    application &app;
    packet_receiver(boost::asio::io_context &io_context, application &app)
        : socket(new tcp::socket(io_context)), packet_sz(0), app(app)
        {}
    void read_packet(const boost::system::error_code & ec/*error*/,
                      size_t bytes_transferred/*bytes_read*/);

    void finish_read(const boost::system::error_code & ec,
                      size_t bytes_transferred);
    shared_ptr<tcp::socket> socket;
    boost::asio::streambuf buffer;
    uint32_t packet_sz;
};

class joinreq_handler: public boost::enable_shared_from_this<joinreq_handler> {
public:
    typedef boost::shared_ptr<joinreq_handler> pointer;
    shared_ptr<tcp::socket> socket;

    static pointer create(MessageHdr *msg, application &app, shared_ptr<tcp::socket> socket) {
        return pointer(new joinreq_handler(msg, app, socket));
    }

private:
    joinreq_handler(MessageHdr *msg, application &app, shared_ptr<tcp::socket> socket);
    application &app;
    boost::asio::streambuf buffer;
};

class application
{
public:
    application(boost::asio::io_context &io_context, int id, unsigned short port, int ring_id);



    void introduce_self_to_group();
    void start_accept();
    void handle_accept(packet_receiver::pointer new_connection,
                       const boost::system::error_code &error);
    void main_loop(const boost::system::error_code& ec);

    bool add_node(const MemberInfo &member);
    void update(const MemberInfo &info);
    void update(const vector<MemberInfo> &info_list);

    bool memberListEntryIsValid(const MemberInfo &e) {
		auto time_diff = get_local_time() - e.timestamp;
        return time_diff.total_seconds() < MyConst::timeoutFail;
	}

    MemberInfo &self_info() {
        return members[0];
    }
private:
    boost::asio::io_context &io_context;
    Address bootstrap_address;
    tcp::acceptor acceptor_;
    
    vector<MemberInfo> members;
    boost::asio::deadline_timer timer;
    int joinreq_retry;
};
