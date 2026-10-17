/**
 * @file tcp_server.cpp
 * @brief Non-blocking TCP server for IPv4 and IPv6
 */

#include "server_tcp.h"

#include <cstring>
#include <cerrno>
#include <cassert>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>

ServerTcp::~ServerTcp()
{
    close();
}

bool ServerTcp::open(const Endpoint &endpoint)
{
    assert(endpoint.type() != IpType::Empty);
    assert(listenSocket_ == -1);

    // Open socket in non-blocking mode
    if (endpoint.type() == IpType::IPv4)
        listenSocket_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    else if (endpoint.type() == IpType::IPv6)
        listenSocket_ = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

    if (listenSocket_ == -1)
    {
        errStr_ = "[TCP] " + std::string(std::strerror(errno));
        return false;
    }

    // Make the socket reusable
    int intFlag = 1;

    if (setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, &intFlag, sizeof(intFlag)) < 0)
    {
        errStr_ = "[TCP] " + std::string(std::strerror(errno));
        close();
        return false;
    }

    // Bind the socket to the address specified in the Endpoint
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

    if (bind(listenSocket_, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0)
    {
        errStr_ = "[TCP] " + std::string(std::strerror(errno));
        close();
        return false;
    }

    // Start to listen on the socket
    if (listen(listenSocket_, SOMAXCONN) < 0)
    {
        errStr_ = "[TCP] " + std::string(std::strerror(errno));
        close();
        return false;
    }

    return true;
}

void ServerTcp::close()
{
    for (auto clientSocket : clientSockets_)
    {
        ::close(clientSocket);
    }

    clientSockets_.clear();
    ::close(listenSocket_);
    listenSocket_ = -1;
}

int ServerTcp::getListenSocket() const
{
   return listenSocket_;
}

bool ServerTcp::acceptClientent(int& clientSocket)
{
    assert(listenSocket_ != -1);

    sockaddr_storage address = {0};
    socklen_t addressSize = sizeof(address);

    // Accept client in non-blocking mode
    clientSocket = accept4(listenSocket_,
                           reinterpret_cast<struct sockaddr*>(&address),
                           &addressSize,
                           SOCK_NONBLOCK);

    if (clientSocket == -1)
    {
        errStr_ = "[TCP] " + std::string(std::strerror(errno));
        return false;
    }

    clientSockets_.insert(clientSocket);

    return true;
}

bool ServerTcp::dropClient(int clientSocket)
{
    assert(clientSockets_.contains(clientSocket));

    ::close(clientSocket);
    clientSockets_.erase(clientSocket);

    return true;
}

bool ServerTcp::send(int clientSocket, const unsigned char* buf, size_t len, size_t &bytesSent)
{
    assert(clientSockets_.contains(clientSocket));

    bytesSent = 0;

    while (true)
    {
        // Add MSG_NOSIGNAL so that, when working with a connection-oriented socket,
        // a SIGPIPE signal is not triggered if the peer terminates the connection,
        // which prevents the server from crashing.
        auto res = ::send(clientSocket, buf, len, MSG_NOSIGNAL);

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

bool ServerTcp::receive(const int clientSocket, unsigned char* buf, size_t maxLen, size_t& bytesReceived)
{
    bytesReceived = 0;

    while (true)
    {
        auto res = recv(clientSocket, buf, maxLen, 0);

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

const std::string& ServerTcp::getLastErrStr() const
{
    return errStr_;
}
