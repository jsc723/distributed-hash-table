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
    typedef boost::function<void(MessageHdr *)> callback_t;
    static pointer create(boost::asio::io_context &io_context, application &app)
    {
        auto conn = new packet_receiver(io_context, app);
        return pointer(conn);
    }
    static pointer create(boost::asio::io_context &io_context, application &app, shared_ptr<tcp::socket> socket)
    {
        auto conn = new packet_receiver(io_context, app, socket);
        return pointer(conn);
    }

    void set_callback(callback_t cb) {
        callback = cb;
    }

    void start(); //read packet size
    shared_ptr<tcp::socket> get_socket() {
        return socket;
    }
    ~packet_receiver() {
        cout << "packet_receiver released" << endl;
    }
private:
    application &app;
    packet_receiver(boost::asio::io_context &io_context, application &app)
        : socket(new tcp::socket(io_context)), packet_sz(0), app(app)
        {}
    packet_receiver(boost::asio::io_context &io_context, application &app, shared_ptr<tcp::socket> socket)
    : socket(socket), packet_sz(0), app(app)
    {}
    void read_packet(const boost::system::error_code & ec/*error*/,
                      size_t bytes_transferred/*bytes_read*/);

    void finish_read(const boost::system::error_code & ec,
                      size_t bytes_transferred);

    callback_t callback;
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
    void start();
    void after_write() {
        cout << "joinreq response is sent" << endl;
        Serializer::Message::dealloc(response);
    }
    ~joinreq_handler() {
        cout << "joinreq is released" << endl;
    }

private:
    joinreq_handler(MessageHdr *msg, application &app, shared_ptr<tcp::socket> socket);

    MessageHdr *response;
    uint32_t response_sz;
    application &app;
    boost::asio::streambuf buffer;
};

class joinreq_client: public boost::enable_shared_from_this<joinreq_client> {
public:
    typedef boost::shared_ptr<joinreq_client> pointer;
    shared_ptr<tcp::socket> socket;

    static pointer create(application &app) {
        return pointer(new joinreq_client(app));
    }
    void start();
    void handle_write(const boost::system::error_code & ec,
                      size_t bytes_transferred, packet_receiver::pointer pr);
    void handle_response(MessageHdr *msg);
    ~joinreq_client() {
        cout << "joinreq client is released" << endl;
        Serializer::Message::dealloc(msg);
    }

private:
    joinreq_client(application &app);

    application &app;
    boost::asio::streambuf buffer;
    MessageHdr *msg;
    boost::asio::deadline_timer timer;
    int joinreq_retry;
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
    boost::asio::deadline_timer timer;
};
