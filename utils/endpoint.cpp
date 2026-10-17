/**
 * @file endpoint.cpp
 * @brief Network address (IP + port)
 */

#include "endpoint.h"

#include <arpa/inet.h>
#include <cstring>
#include <limits>

bool Endpoint::operator==(const Endpoint& other) const
{
    if (ipType_ == IpType::Empty)
        return false;

    if (ipType_ != other.ipType_)
        return false;

    if (netPort_ != other.netPort_)
        return false;

    if (ipType_ == IpType::IPv4 &&
        netIp4_.s_addr != other.netIp4_.s_addr)
    {
        return false;
    }

    if (ipType_ == IpType::IPv6 &&
        std::memcmp(&netIp6_, &other.netIp6_, sizeof(struct in6_addr)) != 0)
    {
        return false;
    }

    return true;
}

bool Endpoint::operator!=(const Endpoint& other) const
{
    return !(*this == other);
}

IpType Endpoint::type() const
{
    return ipType_;
}

in_addr Endpoint::netIp4() const
{
    return netIp4_;
}

in6_addr Endpoint::netIp6() const
{
    return netIp6_;
}

in_port_t Endpoint::netPort() const
{
    return netPort_;
}

bool Endpoint::setFromStr(const char* ip, uint16_t port)
{
    if (port == 0)
        return false;

    if (inet_pton(AF_INET, ip, &netIp4_) == 1)
    {
        ipType_ = IpType::IPv4;
        netIp6_ = {0};
        netPort_ = htons(port);
        return true;
    }

    if (inet_pton(AF_INET6, ip, &netIp6_) == 1)
    {
        ipType_ = IpType::IPv6;
        netIp4_ = {0};
        netPort_ = htons(port);
        return true;
    }

    return false;
}

void Endpoint::setFromNet(const in_addr& netIp4, in_port_t netPort)
{
    ipType_ = IpType::IPv4;
    netIp6_ = {0};
    netPort_ = netPort;
}

void Endpoint::setFromNet(const in6_addr& netIp6, in_port_t netPort)
{
    ipType_ = IpType::IPv6;
    netIp4_ = {0};
    netPort_ = netPort;
}

bool Endpoint::checkIpFromStr(const char* ip)
{
    in_addr netIp4;
    in6_addr netIp6;

    if (inet_pton(AF_INET, ip, &netIp4) == 1 ||
        inet_pton(AF_INET6, ip, &netIp6) == 1)
    {
        return true;
    }

    return false;
}

bool Endpoint::checkPortFromU64(uint64_t port)
{
    return (port != 0 && port <= std::numeric_limits<uint16_t>::max());
}
