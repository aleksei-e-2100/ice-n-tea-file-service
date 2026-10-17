/**
 * @file client_tcp.cpp
 * @brief Non-blocking TCP client for IPv4 and IPv6
 */

#include "client_tcp.h"

#include <cstring>
#include <cerrno>
#include <cassert>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>

ClientTcp::~ClientTcp()
{
    close();
}

bool ClientTcp::open(const Endpoint& endpoint)
{
    assert(socket_ == -1);

    // Open socket
    if (endpoint.type() == IpType::IPv4)
        socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    else if (endpoint.type() == IpType::IPv6)
        socket_ = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if (socket_ == -1)
    {
        errStr_ = "[TCP] " + std::string(std::strerror(errno));
        return false;
    }

    // Make the socket reusable
    int intFlag = 1;

    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &intFlag, sizeof(intFlag)) < 0)
    {
        errStr_ = "[TCP] " + std::string(std::strerror(errno));
        close();
        return false;
    }

    // Connect to the server
    sockaddr_storage address = {0};

    if (endpoint.type() == IpType::IPv4)
    {
        sockaddr_in addr4 = {0};
        addr4.sin_family = AF_INET;
        addr4.sin_addr = endpoint.netIp4();
        addr4.sin_port = endpoint.netPort();

        std::memcpy(&address, &addr4, sizeof(addr4));
    }
    else if (endpoint.type() == IpType::IPv6)
    {
        sockaddr_in6 addr6 = {0};
        addr6.sin6_family = AF_INET6;
        addr6.sin6_addr = endpoint.netIp6();
        addr6.sin6_port = endpoint.netPort();

        std::memcpy(&address, &addr6, sizeof(addr6));
    }

    if (::connect(socket_,
                  reinterpret_cast<struct sockaddr*>(&address),
                  sizeof(address)))
    {
        errStr_ = "[TCP] " + std::string(std::strerror(errno));
        close();
        return false;
    }

    // Switch the socket to non-blocking mode
    int flags = fcntl(socket_, F_GETFL, 0);

    if (flags == -1)
    {
        errStr_ = "[TCP] " + std::string(std::strerror(errno));
        close();
        return false;
    }

    flags |= O_NONBLOCK;

    if (fcntl(socket_, F_SETFL, flags) == -1)
    {
        errStr_ = "[TCP] " + std::string(std::strerror(errno));
        close();
        return false;
    }

    return true;
}

void ClientTcp::close()
{
    ::close(socket_);
    socket_ = -1;
}

int ClientTcp::getSocket() const
{
   return socket_;
}

bool ClientTcp::send(unsigned char* buf, size_t len, size_t &bytesSent)
{
    assert(socket_ != -1);

    bytesSent = 0;

    while (true)
    {
        // Add MSG_NOSIGNAL so that, when working with a connection-oriented socket,
        // a SIGPIPE signal is not triggered if the peer terminates the connection,
        // which prevents the client from crashing.
        auto res = ::send(socket_, buf, len, MSG_NOSIGNAL);

        // res == 0 when the OS send buffer is full
        if (res >= 0)
        {
            bytesSent = res;
            return true;
        }

        if (res < 0 && errno == EAGAIN)
            return true;

        // If sending interrupted by a system signal
        if (res < 0 && errno == EINTR)
            continue;

        break;
    }

    errStr_ = "[TCP] " + std::string(std::strerror(errno));

    return false;
}

bool ClientTcp::receive(unsigned char* buf, size_t maxLen, size_t& bytesReceived)
{
    assert(socket_ != -1);
    assert(maxLen != 0);

    bytesReceived = 0;

    while (true)
    {
        auto res = recv(socket_, buf, maxLen, 0);

        if (res > 0)
        {
            bytesReceived = res;
            return true;
        }

        // If peer has closed the connection
        if (res == 0)
            return true;

        if (res < 0 && errno == EAGAIN)
            return true;

        // If receiving is interrupted by a system signal
        if (res < 0 && errno == EINTR)
            continue;

        break;
    }

    errStr_ = "[TCP] " + std::string(std::strerror(errno));

    return false;
}

const std::string& ClientTcp::getLastErrStr() const
{
    return errStr_;
}
