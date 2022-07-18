#pragma once
#include "application.hpp"

// ------------------------------- protocol base ---------------------------------------//
template<class T>
class protocol_base : public boost::enable_shared_from_this<T> {
public:
    typedef boost::shared_ptr<T> pointer;
    template <typename ... Args>
    static pointer create(Args&& ... args){
        return pointer(new T(std::forward<Args>(args)...));
    }

protected:
};

/* ------------------------------- packet receiver -------------------------------------*/

class packet_receiver : public protocol_base<packet_receiver> {
public:
    friend class protocol_base<packet_receiver>;
    static int nextId;
    int id;
    typedef boost::function<void(shared_msg)> callback_t;
    typedef boost::function<void()> error_handler_t;
    void start(); //read packet size
    shared_socket get_socket() {
        return socket;
    }
    void set_error_handler(error_handler_t handler) {
        error_handler = handler;
    }

    static pointer create_dispatcher(application &app, shared_socket socket) {
        return create(app, socket, bind(&application::dispatch_packet,
            &app, socket, boost::placeholders::_1));
    }
protected:
    packet_receiver(Loggable &log, shared_socket socket, callback_t cb)
        : log(log), socket(socket), callback(cb), packet_sz(0), id(nextId++)
        {}
    void read_packet(const boost::system::error_code & ec/*error*/,
                      size_t bytes_transferred/*bytes_read*/);

    void finish_read(const boost::system::error_code & ec,
                      size_t bytes_transferred);

    MessageHdr hdr;
    Loggable &log;
    callback_t callback;
    error_handler_t error_handler;
    shared_socket socket;
    boost::asio::streambuf buffer;
    uint32_t packet_sz;
};

/*
 -------------------------- joinreq ----------------------------
*/

class joinreq_handler: public protocol_base<joinreq_handler> {
public:
    friend class protocol_base<joinreq_handler>;
    void start();
    void after_write() {
        app.debug("joinreq response is sent");
    }
    ~joinreq_handler() {
        app.debug("joinreq is released");
    }

protected:
    joinreq_handler(shared_msg msg, application &app, shared_socket socket);
    shared_socket socket;
    shared_msg response;
    application &app;
    boost::asio::streambuf buffer;
};

class joinreq_client: public protocol_base<joinreq_client> {
public:
    friend class protocol_base<joinreq_client>;
    void start();
    void handle_write(const boost::system::error_code & ec,
                      size_t bytes_transferred, packet_receiver::pointer pr);
    void handle_response(shared_msg msg);
    ~joinreq_client() {
    }

protected:
    joinreq_client(application &app);
    shared_socket socket;
    application &app;
    boost::asio::streambuf buffer;
    shared_msg msg;
    boost::asio::deadline_timer timer;
    int joinreq_retry;
};


class ad_sender: public protocol_base<ad_sender> {
public:
    friend class protocol_base<ad_sender>;
    void start();
    ~ad_sender() {
    }

protected:
    ad_sender(application &app);

    void async_connect_send(const Address &addr);
    void async_send(shared_socket socket);
    void after_send(shared_socket socket) {
        //need to hold a pointer to socket otherwise it is destroied
    }
    shared_msg msg;
    application &app;
};

class ad_handler : public protocol_base<ad_handler> {
public:
    friend class protocol_base<ad_handler>;
    void start();
    ~ad_handler() {
    }

protected:
    ad_handler(application &app, shared_msg msg):
        app(app), msg(msg) {}
    shared_msg msg;
    application &app;
};

//---------------------------SET--------------------------------
class set_handler : public protocol_base<set_handler> {
public:
    friend class protocol_base<set_handler>;

    void start();
    void do_coordinate();
    void do_execute();
    void start_prc(packet_receiver::pointer prc) {
        prc->start();
    }

    //coordinator
    void send_req_to_peer(int idx);
    void read_peer_response(shared_msg res_msg, int idx);
    void handle_peer_response(shared_msg res_msg, int idx);
    void after_commit_to_peer(int idx);
    void send_response();
    void prc_error_handler();

    //executor
    void handle_commit(shared_msg commit_msg);
    void after_final_response();

    ~set_handler() {
        app.debug("release set_handler, coordinator = %d", is_coordinator);
    }
protected:
    set_handler(application &app, shared_msg msg, shared_socket socket);
    shared_socket socket;
    application &app;
    dh_message::SetRequest request;
    shared_msg request_msg;
    dh_message::SetResponse response;
    shared_msg response_msg;

    //coordinator
    vector<MemberInfo> peers;
    vector<shared_socket> peer_sockets;
    int responsed_peer_cnt;
    int final_responsed_peer_cnt;
    vector<unsigned char> peer_response;
    shared_msg peer_commit_req;

    //executor
    shared_msg response_to_cood;
    shared_msg final_response_to_cood;

    bool is_coordinator;
};

//--------------------------GET----------------------------------

class get_handler : public protocol_base<get_handler> {
public:
    friend class protocol_base<get_handler>;

    void start();

    void do_coordinate();
    void send_req_to_peer(int idx);
    void read_peer_response(int idx, shared_msg msg);
    void start_prc(packet_receiver::pointer prc);
    void prc_error_handler(int idx);
    void send_response();

    void do_execute();
    void after_response();

    ~get_handler() {
        app.debug("release get_handler");
    }

protected:
    get_handler(application &app, shared_msg msg, shared_socket socket);
    shared_socket socket;
    application &app;
    dh_message::GetRequest request;
    shared_msg msg_to_peers;
    dh_message::GetResponse response;
    shared_msg response_msg;

    //coordinator
    vector<MemberInfo> peers;
    vector<shared_socket> peer_sockets;
    int connected_peer_cnt;
    int responsed_peer_cnt;
    vector<dh_message::GetResponse> peer_response;
    int best_peer_response_idx;
    uint64_t latest_version;
    bool is_success;

    //executor

};



