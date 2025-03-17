#include "address.h"

bool Address::operator==(const Address& other) const
{
    return _port == other._port &&
           _ip.s_addr == other._ip.s_addr;
}

bool Address::operator!=(const Address& other) const
{
    return !(_port == other._port &&
             _ip.s_addr == other._ip.s_addr);
}

void Address::setFromString(const char* ip, uint16_t port)
{
    if (inet_pton(AF_INET, ip, &_ip) <= 0)
        return;

    _port = ntohs(port);
}

void Address::setFromNet(const in_addr& ip, uint16_t port)
{
    _ip = ip;

    _port = port;
}

bool Address::valid() const
{
    return _port != 0;
}

in_addr Address::netIp() const
{
    return _ip;
}

uint16_t Address::netPort() const
{
    return _port;
}
