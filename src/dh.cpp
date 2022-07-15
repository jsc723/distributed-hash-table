#include "headers.hpp"
#include "utils.hpp"
#include "printer.hpp"
#include "application.hpp"

using boost::asio::ip::tcp;
using std::string;
using std::vector;
using boost::shared_ptr;


int main(int argc, char *argv[])
{
    try
    {
        if (argc != 4)
        {
            std::cerr << "Usage: dh <id> <port> <ring_id>" << std::endl;
            return 1;
        }
        int id = boost::lexical_cast<int>(argv[1]);
        unsigned short port = boost::lexical_cast<unsigned short>(argv[2]);
        int ring_id = boost::lexical_cast<int>(argv[3]);
        boost::asio::io_context io_context;
        application app(io_context, id, port, ring_id);
        printer p(io_context);
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}