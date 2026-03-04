#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>
#include <string>
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

inline void socket_init() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        throw std::runtime_error("WSAStartup failed");
#endif
}

// Call once at program exit on Windows (no-op on Linux)
inline void socket_cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

socket_t connect_kiss(const std::string host, int port);