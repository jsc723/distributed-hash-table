#include "application.hpp"

void packet_receiver::start()
{
    boost::asio::async_read(*socket, buffer, boost::asio::transfer_exactly(sizeof(uint32_t)),
                            boost::bind(&packet_receiver::read_packet, shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

void packet_receiver::read_packet(const boost::system::error_code & ec, size_t bytes_transferred)
{
    if (ec || bytes_transferred != sizeof(uint32_t)) {
        cout << "did not read the num of bytes to transfer, bytes transfered = " 
        << bytes_transferred << endl;
        if (ec) {
            cout << ec.message() << endl;
        }
        return;
    }
    std::istream is(&buffer);
    is.read((char*)&packet_sz, sizeof(packet_sz));
    cout << "packet sz is " << packet_sz << endl;
    if (packet_sz > MAX_PACKET_SZ || packet_sz < sizeof(packet_sz)) { 
        cout << "packer size is invalid: " << packet_sz << endl;
        return;
    }
    boost::asio::async_read(*socket, buffer, boost::asio::transfer_exactly(packet_sz - sizeof(packet_sz)),
                            boost::bind(&packet_receiver::finish_read, shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
}

void packet_receiver::finish_read(const boost::system::error_code & ec, size_t bytes_transferred)
{
    if (ec) {
        cout << "error: " << ec.message() << endl;
        return;
    }
    if (bytes_transferred != packet_sz - sizeof(packet_sz)) {
        cout << "did not read the whole packet, packet_sz = " << packet_sz
                << ", transferred = " << bytes_transferred << endl;
        return;
    }
    MessageHdr *msg = (MessageHdr *)malloc(packet_sz);
    std::istream is(&buffer);
    msg->size = packet_sz;
    is.read((char*)&(msg->msgType), bytes_transferred);
    print_bytes(msg, msg->size);
    size_t actual = is.gcount();
    if (actual != bytes_transferred) {
        cout << "failed to read the whole packet, actual = " << actual << endl;
        return;
    }
    if (callback) {
        callback(msg);
        return;
    }
    switch(msg->msgType) {
        case JOINREQ: {
            cout << "JOINREQ message received" << endl;
            auto handler = joinreq_handler::create(msg, app, socket);
            handler->start();
        } break;
        case AD: {
            cout << "AD message received" << endl;
        } break;
        default: {
            cout << "Unknown messaged type received" << endl;
        }
    }
}

joinreq_handler::joinreq_handler(MessageHdr *msg, application &app, shared_ptr<tcp::socket> socket)
: app(app), socket(socket) {
    MemberInfo m;
    Serializer::Message::decodeJOINREQ(msg, m.address, m.id, m.ring_id, m.heartbeat);
    m.isAlive = true;
    app.update(m);
}

void joinreq_handler::start() {
    //send an AD
    auto validMembers = app.getValidMembers();
    cout << "begin encode" << endl;
    response = Serializer::Message::allocEncodeAD(validMembers, response_sz);
    cout << "end encode, response_sz = " << response_sz << endl;
    boost::asio::async_write(*socket, boost::asio::buffer(response, response_sz),
                            boost::bind(&joinreq_handler::after_write, shared_from_this()));
}

joinreq_client::joinreq_client(application &app)
: app(app), joinreq_retry(0), timer(app.get_context()) {
    MemberInfo &self = app.self_info();
    uint32_t msgsize;
    msg = Serializer::Message::allocEncodeJOINREQ(
        self.address, self.id, self.ring_id, self.heartbeat, msgsize);
}

void joinreq_client::start() {
    cout << "Trying to join..." << endl;

    //send JOINREQ message to introducer member
    bool success = true;
    try {
        tcp::resolver resolver(app.get_context());
        boost::asio::ip::tcp::endpoint endpoint(
            boost::asio::ip::address(app.get_bootstrap_address().ip), app.get_bootstrap_address().port);
        socket = shared_ptr<tcp::socket>(new tcp::socket(app.get_context()));
        socket->connect(endpoint);
        auto packet_recv = packet_receiver::create(app.get_context(), app, socket);
        packet_recv->set_callback(boost::bind(
            &joinreq_client::handle_response, shared_from_this(),
            boost::placeholders::_1
        ));
        boost::asio::async_write(*socket, boost::asio::buffer(msg, msg->size),
                boost::bind(&joinreq_client::handle_write, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred,
                    packet_recv
                ));
        
        
    } 
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        success = false;
    }
    if (!success) {
        if (joinreq_retry++ < 5) {
            cout << "cannot join the group, retry later ..." << endl;
            timer.expires_from_now(boost::posix_time::seconds(5 * joinreq_retry));
            timer.async_wait(boost::bind(&joinreq_client::start, shared_from_this()));
        }
        else {
            cout << "failed to join the group" << endl;
            exit(1);
        }
    }
    joinreq_retry = 0;
}

void joinreq_client::handle_write(const boost::system::error_code & ec, size_t bytes_transferred,
    packet_receiver::pointer pr) {
    if (ec || bytes_transferred != msg->size) {
        if (joinreq_retry++ < 5) {
            cout << "cannot join the group, retry later ..." << endl;
            timer.expires_from_now(boost::posix_time::seconds(5 * joinreq_retry));
            timer.async_wait(boost::bind(&joinreq_client::start, shared_from_this()));
        }
        else {
            cout << "failed to join the group" << endl;
            exit(1);
        }
    }
    print_bytes(msg, msg->size);
    pr->start();
}

void joinreq_client::handle_response(MessageHdr *msg) {
    printf("heared back from the group!\n");
    app.self_info().isAlive = true;
}