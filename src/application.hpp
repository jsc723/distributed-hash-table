#pragma once 
#include "headers.hpp"
#include "utils.hpp"

using boost::asio::ip::tcp;
using std::string;
using std::vector;
using boost::shared_ptr;

class workflow
    : public boost::enable_shared_from_this<workflow>
{
public:
    static int next_id;
    int id;
    typedef boost::shared_ptr<workflow> pointer;

    static pointer create(boost::asio::io_context &io_context)
    {
        auto conn = new workflow(io_context);
        std::cout << "tcp connection is created" << conn->id << std::endl;
        return pointer(conn);
    }

    tcp::socket &socket()
    {
        return socket_;
    }

    void start()
    {
        message_ = make_daytime_string();

        boost::asio::async_write(socket_, boost::asio::buffer(message_),
                                boost::bind(&workflow::handle_write, shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
    }

    ~workflow()
    {
        std::cout << "tcp connect " << id << " freed" << std::endl;
    }

private:
    workflow(boost::asio::io_context &io_context)
        : socket_(io_context), id(next_id++)
    {
    }

    void handle_write(const boost::system::error_code & /*error*/,
                      size_t /*bytes_transferred*/)
    {
        std::cout << "response is written to the client" << std::endl;
    }

    tcp::socket socket_;
    std::string message_;
};


class application
{
public:
    application(boost::asio::io_context &io_context, int id, unsigned short port, int ring_id);

private:

    void introduce_self_to_group();

    bool add_node(boost::shared_ptr<MemberInfo> member);

    void start_accept();

    void handle_accept(workflow::pointer new_connection,
                       const boost::system::error_code &error)
    {
        std::cout << "handle_accept --- " << std::endl;
        if (!error)
        {
            std::cout << new_connection->id << " handles request " << std::endl;
            new_connection->start();
        }

        start_accept();
        std::cout << "--- handle_accept" << std::endl;
    }

    void main_loop(const boost::system::error_code& ec);

    boost::asio::io_context &io_context;
    Address bootstrap_address;
    tcp::acceptor acceptor_;
    boost::shared_ptr<MemberInfo> memberNode;
    vector<boost::shared_ptr<MemberInfo>> members;
    boost::asio::deadline_timer timer;
    int joinreq_retry;
};
