#include "headers.hpp"
#include "application.hpp"
#include "serializer.hpp"

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
    buffer.commit(sizeof(uint32_t));
    string packet_sz_s;
    std::istream is(&buffer);
    is >> packet_sz_s;
    Mem::read((char *)packet_sz_s.c_str(), packet_sz);
    cout << "packet sz is " << packet_sz << endl;
    if (packet_sz > MAX_PACKET_SZ) {
        cout << "packet is too large" << endl;
        return;
    }
    boost::asio::async_read(*socket, buffer, boost::asio::transfer_exactly(packet_sz),
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
    if (bytes_transferred != packet_sz) {
        cout << "did not read the whole packet, packet_sz = " << packet_sz
                << ", transferred = " << bytes_transferred << endl;
        return;
    }
    buffer.commit(packet_sz);
    string content;
    std::istream is(&buffer);
    is >> content;
    MessageHdr *msg = (MessageHdr *)content.c_str();
    switch(msg->msgType) {
        case JOINREQ: {
            cout << "JOINREQ message received" << endl;
            MemberInfo m;
            Serializer::Message::decodeJOINREQ(msg, m.address, m.id, m.ring_id, m.heartbeat);
            m.isAlive = true;
            app_.update(m);
            
        } break;
        default: {
            cout << "Unknown messaged type received" << endl;
        }
    }
}

application::application(boost::asio::io_context &io_context, int id, unsigned short port, int ring_id)
        : io_context(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
        joinreq_retry(0), timer(io_context, boost::posix_time::seconds(1))
{
    boost::system::error_code ec;
    Address address;
    address.ip = boost::asio::ip::make_address_v4(LOCALHOST, ec);
    if (ec)
    {
        std::cerr << "unable to resolve ip address" << std::endl;
        exit(1);
    }
    address.port = port;

    bootstrap_address.ip = boost::asio::ip::make_address_v4(LOCALHOST, ec);
    bootstrap_address.port = BOOTSTRAP_PORT;

    MemberInfo self(address, id, ring_id);
    members.emplace_back(self);

    introduce_self_to_group();
    timer.async_wait(boost::bind(&application::main_loop, this,
                boost::asio::placeholders::error));
    start_accept();
}

void application::start_accept()
{
    std::cout << "start_accept ---" << std::endl;
    packet_receiver::pointer new_connection =
        packet_receiver::create(io_context, *this);

    acceptor_.async_accept(*new_connection->get_socket(),
                            boost::bind(&application::handle_accept, this, new_connection,
                                        boost::asio::placeholders::error));

    std::cout << "--- start_accept" << std::endl;
}

void application::handle_accept(packet_receiver::pointer new_connection,
                    const boost::system::error_code &error)
{
    std::cout << "handle_accept --- " << std::endl;
    if (!error)
    {
        std::cout << " handles request " << std::endl;
        new_connection->start();
    }

    start_accept();
    std::cout << "--- handle_accept" << std::endl;
}

void application::update(const MemberInfo &info) {
    for(auto &e: members) {
        if (e == info) {
            if (memberListEntryIsValid(e) && e.heartbeat < info.heartbeat) {
                e.heartbeat = info.heartbeat;
                e.timestamp = get_local_time();
            }
            return;
        }
    }
    add_node(info);
}

void application::update(const vector<MemberInfo> &info_list) {
    for(auto &e: info_list) {
        update(e);
    }
}

bool application::add_node(const MemberInfo &member) {
    cout << "add node: " << member.id  << ", port: " << member.address.port << endl;
    members.emplace_back(member);
    return true;
}

void application::introduce_self_to_group() {
    MemberInfo &self = self_info();
    if (self.address == bootstrap_address)
    {
        std::cout << "starting up group" << std::endl;
        self.isAlive = true;
    }
    else
    {
        //add to group
        uint32_t msgsize;
        MessageHdr *msg = Serializer::Message::allocEncodeJOINREQ(
            self.address, self.id, self.ring_id, self.heartbeat, msgsize);

        cout << "Trying to join..." << endl;

        // send JOINREQ message to introducer member
        //emulNet->ENsend(&self.addr, joinaddr, (char *)msg, msgsize);
        bool success = true;
        try {
            tcp::resolver resolver(io_context);
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address(bootstrap_address.ip), bootstrap_address.port);
            tcp::socket socket(io_context);
            socket.connect(endpoint);
            boost::asio::write(socket, boost::asio::buffer(&msgsize, sizeof(msgsize)));
            boost::asio::write(socket, boost::asio::buffer(msg, msgsize));
        } 
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            success = false;
        }
        Serializer::Message::dealloc(msg);

        if (!success) {
            if (joinreq_retry++ < 5) {
                cout << "cannot join the group, retry later ..." << endl;
                boost::this_thread::sleep_for(boost::chrono::seconds(5 * joinreq_retry));
                introduce_self_to_group();
            }
            else {
                cout << "failed to join the group" << endl;
                exit(1);
            }
        }
        cout << "join request is sent" << endl;
    }
}

void application::main_loop(const boost::system::error_code& ec) {
    //TODO
    MemberInfo &self = self_info();
    if (!self.isAlive) {
        cout << "waiting to become alive" << endl;
        goto end_main_loop;
    }
    self.heartbeat++;
    cout << "heartbeat: " << self.heartbeat << endl;

end_main_loop:
    timer.expires_at(timer.expires_at() + boost::posix_time::seconds(2));
    timer.async_wait(boost::bind(&application::main_loop, this,
                boost::asio::placeholders::error));
}