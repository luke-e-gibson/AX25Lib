#include "socket.hpp"

socket_t connect_kiss(const std::string host, int port)
{
    socket_t sockfd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (sockfd == INVALID_SOCK)
            throw std::runtime_error("socket creation failed");
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0)
    {
        socket_close(sockfd);
        throw std::runtime_error("Invalid address: " + host);
    }
    
    if (connect(sockfd, (sockaddr*)&addr, sizeof(addr)))
    {
        close_socket(sockfd);
        throw std::runtime_error("Connection failed");
    }
    
    return sockfd;
}