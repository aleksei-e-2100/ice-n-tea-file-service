
#include "tcpchannel.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

TcpChannel::~TcpChannel()
{
    closeServer();
}

bool TcpChannel::openServer(const uint16_t port)
{
    if (_listenSocket != -1)
    {
        _errorMsg = "TCP listening socket is already opened";

        return false;
    }

    // Открываем сокет в неблокирующем режим
    _listenSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

    if (_listenSocket == -1)
    {
        _errorMsg = strerror(errno);

        return false;
    }

    // Делаем сокет reusable
    int intFlag = 1;

    if (setsockopt(_listenSocket,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &intFlag,
                   sizeof(intFlag)) < 0)
    {
        _errorMsg = strerror(errno);

        closeServer();

        return false;
    }

    // Привязываем сокет (для любого сетевого интерфейса)
    sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { htonl(INADDR_ANY) }
    };

    if (bind(_listenSocket,
             reinterpret_cast<struct sockaddr*>(&address),
             sizeof(address)) < 0)
    {
        _errorMsg = strerror(errno);

        closeServer();

        return false;
    }

    // Начинаем прослушивать сокет
    if (listen(_listenSocket, SOMAXCONN) < 0)
    {
        _errorMsg = strerror(errno);

        closeServer();

        return false;
    }

    return true;
}

void TcpChannel::closeServer()
{
    for (auto& client : _clients)
    {
        ::close(client);
    }

    _clients.clear();

    if (_listenSocket != -1)
    {
        ::close(_listenSocket);

        _listenSocket = -1;
    }
}

bool TcpChannel::acceptClient(int& clientSocket)
{
    sockaddr_in address;
    socklen_t addressSize = sizeof(address);

    // Принимаем клиента в неблокирующем режиме
    clientSocket = accept4(_listenSocket,
                           reinterpret_cast<struct sockaddr*>(&address),
                           &addressSize,
                           SOCK_NONBLOCK);

    if (clientSocket == -1)
    {
        _errorMsg = strerror(errno);

        return false;
    }

    _clients.insert(clientSocket);

    return true;
}

bool TcpChannel::dropClient(int clientSocket)
{
    if (_clients.count(clientSocket) == 0)
        return false;

    ::close(clientSocket);

    _clients.erase(clientSocket);

    return true;
}

bool TcpChannel::sendData(int clientSocket,
                          const unsigned char* buf,
                          uint32_t len)
{
    if (_clients.count(clientSocket) == 0)
    {
        _errorMsg = "Attempt to send data to non-existent client";

        return false;
    }

    uint32_t sent = 0;

    while (sent < len)
    {
        auto res = send(clientSocket, buf + sent, len - sent, 0);

        if (res < 0)
        {
            _errorMsg = strerror(errno);

            if (errno != EAGAIN)
                return false;
        }

        if (res > 0)
            sent += res;
    }

    return true;
}

bool TcpChannel::receiveData(int clientSocket,
                             unsigned char* buf,
                             uint32_t maxLen,
                             uint32_t& len)
{
    if (_clients.count(clientSocket) == 0)
    {
        _errorMsg = "Attempt to receive data from non-existent client";

        return 0;
    }

    auto res = recv(clientSocket, buf, maxLen, 0);

    if (res < 0)
    {
        _errorMsg = strerror(errno);

        if (errno == EAGAIN)
        {
            len = 0;

            return true;
        }

        return false;
    }

    len = res;

    return true;
}

bool TcpChannel::serverIsOpen()
{
    return _listenSocket != -1;
}

bool TcpChannel::validateListenSocket()
{
    int error = 0;

    socklen_t len = sizeof(error);

    if (getsockopt(_listenSocket, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
    {
        _errorMsg = strerror(errno);

        return false;
    }

    if (error != 0)
    {
        _errorMsg = strerror(errno);

        return false;
    }

    return true;
}

std::string TcpChannel::getErrorMsg()
{
    return std::move(_errorMsg);
}
