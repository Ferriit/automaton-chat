#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

namespace nt {
    class Socket {
    public:
        int port;
        std::string server;
        SOCKET sock_fd = INVALID_SOCKET;
        SOCKET listen_fd = INVALID_SOCKET;
        std::string buffer;

        Socket(int port, const std::string& server = "") {
            this->port = port;
            this->server = server;
            // Initialize Winsock
            WSADATA wsaData;
            int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (iResult != 0) {
                std::cerr << "WSAStartup failed: " << iResult << std::endl;
            }
        }

        ~Socket() {
            this->close();
            WSACleanup();
        }

        inline std::string domain_to_ip(const std::string& domain) {
            addrinfo hints{}, *res = nullptr;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            int ret = getaddrinfo(domain.c_str(), nullptr, &hints, &res);
            if (ret != 0) {
                std::cerr << "Failed to resolve domain: " << gai_strerrorA(ret) << std::endl;
                return "";
            }

            sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(res->ai_addr);
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ipv4->sin_addr), ip, INET_ADDRSTRLEN);

            freeaddrinfo(res);
            return std::string(ip);
        }

        inline int connect_client(int try_domain = 0) {
            sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock_fd == INVALID_SOCKET) {
                std::cerr << "Failed to open socket: " << WSAGetLastError() << std::endl;
                return -1;
            }

            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(this->port);

            // Try parsing server as an IP address
            if (inet_pton(AF_INET, this->server.c_str(), &server_addr.sin_addr) <= 0) {
                // Try converting domain to IP once
                if (try_domain == 0) {
                    this->server = domain_to_ip(this->server);
                    if (this->server.empty()) {
                        this->close();
                        return -1;
                    }
                    return connect_client(1);
                }

                this->close();
                std::cerr << "Invalid address: " << this->server << std::endl;
                return -1;
            }

            if (::connect(sock_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
                std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
                this->close();
                return -1;
            }

            return 0;
        }

        inline int connect_server(int port, int backlog = SOMAXCONN) {
            listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (listen_fd == INVALID_SOCKET) {
                std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
                return -1;
            }

            // Allow immediate reuse of the port
            char opt = 1;
            if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == SOCKET_ERROR) {
                std::cerr << "setsockopt failed: " << WSAGetLastError() << std::endl;
                closesocket(listen_fd);
                return -1;
            }

            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;
            server_addr.sin_port = htons(port);

            if (bind(listen_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
                std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
                closesocket(listen_fd);
                return -1;
            }

            if (listen(listen_fd, backlog) == SOCKET_ERROR) {
                std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
                closesocket(listen_fd);
                return -1;
            }

            std::cout << "Server listening on port " << port << std::endl;
            return 0;
        }

        inline int accept_client() {
            sockaddr_in client_addr{};
            int addr_len = sizeof(client_addr);

            sock_fd = accept(listen_fd, (sockaddr*)&client_addr, &addr_len);
            if (sock_fd == INVALID_SOCKET) {
                std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
                return -1;
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            std::cout << "Client connected from " << client_ip << ":" 
                      << ntohs(client_addr.sin_port) << std::endl;

            return sock_fd;
        }

        inline bool is_connected() {
            return sock_fd != INVALID_SOCKET;
        }

        inline bool send(std::string message) {
            message += '\0';
            size_t total_sent = 0;

            while (total_sent < message.size()) {
                int sent = ::send(sock_fd, message.c_str() + total_sent, (int)(message.size() - total_sent), 0);
                if (sent == SOCKET_ERROR) {
                    std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
                    return false;
                }
                total_sent += sent;
            }
            return true;
        }

        inline std::string receive() {
            char buf[1024];
            while (true) {
                int n = ::recv(sock_fd, buf, sizeof(buf), 0);
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
            if (sock_fd != INVALID_SOCKET) {
                closesocket(sock_fd);
                sock_fd = INVALID_SOCKET;
            }
            if (listen_fd != INVALID_SOCKET) {
                closesocket(listen_fd);
                listen_fd = INVALID_SOCKET;
            }
        }
    };
};
