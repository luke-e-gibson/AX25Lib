#include "socket.hpp"

#include <cstdlib>
#include <spdlog/spdlog.h>

socket_t connect_kiss(const std::string host, int port)
{
    socket_t sockfd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (sockfd == INVALID_SOCK)
    {
        spdlog::critical("Failed to create a TCP socket for Direwolf at {}:{}", host, port);
        std::exit(EXIT_FAILURE);
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0)
    {
        socket_close(sockfd);
        spdlog::critical("The configured Direwolf host {} is not a valid IPv4 address. Example: 127.0.0.1", host);
        std::exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (sockaddr*)&addr, sizeof(addr)))
    {
        close_socket(sockfd);
        spdlog::critical("Failed to connect to Direwolf at {}:{}. Ensure Direwolf is running, KISS TCP is enabled, and the port is correct.", host, port);
    }

    return sockfd;
}
