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
    void set_callback(callback_t cb) {
        callback = cb;
    }
    void start(); //read packet size
    shared_ptr<tcp::socket> get_socket() {
        return socket;
    }
protected:
    application &app;
    packet_receiver(application &app)
        : socket(new tcp::socket(app.get_context())), packet_sz(0), app(app), id(nextId++)
        {}
    packet_receiver(application &app, shared_ptr<tcp::socket> socket)
    : socket(socket), packet_sz(0), app(app),  id(nextId++)
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

/*
 -------------------------- joinreq ----------------------------
*/

class joinreq_handler: public protocol_base<joinreq_handler> {
public:
    friend class protocol_base<joinreq_handler>;
    void start();
    void after_write() {
        cout << "joinreq response is sent" << endl;
        Serializer::Message::dealloc(response);
    }
    ~joinreq_handler() {
        cout << "joinreq is released" << endl;
    }

protected:
    joinreq_handler(MessageHdr *msg, application &app, shared_ptr<tcp::socket> socket);
    shared_ptr<tcp::socket> socket;
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
        cout << "joinreq client is released" << endl;
        Serializer::Message::dealloc(msg);
    }

protected:
    joinreq_client(application &app);
    shared_ptr<tcp::socket> socket;
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
    void async_send(shared_ptr<tcp::socket> socket);
    void after_send(shared_ptr<tcp::socket> socket) {
        //need to hold a pointer to socket otherwise it is destroied
        cout << "ad is sent" << endl;
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