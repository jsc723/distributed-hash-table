#include "../headers.hpp"
#include "../utils.hpp"
#include "../serializer.hpp"
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <cctype>

using boost::asio::ip::tcp;
using std::cout;
using std::endl;

void to_lower(string &s)
{
    for (auto &c : s)
    {
        c = tolower(c);
    }
}

int main(int argc, char *argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: client <port>" << std::endl;
            return 1;
        }

        unsigned short port = boost::lexical_cast<unsigned short>(argv[1]);

        boost::asio::io_context io_context;
        ba::ip::address_v4 addr = ba::ip::address_v4::from_string(LOCALHOST);

        tcp::resolver resolver(io_context);
        ba::ip::tcp::endpoint endpoint(addr, port);

        tcp::socket socket(io_context);
        socket.connect(endpoint);

        for (;;)
        {

            string line;
            std::cout << "> " << std::flush;
            getline(std::cin, line);
            vector<string> cmd;
            boost::split(cmd, line, boost::is_any_of(" \t"));
            if (cmd.size() == 0)
                continue;

            boost::array<char, 128> buf;
            boost::system::error_code error;

            to_lower(cmd[0]);
            if (cmd[0] == "set")
            {
                if (cmd.size() == 3) {
                    cmd.emplace_back("0");
                }
                if (cmd.size() != 4)
                {
                    cout << "invalid num of args" << endl;
                    continue;
                }
                dh_message::VersionedValue *vv = new dh_message::VersionedValue();
                vv->set_value(cmd[2]);
                vv->set_version(boost::lexical_cast<uint64_t>(cmd[3]));

                dh_message::SetRequest req;
                req.set_ttl(MyConst::request_default_ttl); 
                req.set_sender_id(-1);
                req.set_transaction_id(0);
                req.set_key(cmd[1]);
                req.set_allocated_value(vv);
                MessageHdr *msg_req = Serializer::Message::allocEncode(MsgType::SET, req);
                //print_bytes(msg_req, std::min<int>(100, msg_req->size));

                ba::write(socket, ba::buffer(msg_req, msg_req->size));

                MessageHdr hdr;
                ba::read(socket, ba::buffer(&hdr, sizeof(MessageHdr)), ba::transfer_exactly(sizeof(MessageHdr)));
                uint32_t packet_sz = hdr.size;
                if (hdr.HEAD != MSG_HEAD) {
                    cout << "message is corruptted" << endl;
                }

                MessageHdr *msg_result = (MessageHdr *)malloc(packet_sz);
                *msg_result = hdr;
                ba::read(socket, ba::buffer(msg_result->payload, packet_sz - sizeof(MessageHdr)));

                dh_message::SetResponse response;
                response.ParseFromArray(msg_result->payload, packet_sz - sizeof(MessageHdr));

                if (response.success()) {
                    cout << "ok" << endl;
                } else {
                    cout << "failed: " << response.msg() << endl;
                }

                Serializer::Message::dealloc(msg_result);
                Serializer::Message::dealloc(msg_req);
            }
            else if (cmd[0] == "get")
            {
                if (cmd.size() != 2)
                {
                    cout << "invalid num of args" << endl;
                    continue;
                }
                dh_message::GetRequest req;
                req.set_ttl(MyConst::request_default_ttl); 
                req.set_sender_id(-1);
                req.set_transaction_id(0);
                req.set_key(cmd[1]);
                MessageHdr *msg_req = Serializer::Message::allocEncode(MsgType::GET, req);
                //print_bytes(msg_req, std::min<int>(100, msg_req->size));

                ba::write(socket, ba::buffer(msg_req, msg_req->size));

                MessageHdr hdr;
                ba::read(socket, ba::buffer(&hdr, sizeof(MessageHdr)), ba::transfer_exactly(sizeof(MessageHdr)));
                uint32_t packet_sz = hdr.size;
                if (hdr.HEAD != MSG_HEAD) {
                    cout << "message is corruptted" << endl;
                }

                MessageHdr *msg_result = (MessageHdr *)malloc(packet_sz);
                *msg_result = hdr;
                ba::read(socket, ba::buffer(msg_result->payload, packet_sz - sizeof(MessageHdr)));

                dh_message::GetResponse response;
                response.ParseFromArray(msg_result->payload, packet_sz - sizeof(MessageHdr));

                if (response.success()) {
                    cout << response.value().value() << endl;
                } else {
                    cout << "failed" << endl;
                }

                Serializer::Message::dealloc(msg_result);
                Serializer::Message::dealloc(msg_req);
            }
            else if (cmd[0] == "exit")
            {
                return 0;
            }
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}