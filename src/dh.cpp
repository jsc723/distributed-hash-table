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
        if (argc != 4)
        {
            std::cout << "Usage: dh <id> <port> <ring_id>" << std::endl;
            return 1;
        }
        int id = boost::lexical_cast<int>(argv[1]);
        unsigned short port = boost::lexical_cast<unsigned short>(argv[2]);
        int ring_id = boost::lexical_cast<int>(argv[3]);
        boost::asio::io_context io_context;
        application app(io_context, id, port, ring_id);
        app.init();
        io_context.run();
    }
    catch (std::exception &e)
    {
        logger.log(LogLevel::CRITICAL, "", e.what());
    }

    return 0;
}