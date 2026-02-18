#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <iostream>

namespace posix {
    class Socket {
        public:
        int port;
        std::string server;
        int sock_fd = -1;
        int listen_fd = -1;

        std::string buffer;

        Socket(int port, const std::string& server) {
            this->port = port;
            this->server = server;
        }

        inline std::string domain_to_ip(const std::string& domain) {
            const char* cdomain = domain.c_str();

            addrinfo hints{}, *res;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            if (getaddrinfo(cdomain, nullptr, &hints, &res) != 0) {
                std::cerr << "Failed to resolve domain\n";
                return "";
            }

            sockaddr_in* ipv4 = (sockaddr_in*)res->ai_addr;
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ipv4->sin_addr), ip, INET_ADDRSTRLEN);
        
            return std::string(ip);
        }

        inline int connect_client(int try_domain = 0) {
            // Connects to a server using the server and port specified in the constructor

            this->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (this->sock_fd < 0) {
                std::cerr << "Failed to open socket: " << strerror(errno) << std::endl;
                return -1;
            }
            sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(this->port);
            if (inet_pton(AF_INET, this->server.c_str(), &server_addr.sin_addr) <= 0) {
                // Try converting domain to IP
                if (try_domain == 0) {
                    server = this->domain_to_ip(this->server);
                    return connect_client(1);
                }

                this->close();
                std::cerr << "Invalid address: " << this->server << std::endl;
                return -1;
            }
            
            if (::connect(this->sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                std::cerr << "Connect failed: " << strerror(errno) << std::endl;
                this->close();
                return -1;
            }
            return 0;
        }

        inline int connect_server(int port, int backlog = SOMAXCONN) {
            // Create the listening socket
            listen_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (listen_fd < 0) {
                std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
                return -1;
            }

            // Allow immediate reuse of the port after closing
            int opt = 1;
            if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
                std::cerr << "setsockopt failed: " << strerror(errno) << std::endl;
                ::close(listen_fd);
                return -1;
            }

            // Bind to all interfaces on the given port
            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;
            server_addr.sin_port = htons(port);

            if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                std::cerr << "Bind failed: " << strerror(errno) << std::endl;
                ::close(listen_fd);
                return -1;
            }

            // Start listening
            if (listen(listen_fd, backlog) < 0) {
                std::cerr << "Listen failed: " << strerror(errno) << std::endl;
                ::close(listen_fd);
                return -1;
            }

            std::cout << "Server listening on port " << port << std::endl;
            return 0;
        }

        inline int accept_client() {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);

            sock_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (sock_fd < 0) {
                std::cerr << "Accept failed: " << strerror(errno) << std::endl;
                return -1;
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            std::cout << "Client connected from " << client_ip << ":" 
                      << ntohs(client_addr.sin_port) << std::endl;

            return sock_fd;
        }

        inline bool is_connected() {
            return this->sock_fd >= 0;
        }

        inline bool send(std::string message) {
            message += '\0';

            size_t total_sent = 0;
            while (total_sent < message.size()) {
                ssize_t sent = ::send(this->sock_fd, message.c_str() + total_sent, message.size() - total_sent, 0);
                if (sent < 0) {
                    std::cerr << "Send failed: " << strerror(errno) << std::endl;
                    return false;
                }
                total_sent += sent;
            }
            return true;
        }


        inline std::string receive() {
            char buf[1024];
            while (true) {
                ssize_t n = ::recv(sock_fd, buf, sizeof(buf), 0);
                if (n <= 0) break;

                buffer.append(buf, n);
                size_t pos = buffer.find('\0');
                if (pos != std::string::npos) {
                    std::string message = buffer.substr(0, pos);
                    buffer = buffer.substr(pos + 1);
                    return message;
                }
            }
            return "";
        }

        inline void close() {
            if (sock_fd >= 0) {
                ::close(sock_fd);
                sock_fd = -1;
            }
        }

        ~Socket() {
            this->close();
        }
    };
};

