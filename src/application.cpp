#include "headers.hpp"
#include "application.hpp"
#include "serializer.hpp"


application::application(boost::asio::io_context &io_context, int id, unsigned short port, int ring_id)
        : io_context(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
        timer(io_context, boost::posix_time::seconds(1))
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

vector<MemberInfo> application::getValidMembers() 
{
    vector<MemberInfo> validNodes;
    for(auto &e: members) {
        if (memberListEntryIsValid(e)) {
            validNodes.emplace_back(e);
        }
    }
    return validNodes;
}

vector<Address> application::sampleNodes(int k) {
    vector<MemberInfo> validNodes;
    for(auto &e: members) {
        if (e == self_info()) {
            continue;
        }
        if (memberListEntryIsValid(e)) {
            validNodes.emplace_back(e);
        }
    }
    k = std::min<int>(k, validNodes.size());
    vector<MemberInfo> shuffled(validNodes);
    std::random_shuffle(shuffled.begin(), shuffled.end());
    vector<Address> sampled(k);
    for(int i = 0; i < k; i++) {
        sampled[i] = shuffled[i].address;
    }
    return sampled;
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
        auto joinreq = joinreq_client::create(*this);
        joinreq->start();
        // uint32_t msgsize;
        // MessageHdr *msg = Serializer::Message::allocEncodeJOINREQ(
        //     self.address, self.id, self.ring_id, self.heartbeat, msgsize);

        // cout << "Trying to join..." << endl;

        // send JOINREQ message to introducer member
        // bool success = true;
        // try {
        //     tcp::resolver resolver(io_context);
        //     boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address(bootstrap_address.ip), bootstrap_address.port);
        //     tcp::socket socket(io_context);
        //     socket.connect(endpoint);
        //     boost::asio::write(socket, boost::asio::buffer(msg, msg->size));
        //     print_bytes(msg, msg->size);
        // } 
        // catch (std::exception& e)
        // {
        //     std::cerr << e.what() << std::endl;
        //     success = false;
        // }
        // Serializer::Message::dealloc(msg);

        // if (!success) {
        //     if (joinreq_retry++ < 5) {
        //         cout << "cannot join the group, retry later ..." << endl;
        //         boost::this_thread::sleep_for(boost::chrono::seconds(5 * joinreq_retry));
        //         introduce_self_to_group();
        //     }
        //     else {
        //         cout << "failed to join the group" << endl;
        //         exit(1);
        //     }
        // }
        // cout << "join request is sent" << endl;
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