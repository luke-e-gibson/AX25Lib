#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
#define INVALID_SOCK INVALID_SOCKET
#define close_socket closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
using socket_t = int;
#define INVALID_SOCK -1
#define close_socket close
#endif

#include <string>
#include <spdlog/spdlog.h>

inline void socket_init() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        direwolf_fatal::fail(
            "Windows socket initialization failed",
            "WSAStartup failed while preparing the TCP client used to connect to Direwolf."
        );
#endif
}

inline void socket_close(socket_t sockfd)
{
#ifdef _WIN32
    closesocket(sockfd);
#else
    close_socket(sockfd);
#endif
}

// Call once at program exit on Windows (no-op on Linux)
inline void socket_cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

socket_t connect_kiss(const std::string host, int port);
