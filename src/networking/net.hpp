#pragma once

#ifdef _WIN32
#include "nt.hpp"
using Socket = nt::Socket;
#else
#include "posix.hpp"
using Socket = posix::Socket;
#endif

#define DEF_PORT 55000

namespace net {
    class NetLib {
        public:
        Socket sock;

        NetLib(int port, const std::string& server = "")
            : sock(port, server) {}

        inline std::string domain_to_ip(const std::string& domain) {
            return this->sock.domain_to_ip(domain);
        }

        inline int connect_client() {
            return this->sock.connect_client();
        }

        inline int connect_server(int port, int backlog = SOMAXCONN) {
            return this->sock.connect_server(port, backlog);
        }

        inline int accept_client() {
            return this->sock.accept_client();
        }

        inline bool is_connected() {
            return this->sock.is_connected();
        }

        inline bool send(std::string message) {
            return this->sock.send(message);
        }

        inline std::string receive() {
            return this->sock.receive();
        }

        inline void close() {
            this->sock.close();
        }
    };
};
