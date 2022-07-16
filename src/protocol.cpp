#include "application.hpp"
#include "protocol.hpp"

/* ------------------------------- packet receiver -------------------------------------*/
int packet_receiver::nextId = 0;
void packet_receiver::start()
{
    ba::async_read(*socket, buffer, ba::transfer_exactly(sizeof(uint32_t)),
                            bind(&packet_receiver::read_packet, shared_from_this(),
                            ba::placeholders::error,
                            ba::placeholders::bytes_transferred));
}

void packet_receiver::read_packet(const boost::system::error_code & ec, size_t bytes_transferred)
{
    if (ec || bytes_transferred != sizeof(uint32_t)) {
        log.info("Did not read the num of bytes to transfer, bytes transfered = %d", bytes_transferred);
        if (ec) {
            log.info("error: %s", ec.message().c_str());
        }
        return;
    }
    std::istream is(&buffer);
    is.read((char*)&packet_sz, sizeof(packet_sz));
    if (packet_sz > MAX_PACKET_SZ || packet_sz < sizeof(packet_sz)) { 
        log.important("packet size is invalid: %u", packet_sz);
        return;
    }
    ba::async_read(*socket, buffer, ba::transfer_exactly(packet_sz - sizeof(packet_sz)),
                            bind(&packet_receiver::finish_read, shared_from_this(),
                            ba::placeholders::error,
                            ba::placeholders::bytes_transferred));
}

void packet_receiver::finish_read(const boost::system::error_code & ec, size_t bytes_transferred)
{
    if (ec) {
        log.info("error: %s", ec.message().c_str());
        return;
    }
    if (bytes_transferred != packet_sz - sizeof(packet_sz)) {
        log.info("did not read the whole packet, packet_sz = %u, transferred = %u", packet_sz, bytes_transferred);
        return;
    }
    MessageHdr *msg = (MessageHdr *)malloc(packet_sz);
    std::istream is(&buffer);
    msg->size = packet_sz;
    is.read((char*)&(msg->msgType), bytes_transferred);
    size_t actual = is.gcount();
    if (actual != bytes_transferred) {
        log.info("failed to read the whole packet, actual = %u", actual);
        return;
    }
    if (callback) {
        callback(msg);
        return;
    }
}
/* ------------------------------- joinreq -------------------------------------*/

joinreq_handler::joinreq_handler(MessageHdr *msg, application &app, shared_ptr<tcp::socket> socket)
: app(app), socket(socket) {
    MemberInfo m;
    Serializer::Message::decodeJOINREQ(msg, m.address, m.id, m.ring_id, m.heartbeat);
    m.isAlive = true;
    app.update(m, true);
}

void joinreq_handler::start() {
    //send an AD
    auto validMembers = app.getValidMembers();
    response = Serializer::Message::allocEncodeAD(validMembers);
    ba::async_write(*socket, ba::buffer(response, response->size),
                            bind(&joinreq_handler::after_write, shared_from_this()));
}

joinreq_client::joinreq_client(application &app)
: app(app), joinreq_retry(0), timer(app.get_context()) {
    MemberInfo &self = app.self_info();
    uint32_t msgsize;
    msg = Serializer::Message::allocEncodeJOINREQ(
        self.address, self.id, self.ring_id, self.heartbeat, msgsize);
}

void joinreq_client::start() {
    app.info("Trying to join...");

    //send JOINREQ message to introducer member
    bool success = true;
    try {
        ba::ip::tcp::endpoint endpoint(
            ba::ip::address(app.get_bootstrap_address().ip), app.get_bootstrap_address().port);
        socket = shared_ptr<tcp::socket>(new tcp::socket(app.get_context()));
        socket->connect(endpoint);
        auto packet_recv = packet_receiver::create(app, socket, 
            bind(&joinreq_client::handle_response, shared_from_this(),
                 boost::placeholders::_1
        ));
        ba::async_write(*socket, ba::buffer(msg, msg->size),
                bind(&joinreq_client::handle_write, shared_from_this(),
                    ba::placeholders::error,
                    ba::placeholders::bytes_transferred,
                    packet_recv
                ));
    } 
    catch (std::exception& e)
    {
        app.info(e.what());
        success = false;
    }
    if (!success) {
        if (joinreq_retry++ < MyConst::joinreq_retry_max) {
            app.info("cannot join the group, retry later ...");
            timer.expires_from_now(bpt::seconds(MyConst::joinreq_retry_factor * joinreq_retry));
            timer.async_wait(bind(&joinreq_client::start, shared_from_this()));
        }
        else {
            app.info("failed to join the group");
            exit(1);
        }
    }
    joinreq_retry = 0;
}

void joinreq_client::handle_write(const boost::system::error_code & ec, size_t bytes_transferred,
    packet_receiver::pointer pr) {
    if (ec || bytes_transferred != msg->size) {
        if (joinreq_retry++ < MyConst::joinreq_retry_max) {
            app.info("cannot join the group, retry later ...");
            timer.expires_from_now(bpt::seconds(MyConst::joinreq_retry_factor * joinreq_retry));
            timer.async_wait(bind(&joinreq_client::start, shared_from_this()));
        }
        else {
            app.info("failed to join the group");
            exit(1);
        }
    }
    pr->start();
}

void joinreq_client::handle_response(MessageHdr *msg) {
    app.info("heared back from the group, set self to alive");
    vector<MemberInfo> adlst;
    Serializer::Message::decodeAD(msg, adlst);
    app.self_info().isAlive = true;
    app.update(adlst);
}


/* ------------------------------- ad  -------------------------------------*/
ad_sender::ad_sender(application &app)
    :app(app)
{
    auto validEntries = app.getValidMembers();
    msg = Serializer::Message::allocEncodeAD(validEntries);
}

void ad_sender::start() {
    auto sampled_addr = app.sampleNodes(MyConst::GossipFan);
    for(auto &addr: sampled_addr) {
        app.info("send ad to node port = %d", addr.port);
        async_connect_send(addr);
    }
}
void ad_sender::async_connect_send(const Address &addr) {
    try {
        ba::ip::tcp::endpoint endpoint(ba::ip::address(addr.ip), addr.port);
        auto socket = shared_ptr<tcp::socket>(new tcp::socket(app.get_context()));
        socket->async_connect(endpoint, bind(&ad_sender::async_send, shared_from_this(), socket));
    }
    catch (std::exception& e)
    {
        app.info("%s", e.what());
    }
}
void ad_sender::async_send(shared_ptr<tcp::socket> socket) {
    try {
        ba::async_write(*socket, ba::buffer(msg, msg->size), 
            bind(&ad_sender::after_send, shared_from_this(), socket));
    }
    catch (std::exception& e)
    {
        app.info("%s", e.what());
    }
}

void ad_handler::start() {
    vector<MemberInfo> adlst;
    Serializer::Message::decodeAD(msg, adlst);
    app.update(adlst);
}