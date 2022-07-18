#include "application.hpp"
#include "protocol.hpp"

/* ------------------------------- packet receiver -------------------------------------*/
int packet_receiver::nextId = 0;
void packet_receiver::start()
{
    ba::async_read(*socket, buffer, ba::transfer_exactly(sizeof(MsgHdr)),
                            bind(&packet_receiver::read_packet, shared_from_this(),
                            ba::placeholders::error,
                            ba::placeholders::bytes_transferred));
}

void packet_receiver::read_packet(const boost::system::error_code & ec, size_t bytes_transferred)
{
    if (ec || bytes_transferred != sizeof(MsgHdr)) {
        log.info("Did not read the num of bytes to transfer, bytes transfered = %d", bytes_transferred);
        if (ec) {
            log.info("%s", ec.message().c_str());
        }
        if (error_handler) {
            error_handler();
        }
        return;
    }
    std::istream is(&buffer);
    is.read((char*)&hdr, sizeof(MsgHdr));

    if(hdr.HEAD != MSG_HEAD) {
        log.critical("message is corrupted");
        exit(1);
    } else {
        log.debug("message head is ok, sz=%d, type=%d", hdr.size, hdr.msgType);
    }

    packet_sz = hdr.size;
    if (packet_sz > DHConst::MaxPacketSize || packet_sz < sizeof(packet_sz)) { 
        log.important("packet size is invalid: %u", packet_sz);
        return;
    }
    ba::async_read(*socket, buffer, ba::transfer_exactly(hdr.payload_size()),
                            bind(&packet_receiver::finish_read, shared_from_this(),
                            ba::placeholders::error,
                            ba::placeholders::bytes_transferred));
}

void packet_receiver::finish_read(const boost::system::error_code & ec, size_t bytes_transferred)
{
    if (ec) {
        log.info("%s", ec.message().c_str());
        if (error_handler) {
            error_handler();
        }
        return;
    }
    if (bytes_transferred != packet_sz - sizeof(MsgHdr)) {
        log.info("did not read the whole packet, packet_sz = %u, transferred = %u", packet_sz, bytes_transferred);
        if (error_handler) {
            error_handler();
        }
        return;
    }
    shared_msg msg = MsgHdr::create_shared(packet_sz);
    std::istream is(&buffer);
    *msg = hdr;
    is.read(msg->payload, bytes_transferred);
    size_t actual = is.gcount();
    if (actual != bytes_transferred) {
        log.info("failed to read the whole packet, actual = %u", actual);
        if (error_handler) {
            error_handler();
        }
        return;
    }
    if (callback) {
        callback(msg);
        return;
    }
}
/* ------------------------------- join -------------------------------------*/

join_handler::join_handler(shared_msg msg, application &app, shared_socket socket)
: app(app), socket(socket) {
    dh_message::JoinRequest joinreq_proto;
    joinreq_proto.ParseFromArray(msg->payload, msg->payload_size());
    MemberInfo m(joinreq_proto.info());
    m.isAlive = true;
    app.update(m, true);
}

void join_handler::start() {
    //send an AD
    auto validMembers = app.getValidMembers();
    response = Serializer::allocEncodeAD(validMembers);
    ba::async_write(*socket, response->buffer(),
                        bind(&join_handler::after_write, shared_from_this()));
}

join_client::join_client(application &app)
: app(app), join_retry(0), timer(app.get_context()) {
    MemberInfo &self = app.self_info();
    dh_message::JoinRequest joinreq_proto;
    *joinreq_proto.mutable_info() = self.toProto();
    msg = Serializer::allocEncode(MsgType::JOIN, joinreq_proto);
}

void join_client::start() {
    app.info("Trying to join...");

    //send JOIN message to introducer member
    bool success = true;
    try {
        ba::ip::tcp::endpoint endpoint(
            ba::ip::address(app.get_bootstrap_address().ip), app.get_bootstrap_address().port);
        socket = shared_socket(new tcp::socket(app.get_context()));
        socket->connect(endpoint);
        auto packet_recv = packet_receiver::create(app, socket, 
            bind(&join_client::handle_response, shared_from_this(),
                 boost::placeholders::_1
        ));
        ba::async_write(*socket, msg->buffer(),
                bind(&join_client::handle_write, shared_from_this(),
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
        if (join_retry++ < DHConst::JoinRetryMax) {
            app.info("cannot join the group, retry later ...");
            timer.expires_from_now(bpt::seconds(DHConst::JoinRetryFactor * join_retry));
            timer.async_wait(bind(&join_client::start, shared_from_this()));
        }
        else {
            app.info("failed to join the group");
            exit(1);
        }
    }
    join_retry = 0;
}

void join_client::handle_write(const boost::system::error_code & ec, size_t bytes_transferred,
    packet_receiver::pointer pr) {
    if (ec || bytes_transferred != msg->size) {
        if (join_retry++ < DHConst::JoinRetryMax) {
            app.info("cannot join the group, retry later ...");
            timer.expires_from_now(bpt::seconds(DHConst::JoinRetryFactor * join_retry));
            timer.async_wait(bind(&join_client::start, shared_from_this()));
        }
        else {
            app.info("failed to join the group");
            exit(1);
        }
    }
    pr->start();
}

void join_client::handle_response(shared_msg msg) {
    app.info("heared back from the group, set self to alive");
    vector<MemberInfo> adlst;
    Serializer::decodeAD(msg, adlst);
    app.self_info().isAlive = true;
    app.update(adlst);
}


/* ------------------------------- ad  -------------------------------------*/
ad_sender::ad_sender(application &app)
    :app(app)
{
    auto validEntries = app.getValidMembers();
    msg = Serializer::allocEncodeAD(validEntries);
}

void ad_sender::start() {
    auto sampled_addr = app.sampleNodes(DHConst::GossipFan);
    for(auto &addr: sampled_addr) {
        async_connect_send(addr);
    }
}
void ad_sender::async_connect_send(const Address &addr) {
    try {
        ba::ip::tcp::endpoint endpoint(ba::ip::address(addr.ip), addr.port);
        auto socket = shared_socket(new tcp::socket(app.get_context()));
        socket->async_connect(endpoint, bind(&ad_sender::async_send, shared_from_this(), socket));
    }
    catch (std::exception& e)
    {
        app.info("%s", e.what());
    }
}
void ad_sender::async_send(shared_socket socket) {
    try {
        ba::async_write(*socket, msg->buffer(), 
            bind(&ad_sender::after_send, shared_from_this(), socket));
    }
    catch (std::exception& e)
    {
        app.info("%s", e.what());
    }
}

void ad_handler::start() {
    vector<MemberInfo> adlst;
    Serializer::decodeAD(msg, adlst);
    app.update(adlst);
}

// -------------------GET-----------------------------
get_handler::get_handler(application &app, shared_msg msg, shared_socket socket)
    :app(app), socket(socket), response_msg(nullptr), msg_to_peers(nullptr),
     responsed_peer_cnt(0), latest_version(0), best_peer_response_idx(0),
     is_success(false) {
    request.ParseFromArray(msg->payload, msg->payload_size());
    app.info("get req: key=%s", request.key().c_str());
}
void get_handler::start() {
    app.debug("handle get request");
    if (request.sender_id() != -1) {
        do_execute();
        return;
    }
    do_coordinate();
}

void get_handler::do_coordinate() {
    int peer_idx = app.map_key_to_node_idx(request.key());
    peers = app.get_group_starting_at(peer_idx, DHConst::ReplicaSize);
    for(auto &p: peers) {
        app.info("route get request to peer id = %d", p.id);
    }

    request.set_sender_id(app.self_info().id);
    msg_to_peers = Serializer::allocEncode(MsgType::GET, request);
    for(int idx = 0; idx < peers.size(); idx++) {
        peer_response.emplace_back();
        try {
            ba::ip::tcp::endpoint endpoint(ba::ip::address(peers[idx].address.ip), peers[idx].address.port);
            auto peer_socket = shared_socket(new tcp::socket(app.get_context()));
            peer_sockets.emplace_back(peer_socket);
            peer_socket->async_connect(endpoint, 
                bind(&get_handler::send_req_to_peer, 
                    shared_from_this(),
                    idx
                ));
        }
        catch (std::exception& e)
        {
            app.info("%s", e.what());
        }
    }
}

void get_handler::send_req_to_peer(int idx) {
    if (!peer_sockets[idx]->is_open()) {
        app.info("failed to connect to peer id = %d", peers[idx].id);
        responsed_peer_cnt++;
        return;
    }
    try {
        auto prc = packet_receiver::create(app, peer_sockets[idx], 
            bind(&get_handler::read_peer_response, shared_from_this(), 
            idx, boost::placeholders::_1));
        prc->set_error_handler(bind(&get_handler::prc_error_handler, shared_from_this(),
            idx));
        
        ba::async_write(*peer_sockets[idx], msg_to_peers->buffer(),
            bind(&get_handler::start_prc, shared_from_this(), prc));
    }
    catch (std::exception& e)
    {
        app.info(e.what());
    }
}

void get_handler::start_prc(packet_receiver::pointer prc)
{
    prc->start();
}

void get_handler::read_peer_response(int idx, shared_msg msg) {
    responsed_peer_cnt++;
    try {
        auto &res = peer_response[idx];
        res.ParseFromArray(msg->payload, msg->payload_size());
        if (res.success() && res.value().version() >= latest_version) {
            is_success = true;
            latest_version = res.value().version();
            best_peer_response_idx = idx;
        }
        if(responsed_peer_cnt == peers.size()) {
            send_response();
        }
    }
    catch (std::exception& e)
    {
        app.info(e.what());
    }
}

void get_handler::prc_error_handler(int idx) {
    app.info("handle prc read error");
    responsed_peer_cnt++;
    if(responsed_peer_cnt == peers.size()) {
        send_response();
    }
}

void get_handler::send_response() {
    if (is_success) {
        response = peer_response[best_peer_response_idx];
    } else {
        response.set_success(false);
    }
    response_msg = Serializer::allocEncode(MsgType::GET_RESPONSE, response);

    auto prc = packet_receiver::create_dispatcher(app, socket);
    ba::async_write(*socket, response_msg->buffer(), 
        bind(&get_handler::start_prc, shared_from_this(), prc)
    );
}

void get_handler::do_execute() {
    app.debug("do GET for %s", request.key().c_str());
    auto val_exist = app.get_store().get(request.key());
    response.set_responder_id(app.self_info().id);
    if (!val_exist.second) { //not ok
        response.set_success(false);
        app.debug("not found key");
    } else {
        auto val = val_exist.first;
        response.set_success(true);
        response.mutable_value()->set_value(val.value);
        response.mutable_value()->set_version(val.version);
        app.debug("found key");
    }
    response_msg = Serializer::allocEncode(MsgType::GET_RESPONSE, response);
    ba::async_write(*socket, response_msg->buffer(),
        bind(&get_handler::after_response, shared_from_this()));
}

void get_handler::after_response() {
    //nothing
}


// -------------------SET-----------------------------

set_handler::set_handler(application &app, shared_msg msg, shared_socket socket)
    :app(app), socket(socket), response_msg(nullptr), request_msg(nullptr),
     responsed_peer_cnt(0), peer_commit_req(nullptr), response_to_cood(nullptr),
     final_response_to_cood(nullptr), final_responsed_peer_cnt(0), is_coordinator(false)  {
    request.ParseFromArray(msg->payload, msg->payload_size());
    app.info("set req: key=%s, val=%s, version=%ld", 
        request.key().c_str(), request.value().value().c_str(), request.value().version());
}
void set_handler::start() {
    app.debug("handle set request");
    if (request.sender_id() != -1) {
        //SET executer
        is_coordinator = false;
        do_execute();
        return;
    }
    is_coordinator = true;
    do_coordinate();
}

void set_handler::do_coordinate() {
    int peer_idx = app.map_key_to_node_idx(request.key());
    peers = app.get_group_starting_at(peer_idx, DHConst::ReplicaSize);
    for(auto &p: peers) {
        app.info("route set request to peer id = %d", p.id);
    }

    request.set_sender_id(app.self_info().id);
    request_msg = Serializer::allocEncode(MsgType::SET, request);
    for(int idx = 0; idx < peers.size(); idx++) {
        try {
            ba::ip::tcp::endpoint endpoint(ba::ip::address(peers[idx].address.ip), peers[idx].address.port);
            auto peer_socket = shared_socket(new tcp::socket(app.get_context()));
            peer_sockets.emplace_back(peer_socket);
            peer_response.emplace_back();
            peer_socket->async_connect(endpoint, 
                bind(&set_handler::send_req_to_peer, shared_from_this(), idx));
        }
        catch (std::exception& e)
        {
            app.info("%s", e.what());
        }
    }
}


///////////////////set handler coordinator///////////////////////

void set_handler::send_req_to_peer(int idx) {
    try {
        app.info("send_req_to_peer, peer id = %d", peers[idx].id);
        auto prc = packet_receiver::create(app, peer_sockets[idx], 
            bind(&set_handler::handle_peer_response, shared_from_this(),
                boost::placeholders::_1, idx));
        prc->set_error_handler(bind(&set_handler::prc_error_handler, shared_from_this()));
        ba::async_write(*peer_sockets[idx], request_msg->buffer(),
            bind(&set_handler::start_prc, shared_from_this(), prc)
        );
    }
    catch (std::exception& e)
    {
        app.info("send_req_to_peer, peer id = %d: %s", peers[idx].id, e.what());
    }
}

void set_handler::handle_peer_response(shared_msg res_msg, int idx) {
    responsed_peer_cnt++;
    app.info("handle_peer_response, peer id = %d", peers[idx].id);
    dh_message::SingleIntMessage bmsg;
    bmsg.ParseFromArray(res_msg->payload, res_msg->payload_size());

    peer_response[idx] = bmsg.val();
    if (responsed_peer_cnt == peers.size()) {
        dh_message::SingleIntMessage bmsg;
        bmsg.set_val(true);
        for(int i = 0; i < peers.size(); i++) {
            if (!peer_response[i]) {
                app.info("decided to not commit");
                bmsg.set_val(false);
                break;
            }
        }
        peer_commit_req = Serializer::allocEncode(MsgType::DUMMY_START, bmsg);
        for(int idx = 0; idx < peers.size(); idx++) {
            ba::async_write(*peer_sockets[idx], peer_commit_req->buffer(),
                bind(&set_handler::after_commit_to_peer, shared_from_this(), idx));
        }
    }
}
void set_handler::after_commit_to_peer(int idx) {
    final_responsed_peer_cnt++;
    app.info("after_commit_to_peer, peer id = %d, final_responsed_peer_cnt=%d", peers[idx].id, final_responsed_peer_cnt);
    if (final_responsed_peer_cnt == peers.size()) {
        send_response();
    }
}

void set_handler::send_response() {
    int ok = 0, failed = 0;
    for(int i = 0; i < peers.size(); i++) {
        if (peer_response[i]) {
            ok++;
        } else {
            failed++;
        }
    }
    app.info("finish SET, ok=%d, failed=%d", ok, failed);
    bool res = ok > 0;
    response.set_success(res);
    response_msg = Serializer::allocEncode(MsgType::SET_RESPONSE, response);
    app.debug("response msg = %p, sz = %d", response_msg, response_msg->size);

    ba::async_write(*socket, response_msg->buffer(),
        bind(&set_handler::start_prc, shared_from_this(), 
            packet_receiver::create_dispatcher(app, socket)) 
    );
    //continue to handle client's upcoming requests
}
void set_handler::prc_error_handler() {
    responsed_peer_cnt++;
    final_responsed_peer_cnt++;
    if (final_responsed_peer_cnt == peers.size()) {
        send_response();
    }
}

////////////////////////set handler execute//////////////////////////////

void set_handler::do_execute() {
    app.info("be selected as the storage node");
    bool ok = true;
    if (app.get_store().lock(request.key())) {
        app.debug("got the lock for %s", request.key().c_str());
        ok = true;
    } else {
        app.debug("unable to get lock for %s" , request.key().c_str());
        ok = false;
    }
    dh_message::SingleIntMessage b;
    b.set_val(ok);
    response_to_cood = Serializer::allocEncode(MsgType::DUMMY_START, b);

    auto prc = packet_receiver::create(app, socket, 
        bind(&set_handler::handle_commit, shared_from_this(), boost::placeholders::_1));
    ba::async_write(*socket, response_to_cood->buffer(),
        bind(&set_handler::start_prc, shared_from_this(), prc));
}

void set_handler::handle_commit(shared_msg commit_msg) {
    dh_message::SingleIntMessage b;
    b.ParseFromArray(commit_msg->payload, commit_msg->payload_size());
    bool commit_from_cood = b.val();
    bool final_ok;
    app.debug("got commit message = %d", (int)commit_from_cood);
    if (commit_from_cood && app.get_store().check_lock(request.key())) {
        app.info("SET %s to %s", request.key().c_str(), request.value().value().c_str());
        app.get_store().set(request.key(), request.value());
        app.info("release lock for %s", request.key().c_str());
        auto check = app.get_store().get(request.key());
        final_ok = true;
    } else {
        app.info("SET %s ABORTED", request.key().c_str());
        final_ok = false;
    }

    if (app.get_store().check_lock(request.key())) {
        app.info("release lock for %s", request.key().c_str());
        app.get_store().release(request.key());
    }
    
    b.set_val(final_ok);
    final_response_to_cood = Serializer::allocEncode(MsgType::DUMMY_START, b);
    ba::async_write(*socket, final_response_to_cood->buffer(),
        bind(&set_handler::after_final_response, shared_from_this()));
}

void set_handler::after_final_response() {
    //nothing
}