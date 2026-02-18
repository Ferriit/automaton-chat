#include "networking/net.hpp"
#include "formatting/fml.hpp"

#include <string>

int main(int argc, char** argv) {
    //if (argc < 2) {
    //    std::cerr << "Too few arguments, message is required" << std::endl;
    //    return 1;
    //}

    fml::FmlData data = {
        {"op", {
            {"type", {"SEND"}},
        }},
        {"user", {
            {"username", {"[USERNAME]"}},
            {"uuid", {"[UUID]"}},
            {"pblkey", {"[PublicKey]"}}
        }},
        {"message", {
            {"messageid", {"[MESSAGEID]"}},
            {"messagecontent", {"[MESSAGECONTENT]"}},
            {"attachment", {"[ATTACHMENT1]", "[ATTACHMENT2]"}}
        }}
    };

    net::NetLib sock(DEF_PORT, "192.168.86.99");

    if (sock.connect_client() < 0) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }
    sock.send(fml::encode_fml(data));

    return 0;
}
