#pragma once

#include <arpa/inet.h>

#include <cstring>

class Address
{
public:
    Address() = default;

    bool operator==(const Address& other) const;

    bool operator!=(const Address& other) const;

    void setFromString(const char* ip, uint16_t port);

    void setFromNet(const in_addr& ip_ia, uint16_t port_ns);

    bool valid() const;

    in_addr netIp() const;

    uint16_t netPort() const;

private:
    in_addr _ip;

    uint16_t _port = 0;
};
