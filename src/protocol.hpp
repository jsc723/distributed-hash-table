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
    typedef boost::function<void(MessageHdr *)> callback_t;
    void start(); //read packet size
    shared_socket get_socket() {
        return socket;
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

    Loggable &log;
    callback_t callback;
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
        Serializer::Message::dealloc(response);
    }
    ~joinreq_handler() {
        app.debug("joinreq is released");
    }

protected:
    joinreq_handler(MessageHdr *msg, application &app, shared_socket socket);
    shared_socket socket;
    MessageHdr *response;
    application &app;
    boost::asio::streambuf buffer;
};

class joinreq_client: public protocol_base<joinreq_client> {
public:
    friend class protocol_base<joinreq_client>;
    void start();
    void handle_write(const boost::system::error_code & ec,
                      size_t bytes_transferred, packet_receiver::pointer pr);
    void handle_response(MessageHdr *msg);
    ~joinreq_client() {
        Serializer::Message::dealloc(msg);
    }

protected:
    joinreq_client(application &app);
    shared_socket socket;
    application &app;
    boost::asio::streambuf buffer;
    MessageHdr *msg;
    boost::asio::deadline_timer timer;
    int joinreq_retry;
};


class ad_sender: public protocol_base<ad_sender> {
public:
    friend class protocol_base<ad_sender>;
    void start();
    ~ad_sender() {
        Serializer::Message::dealloc(msg);
    }

protected:
    ad_sender(application &app);

    void async_connect_send(const Address &addr);
    void async_send(shared_socket socket);
    void after_send(shared_socket socket) {
        //need to hold a pointer to socket otherwise it is destroied
    }
    MessageHdr *msg;
    application &app;
};

class ad_handler : public protocol_base<ad_handler> {
public:
    friend class protocol_base<ad_handler>;
    void start();
    ~ad_handler() {
        Serializer::Message::dealloc(msg);
    }

protected:
    ad_handler(application &app, MessageHdr *msg):
        app(app), msg(msg) {}
    MessageHdr *msg;
    application &app;
};

//---------------------------GET--------------------------------
class get_handler : public protocol_base<get_handler> {
public:
    friend class protocol_base<get_handler>;
    void start(){}
    ~get_handler() {}
protected:
    get_handler(application &app)
    :app(app) {}
    application &app;
};

//---------------------------SET--------------------------------
class set_handler : public protocol_base<set_handler> {
public:
    friend class protocol_base<set_handler>;
    typedef boost::function<void(shared_socket socket, MessageHdr *)> callback_t;

    void set_callback(callback_t cb) {
        callback = cb;
    }

    void start();
    void do_coordinate();
    void do_execute();
    void after_response(packet_receiver::pointer prc) {
        prc->start();
    }

    //coordinator
    void send_req_to_peer(int idx);
    void read_peer_response(int idx);
    void handle_peer_response(int idx);
    void after_commit_to_peer(int idx);

    //executor
    void read_commit();
    void handle_commit();
    void after_final_response();

    ~set_handler() {
        app.debug("release set_handler");
        Serializer::Message::dealloc(response_msg);
        Serializer::Message::dealloc(request_msg);
    }
protected:
    set_handler(application &app, MessageHdr *msg, shared_socket socket);
    shared_socket socket;
    application &app;
    dh_message::SetRequest request;
    MessageHdr *request_msg;
    dh_message::SetResponse response;
    MessageHdr *response_msg;

    //coordinator
    vector<MemberInfo> peers;
    vector<shared_socket> peer_sockets;
    int responsed_peer_cnt;
    unsigned char peer_response[MyConst::MAX_PEER_SIZE];
    unsigned char peer_commit_req;
    unsigned char peer_commit_response[MyConst::MAX_PEER_SIZE];

    //executor
    unsigned char response_to_cood;
    unsigned char commit_from_cood;
    unsigned char final_response_to_cood;
    callback_t callback;
};


