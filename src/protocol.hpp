#pragma once
#include "application.hpp"

/* ------------------------------- packet receiver -------------------------------------*/

class packet_receiver : public boost::enable_shared_from_this<packet_receiver> {
public:
    static int nextId;
    int id;
    typedef boost::shared_ptr<packet_receiver> pointer;
    typedef boost::function<void(MessageHdr *)> callback_t;
    static pointer create(boost::asio::io_context &io_context, application &app)
    {
        auto conn = new packet_receiver(io_context, app);
        printf("packet_receiver %d created\n", conn->id);
        return pointer(conn);
    }
    static pointer create(boost::asio::io_context &io_context, application &app, shared_ptr<tcp::socket> socket)
    {
        auto conn = new packet_receiver(io_context, app, socket);
        printf("packet_receiver %d created\n", conn->id);
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
        printf("packet_receiver %d released\n", id);
    }
private:
    application &app;
    packet_receiver(boost::asio::io_context &io_context, application &app)
        : socket(new tcp::socket(io_context)), packet_sz(0), app(app), id(nextId++)
        {}
    packet_receiver(boost::asio::io_context &io_context, application &app, shared_ptr<tcp::socket> socket)
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


class ad_sender: public boost::enable_shared_from_this<ad_sender> {
public:
    typedef boost::shared_ptr<ad_sender> pointer;

    static pointer create(application &app) {
        return pointer(new ad_sender(app));
    }
    void start();
    ~ad_sender() {
        Serializer::Message::dealloc(msg);
    }

private:
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

class ad_handler : public boost::enable_shared_from_this<ad_handler> {
public:
    typedef shared_ptr<ad_handler> pointer;

    static pointer create(application &app, MessageHdr *msg) {
        return pointer(new ad_handler(app, msg));
    }
    void start();
    ~ad_handler() {
        Serializer::Message::dealloc(msg);
    }

private:
    ad_handler(application &app, MessageHdr *msg):
        app(app), msg(msg) {}
    MessageHdr *msg;
    application &app;
};