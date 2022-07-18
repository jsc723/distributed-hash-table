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
                shared_msg msg_req = Serializer::Message::allocEncode(MsgType::SET, req);
                //print_bytes(msg_req, std::min<int>(100, msg_req->size));

                ba::write(socket, msg_req->buffer());

                MsgHdr hdr;
                ba::read(socket, ba::buffer(&hdr, sizeof(MsgHdr)), ba::transfer_exactly(sizeof(MsgHdr)));
                uint32_t packet_sz = hdr.size;
                if (hdr.HEAD != MSG_HEAD) {
                    cout << "message is corruptted" << endl;
                }

                shared_msg msg_result = MsgHdr::create_shared(packet_sz);
                *msg_result = hdr;
                ba::read(socket, msg_result->payload_buffer());

                dh_message::SetResponse response;
                response.ParseFromArray(msg_result->payload, msg_result->payload_size());

                if (response.success()) {
                    cout << "ok" << endl;
                } else {
                    cout << "failed: " << response.msg() << endl;
                }

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
                shared_msg msg_req = Serializer::Message::allocEncode(MsgType::GET, req);
                //print_bytes(msg_req, std::min<int>(100, msg_req->size));

                ba::write(socket, msg_req->buffer());

                MsgHdr hdr;
                ba::read(socket, ba::buffer(&hdr, sizeof(MsgHdr)), ba::transfer_exactly(sizeof(MsgHdr)));
                uint32_t packet_sz = hdr.size;
                if (hdr.HEAD != MSG_HEAD) {
                    cout << "message is corruptted" << endl;
                }

                shared_msg msg_result = MsgHdr::create_shared(packet_sz);
                *msg_result = hdr;
                ba::read(socket, msg_result->payload_buffer());

                dh_message::GetResponse response;
                response.ParseFromArray(msg_result->payload, msg_result->payload_size());

                if (response.success()) {
                    cout << response.value().value() << endl;
                } else {
                    cout << "failed" << endl;
                }

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