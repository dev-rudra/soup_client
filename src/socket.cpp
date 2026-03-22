#include "socket.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>

Socket::Socket() : sock_fd(-1) {}

Socket::~Socket() {
    close();
}

bool Socket::is_connected() const {
    return sock_fd >= 0;
}

bool Socket::connect(const std::string& host, int port) {
    close();

    sock_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        return false;
    }

    // Resolve hostname
    // works for both IPs
    // and domain name
    addrinfo resolver_hints;
    memset(&resolver_hints, 0, sizeof(resolver_hints));
    resolver_hints.ai_family = AF_INET;
    resolver_hints.ai_socktype = SOCK_STREAM;

    addrinfo* resolved_addr = 0;

    if (::getaddrinfo(host.c_str(), 0, & resolver_hints, &resolved_addr) != 0 || resolved_addr == 0) {
        close();
        return false;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memcpy(&server_addr, resolved_addr-> ai_addr, sizeof(server_addr));
    server_addr.sin_port = htons(port);
    ::freeaddrinfo(resolved_addr);

    if (::connect(sock_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        close();
        return false;
    }

    // Disable Nagle (Algorith that optimize a TCP/IP network optimization technique)
    // Sends small packets immediately, no buffering
    int nodelay = 1;
    ::setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

    // Switch to non-blocking
    // so recv_bytes returns when 
    // no data is ready
    int socket_flags = ::fcntl(sock_fd, F_GETFL, 0);
    ::fcntl(sock_fd, F_SETFL, socket_flags | O_NONBLOCK);

    return true;
}

void Socket::close() {
    if (sock_fd >= 0) {
        ::shutdown(sock_fd, SHUT_RDWR);
        ::close(sock_fd);
        sock_fd = -1;
    }
}

bool Socket::send_bytes(const char* buf, size_t length) {
    if (sock_fd < 0) return false;

    const char* write_ptr      = buf;
    size_t      bytes_remaining = length;

    while (bytes_remaining > 0) {
        // MSG_NOSIGNAL: return error instead of raising SIGPIPE if peer closed
        ssize_t bytes_sent = ::send(sock_fd, write_ptr, bytes_remaining, MSG_NOSIGNAL);

        if (bytes_sent > 0) {
            write_ptr       += bytes_sent;
            bytes_remaining -= static_cast<size_t>(bytes_sent);
            continue;
        }

        if (bytes_sent < 0 && errno == EINTR)  {
            continue;
        }

        return false;
    }

    return true;
}

int Socket::recv_bytes(char* buf, size_t max_len) {
    if (sock_fd < 0) {
        return -1;
    }

    ssize_t bytes_read = ::recv(sock_fd, buf, max_len, 0);

    if (bytes_read > 0) {
        return static_cast<int>(bytes_read);
    }

    // Peer closed the connection
    if (bytes_read == 0) {
        return 0;
    }

    // No data available right now — caller should try again later
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
        return -2;
    }

    return -1;
}

int Socket::get_fd() const {
    return sock_fd;
}
