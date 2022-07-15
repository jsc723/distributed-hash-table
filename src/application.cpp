#include "headers.hpp"
#include "application.hpp"
#include "serializer.hpp"

int workflow::next_id = 0;

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

    memberNode = boost::shared_ptr<MemberInfo>(new MemberInfo(address, id, ring_id));

    introduce_self_to_group();
    timer.async_wait(boost::bind(&application::main_loop, this,
                boost::asio::placeholders::error));
    start_accept();
}

void application::start_accept()
{
    std::cout << "start_accept ---" << std::endl;
    workflow::pointer new_connection =
        workflow::create(acceptor_.get_executor().context());

    acceptor_.async_accept(new_connection->socket(),
                            boost::bind(&application::handle_accept, this, new_connection,
                                        boost::asio::placeholders::error));

    std::cout << "--- start_accept" << std::endl;
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

void application::introduce_self_to_group() {
    if (memberNode->address == bootstrap_address)
    {
        std::cout << "starting up group" << std::endl;
        memberNode->isAlive = true;
        add_node(memberNode);
    }
    else
    {
        //add to group
        size_t msgsize;
        MessageHdr *msg = Serializer::Message::allocEncodeJOINREQ(
            memberNode->address, memberNode->id, memberNode->ring_id, memberNode->heartbeat, msgsize);

        cout << "Trying to join..." << endl;

        // send JOINREQ message to introducer member
        //emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);
        bool success = true;
        try {
            tcp::resolver resolver(io_context);
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address(bootstrap_address.ip), bootstrap_address.port);
            tcp::socket socket(io_context);
            socket.connect(endpoint);

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
        } else {
            cout << "failed to join the group" << endl;
            exit(1);
        }
    }
}

void application::main_loop(const boost::system::error_code& ec) {
    //TODO
    memberNode->heartbeat++;
}