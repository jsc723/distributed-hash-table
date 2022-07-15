#include "headers.hpp"

using boost::asio::ip::tcp;
#define BOOTSTRAP_PORT 9000

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}


class workflow
  : public boost::enable_shared_from_this<workflow>
{
public:
    static int next_id;
    int id;
  typedef boost::shared_ptr<workflow> pointer;

  static pointer create(boost::asio::io_context& io_context)
  {
    auto conn = new workflow(io_context);
    std::cout << "tcp connection is created" << conn->id << std::endl;
    return pointer(conn);
  }

  tcp::socket& socket()
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

  ~workflow() {
    std::cout << "tcp connect " << id << " freed" << std::endl;
  }

private:
  workflow(boost::asio::io_context& io_context)
    : socket_(io_context), id(next_id++)
  {
  }

  void handle_write(const boost::system::error_code& /*error*/,
      size_t /*bytes_transferred*/)
  {
    std::cout << "response is written to the client" << std::endl;
  }


  tcp::socket socket_;
  std::string message_;
};
int workflow::next_id = 0;

class application
{
public:
  application(boost::asio::io_context& io_context, unsigned short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    start_accept();
  }

private:
  void start_accept()
  {
    std::cout << "start_accept ---" << std::endl;
    workflow::pointer new_connection =
      workflow::create(acceptor_.get_executor().context());

    acceptor_.async_accept(new_connection->socket(),
        boost::bind(&application::handle_accept, this, new_connection,
          boost::asio::placeholders::error));

    std::cout << "--- start_accept" << std::endl;
  }

  void handle_accept(workflow::pointer new_connection,
      const boost::system::error_code& error)
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

  tcp::acceptor acceptor_;
};


class printer {
public:
    printer(boost::asio::io_context &io)
    : timer_(io, boost::posix_time::seconds(1)), 
      counter_(0) {
        timer_.async_wait(boost::bind(&printer::print, this,
                boost::asio::placeholders::error));
    }
    void print(const boost::system::error_code& /*e*/)
    {
        if (true)
        {
            std::cout << "Server run time: " << counter_ << std::endl;
            ++(counter_);

            timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(5));
            timer_.async_wait(boost::bind(&printer::print, this,
                boost::asio::placeholders::error));
        }
    }
private:
    int counter_;
    boost::asio::deadline_timer timer_;
    
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2) {
        std::cerr << "Usage: dh <port>" << std::endl;
        return 1;
    }
    unsigned short port = boost::lexical_cast<unsigned short>(argv[1]);
    boost::asio::io_context io_context;
    application app(io_context, port);
    printer p(io_context);
    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}