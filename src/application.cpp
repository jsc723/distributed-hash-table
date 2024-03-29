#include "headers.hpp"
#include "application.hpp"
#include "protocol.hpp"
#include "serializer.hpp"


application::application(boost::asio::io_context &io_context, int id, string ip, unsigned short port, int ring_id,
                        string bootstrap_ip, unsigned short bootstrap_port)
        : io_context(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
{
    is_bootstrap_server = bootstrap_ip == BOOTSTRAP_IP && bootstrap_port == BOOTSTRAP_PORT;
    boost::system::error_code ec;
    Address address;
    address.ip = boost::asio::ip::make_address_v4(ip, ec);
    if (ec)
    {
        logger.log(LogLevel::CRITICAL, "", "unable to resolve ip address");
        exit(1);
    }
    address.port = port;

    bootstrap_address.ip = boost::asio::ip::make_address_v4(bootstrap_ip, ec);
    bootstrap_address.port = bootstrap_port;
    MemberInfo self(address, id, ring_id);
    members.emplace_back(self);
    self_index = 0;
    logger.log(LogLevel::DEBUG, "", "app ctor done");
}

void application::init() {
    info("---Start application---");
    join_cluster();
    // ----------------------------- repeating tasks ------------------------------//

    add_repeating_task(bind(&application::heartbeat_loop, this), 
        bpt::seconds(DHConst::HeartBeatInterval));

    add_repeating_task(bind(&application::ad_loop, this),
        bpt::seconds(DHConst::GossipInterval));

    add_repeating_task(bind(&application::push_loop, this),
        bpt::seconds(DHConst::PushInterval));

    add_repeating_task(bind(&application::check_member_loop, this),
        bpt::seconds(DHConst::CheckMemberInterval));

    // -----------------------------------------------------------------------------//
    start_accept();
}

void application::start_accept()
{
    auto socket = shared_socket(new tcp::socket(io_context));
    auto prc = packet_receiver::create_dispatcher(*this, socket);

    acceptor_.async_accept(*socket, boost::bind(&application::handle_accept, this, prc,
                                        boost::asio::placeholders::error));

}

void application::handle_accept(packet_receiver::pointer packet_recv,
                    const boost::system::error_code &error)
{
    if (!error)
    {
        info("Handle new request");
        packet_recv->start();
    }
    start_accept();
}

void application::dispatch_packet(shared_socket socket, shared_msg msg) {
    switch(msg->msgType) {
        case MsgType::JOIN: {
            info("JOIN message received");
            auto handler = join_handler::create(msg, *this, socket);
            handler->start();
        } break;

        case MsgType::AD: {
            info("AD message received");
            auto handler = ad_handler::create(*this, msg);
            handler->start();
        } break;
        
        case MsgType::GET: {
            info("GET message received");
            auto handler = get_handler::create(*this, msg, socket);
            handler->start();
        } break;

        case MsgType::SET: {
            info("SET message received");
            auto handler = set_handler::create(*this, msg, socket);
            handler->start();
        } break;

        case MsgType::PUSH: {
            info("PUSH message received");
            auto handler = push_handler::create(*this, msg);
            handler->start();
        } break;

        default: {
            info("Unknown messaged type received");
        }
    }
}

void application::update_self() {
    self_info().heartbeat++;
    self_info().timestamp = get_local_time();
}

void application::update(const MemberInfo &info, bool forced) {
    if (forced) {
        debug("Forced update for join req");
    }
    for(auto &e: members) {
        if (e == info) {
            if ((e.isValid() && e.heartbeat < info.heartbeat )|| forced) {
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
    info("Add node: %d, Port: %hu", member.id, member.address.port);
    auto self = self_info();
    members.emplace_back(member);
    sort(members.begin(), members.end(), [](const MemberInfo &p, const MemberInfo &q) {
        if (p.ring_id == q.ring_id) {
            return p.id < q.id;
        }
        return p.ring_id < q.ring_id;
    });
    for(int i = 0; i < members.size(); i++) {
        if (members[i] == self) {
            self_index = i;
            break;
        }
    }
    return true;
}

vector<MemberInfo> application::getValidMembers() 
{
    vector<MemberInfo> validNodes;
    for(auto &e: members) {
        if (e.isValid()) {
            validNodes.emplace_back(e);
        }
    }
    return validNodes;
}

vector<Address> application::sampleNodes(int maxCount) {
    vector<MemberInfo> validNodes;
    for(auto &e: members) {
        if (e == self_info()) {
            continue;
        }
        if (e.isValid()) {
            validNodes.emplace_back(e);
        }
    }
    maxCount = std::min<int>(maxCount, validNodes.size());
    vector<MemberInfo> shuffled(validNodes);
    std::random_shuffle(shuffled.begin(), shuffled.end());
    vector<Address> sampled(maxCount);
    for(int i = 0; i < maxCount; i++) {
        sampled[i] = shuffled[i].address;
    }
    return sampled;
}


void application::join_cluster() {
    MemberInfo &self = self_info();
    if (is_bootstrap_server)
    {
        info("Starting up group");
        self.isAlive = true;
    }
    else
    {
        //add to group
        auto jc = join_client::create(*this);
        jc->start();
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
        info("%s", ec.message().c_str());
        goto end_template_loop;
    }
    if (!self.isAlive) {
        info("Waiting to become alive...");
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
}

void application::ad_loop() {
    debug("Send AD");
    auto sender = ad_sender::create(*this);
    sender->start();
}

void application::push_loop() {
    debug("Send PUSH");
    auto sender = push_sender::create(*this);
    sender->start();
}

void application::check_member_loop() {
    vector<MemberInfo> updated;
    int next_self_index;
    for(size_t i = 0; i < members.size(); i++) {
        auto &e = members[i];
        if (i == self_index) {
            next_self_index = updated.size();
            updated.emplace_back(e);
            continue;
        }

        if (!e.isValid() && e.isAlive) {
            e.isAlive = false;
            info("node %d left the group", self_info().id, e.id);
        }
        if (!e.shouldRemoved()) {
            updated.emplace_back(e);
        }
    }
    members = move(updated);
    self_index = next_self_index;
}

//--------------------------------------------------------

int application::map_key_to_node_idx(const data_store::key_t &key) {
    int ring_pos = md5_mod(key);
    size_t best_node_idx = 0;
    int min_diff = INT_MAX;
    for(size_t i = 0; i < members.size(); i++) {
        if (!members[i].isValid()) {
            continue;
        }
        int ring_id = members[i].ring_id;
        if (ring_id < ring_pos) {
            ring_id += DHConst::RingSize;
        }
        if (ring_id - ring_pos < min_diff) {
            min_diff = ring_id - ring_pos;
            best_node_idx = i;
        }
    }
    debug("key=%s, ring_pos=%d, ring_id=%d, id=%d", key.c_str(), ring_pos, 
        members[best_node_idx].ring_id, members[best_node_idx].id);
    return best_node_idx;
}

vector<MemberInfo> application::get_group_starting_at(int idx, int max_size) {
    debug("start get_group_starting_at");
    vector<MemberInfo> group;
    int i = idx;
    do {
        if (members[i].isValid()) {
            group.emplace_back(members[i]);
        }
        i++;
        if (i >= members.size()) {
            i = 0;
        }
    } while(i != idx && group.size() < max_size);
    debug("end get_group_starting_at");
    return group;
}

bool application::self_is_responsible_for_key(const string &key) {
    auto start = map_key_to_node_idx(key);
    if (self_index >= start && self_index < start + DHConst::ReplicaSize) {
        return true;
    }
    int extra = start + DHConst::ReplicaSize - members.size();
    if (extra > 0 && self_index < extra) {
        return true;
    }
    return false;
}