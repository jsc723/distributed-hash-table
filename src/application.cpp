#include "headers.hpp"
#include "application.hpp"
#include "protocol.hpp"
#include "serializer.hpp"


application::application(boost::asio::io_context &io_context, int id, unsigned short port, int ring_id)
        : io_context(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
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

    // ----------------------------- repeating tasks ------------------------------//

    add_repeating_task(boost::bind(&application::heartbeat_loop, this), 
        boost::posix_time::seconds(MyConst::heartbeatInterval));

    add_repeating_task(bind(&application::ad_loop, this),
        boost::posix_time::seconds(MyConst::GossipInterval));

    // -----------------------------------------------------------------------------//
    start_accept();
}

void application::start_accept()
{
    std::cout << "start_accept ---" << std::endl;
    packet_receiver::pointer new_connection = packet_receiver::create(*this);

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

void application::update_self() {
    self_info().heartbeat++;
    self_info().timestamp = get_local_time();
}

void application::update(const MemberInfo &info) {
    for(auto &e: members) {
        if (e == info) {
            if (memberListEntryIsValid(e) && e.heartbeat < info.heartbeat) {
                e.heartbeat = info.heartbeat;
                e.timestamp = get_local_time();
                printf("update node %d, heartbeat = %d\n", e.id, e.heartbeat);
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
    }
}

void application::add_repeating_task(task_callback_t callback, duration_t interval) {
    timers.emplace_back(get_context(), interval);
    int timer_idx = timers.size() - 1;
    timers[timer_idx].async_wait(boost::bind(&application::repeating_task_template,
        this, boost::asio::placeholders::error, timer_idx, callback, interval));
}

void application::repeating_task_template(const boost::system::error_code& ec, int timer_idx, 
                                        task_callback_t callback, duration_t interval) {
    MemberInfo &self = self_info();
    if (ec) {
        cout << ec.message() << endl;
        goto end_template_loop;
    }
    if (!self.isAlive) {
        cout << "waiting to become alive" << endl;
        goto end_template_loop;
    }

    callback();

end_template_loop:
    timers[timer_idx].expires_at(timers[timer_idx].expires_at() + interval);
    timers[timer_idx].async_wait(boost::bind(&application::repeating_task_template,
        this, boost::asio::placeholders::error, timer_idx, callback, interval));
}

void application::heartbeat_loop() {
    MemberInfo &self = self_info();
    update_self();
    cout << "heartbeat: " << self.heartbeat << endl;
}

void application::ad_loop() {
    cout << "send ad" << endl;
    auto sender = ad_sender::create(*this);
    sender->start();
}