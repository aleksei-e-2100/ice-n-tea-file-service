
#include "tcpchannel.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

TcpChannel::~TcpChannel()
{
    close();
}

bool TcpChannel::open(const Address& serverAddress)
{
    if (_socket != -1)
    {
        _errorMsg = "TCP socket is already opened";

        return false;
    }

    // Открываем сокет
    _socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (_socket == -1)
    {
        _errorMsg = strerror(errno);

        return false;
    }

    // Делаем сокет reusable
    int intFlag = 1;

    if (setsockopt(_socket,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &intFlag,
                   sizeof(intFlag)) < 0)
    {
        _errorMsg = strerror(errno);

        close();

        return false;
    }

    // Соединяемся с сервером
    sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = serverAddress.netPort(),
        .sin_addr = serverAddress.netIp()
    };

    if (connect(_socket,
                reinterpret_cast<struct sockaddr*>(&address),
                sizeof(address)))
    {
        _errorMsg = strerror(errno);

        close();

        return false;
    }

    // Переводим сокет в неблокирующий режим
    int flags = fcntl(_socket, F_GETFL, 0);

    if (flags == -1)
    {
        _errorMsg = strerror(errno);

        close();

        return false;
    }

    flags |= O_NONBLOCK;

    if (fcntl(_socket, F_SETFL, flags) == -1)
    {
        _errorMsg = strerror(errno);

        close();

        return false;
    }

    return true;
}

void TcpChannel::close()
{
    if (_socket != -1)
    {
        ::close(_socket);

        _socket = -1;
    }
}

bool TcpChannel::sendData(unsigned char* buf, uint32_t len)
{
    uint32_t sent = 0;

    while (sent < len)
    {
        auto res = send(_socket, buf + sent, len - sent, 0);

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

bool TcpChannel::receiveData(unsigned char* buf,
                             uint32_t maxLen,
                             uint32_t& len)
{
    auto res = recv(_socket, buf, maxLen, 0);

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

bool TcpChannel::isOpen()
{
    return _socket != -1;
}

std::string TcpChannel::getErrorMsg()
{
    return std::move(_errorMsg);
}
