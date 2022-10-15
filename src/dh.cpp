#include "headers.hpp"
#include "utils.hpp"
#include "application.hpp"

using boost::asio::ip::tcp;
using std::string;
using std::vector;
using boost::shared_ptr;

int main(int argc, char *argv[])
{
    srand((unsigned)time(NULL));
    logger.set_log_level(LogLevel::DEBUG);
    try
    {
        if (argc != 5 && argc != 7)
        {
            std::cout << "Usage: dh <id> <ip> <port> <ring_id> [<bootstrap_ip> <bootstrap_port>]" << std::endl;
            return 1;
        }
        int id = boost::lexical_cast<int>(argv[1]);
        string ip = argv[2];
        unsigned short port = boost::lexical_cast<unsigned short>(argv[3]);
        int ring_id = boost::lexical_cast<int>(argv[4]);
        string bootstrap_ip = argc >= 6 ? argv[5] : BOOTSTRAP_IP;
        unsigned short bootstrap_port = argc >= 7 
            ? boost::lexical_cast<unsigned short>(argv[6]) : BOOTSTRAP_PORT;
        boost::asio::io_context io_context;
        application app(io_context, id, ip, port, ring_id, bootstrap_ip, bootstrap_port);
        app.init();
        io_context.run();
    }
    catch (std::exception &e)
    {
        logger.log(LogLevel::CRITICAL, "", e.what());
    }

    return 0;
}