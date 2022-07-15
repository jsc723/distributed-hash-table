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
        cout << "did not read the num of bytes to transfer" << endl;
        return;
    }
    buffer.commit(bytes_transferred);
    string packet_sz_s;
    std::istream is(&buffer);
    is >> packet_sz_s;
    Mem::read((char *)packet_sz_s.c_str(), packet_sz);
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
    buffer.commit(bytes_transferred);
    MessageHdr *msg = (MessageHdr *)malloc(packet_sz);
    std::istream is(&buffer);
    msg->size = packet_sz;
    is.read((char*)&(msg->msgType), bytes_transferred);
    size_t actual = is.gcount();
    if (actual != bytes_transferred) {
        cout << "failed to read the whole packet, actual = " << actual << endl;
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
