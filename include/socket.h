#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <cstddef>

class Socket {
public:
    Socket();
    ~Socket();

    // Connect to server:port
    bool connect(const std::string& host, int port);

    // Close the connection
    void close();

    // sends all bytes in buffer
    bool send_bytes(const char* buf, size_t length);

    // Reads up to max_len bytes into buf.
    // Returns
    // > 0 bytes read
    // = 0 peer closed
    // < 0 error
    int recv_bytes(char* buf, size_t max_len);

    // Returns the raw file
    // descriptor (used by poll/select in the event loop)
    int get_fd() const;

    bool is_connected() const;

private:
    int sock_fd;
};

#endif
