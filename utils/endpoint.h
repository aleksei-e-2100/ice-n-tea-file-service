/**
* @file endpoint.h
* @brief Network address (IP + port)
*/

#pragma once

#include <netinet/in.h>
#include <cstdint>

/**
 * @enum IpType
 * @brief IP address type
 */
enum class IpType
{
    IPv4,
    IPv6,
    Empty
};

/**
 * @class Endpoint
 * @brief Network address (IP + port)
 */
class Endpoint
{
public:
    /**
     * @brief Compare the endpoints
     * @param [in] other Object for comparison
     * @return true - if the address and port match
     */
    bool operator==(const Endpoint& other) const;

    /**
     * @brief Check endpoints for inequality
     * @param [in] other Object for comparison
     * @return true - if the objects are different
     */
    bool operator!=(const Endpoint& other) const;

    /**
     * @brief Get the address type
     * @return IP address type
     */
    IpType type() const;

    /**
     * @brief Get the IPv4 address
     * @return IPv4 address in network byte order.
     */
    in_addr netIp4() const;

    /**
     * @brief Get the IPv6 address
     * @return IPv6 address in network byte order
     */
    in6_addr netIp6() const;

    /**
     * @brief Get the port
     * @return Port in network byte order
     */
    in_port_t netPort() const;

    /**
     * @brief Set the address and port from a string
     * @param [in] ip IPv4 or IPv6 address
     * @param [in] port Port in host byte order
     * @return true - if set successfully
     */
    bool setFromStr(const char* ip, uint16_t port);

    /**
     * @brief Set the IPv4 address and port
     * @param [in] netIp4 IPv4 address in network byte order
     * @param [in] netPort Port in network byte order
     */
    void setFromNet(const in_addr& netIp4, in_port_t netPort);

    /**
     * @brief Set the IPv6 address and port
     * @param [in] netIp4 IPv6 address in network byte order
     * @param [in] netPort Port in network byte order
     */
    void setFromNet(const in6_addr& netIp6, in_port_t netPort);

    /**
     * @brief Check the validity of an IP address
     * @param [in] ip String representation of the address
     * @return true - if the address is valid
     */
    static bool checkIpFromStr(const char* ip);

    /**
     * @brief Check the validity of the port
     * @param [in] port The port value
     * @return true - if the port is valid
     */
    static bool checkPortFromU64(uint64_t port);

private:
    IpType ipType_ = IpType::Empty;

    in_addr netIp4_ = {0};
    in6_addr netIp6_ = {0};

    in_port_t netPort_ = 0;
};
