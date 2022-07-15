#include "application.hpp"


int workflow::next_id = 0;

application::application(boost::asio::io_context &io_context, int id, unsigned short port, int ring_id)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
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

    memberNode = boost::shared_ptr<MemberInfo>(new MemberInfo(address, id, ring_id));

    introduce_self_to_group();
    start_accept();
}

bool application::add_node(boost::shared_ptr<MemberInfo> member) {
    auto it = std::find_if(members.begin(), members.end(),
        [member](const shared_ptr<MemberInfo> p) {
            return *p == *member;
        });
    if (it == members.end()) {
        members.emplace_back(member);
        return true;
    }
    return false;
}