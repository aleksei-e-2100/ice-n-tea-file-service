/**
 * @file  client_tcp.h
 * @brief Non-blocking TCP client for IPv4 and IPv6
 */

#pragma once

#include <string>

#include "endpoint.h"

/**
 * @class ClientTcp
 * @brief Implements TCP client with non-blocking data exchange
 */
class ClientTcp
{
public:
    /**
     * @brief Constructor
     */
    ClientTcp() = default;

    /**
     * @brief Destructor
     */
    ~ClientTcp();

    // Assignment and transfer are prohibited
    ClientTcp(const ClientTcp&) = delete;
    ClientTcp& operator=(const ClientTcp&) = delete;
    ClientTcp(ClientTcp&&) = delete;
    ClientTcp& operator=(ClientTcp&&) = delete;

public:
    /**
     * @brief Open a TCP socket and connect to the server
     * @param [in] endpoint Endpoint (server address and port)
     * @return true - if the connection is successful
     */
    bool open(const Endpoint& endpoint);

    /**
     * @brief Close connection
     */
    void close();

    /**
     * @brief Get socket descriptor
     * @return The socket descriptor, or -1
     */
    int getSocket() const;

    /**
     * @brief Send data
     * @param [in] buf Buffer containing the data
     * @param [in] len Number of bytes to send
     * @param [out] bytesSent Number of bytes sent
     * @return true - if the operation was processed successfully
     */
    bool send(unsigned char* buf, size_t len, size_t& bytesSent);

    /**
     * @brief Receive data
     * @param [out] buf Buffer for receiving data
     * @param [in] maxLen Length of the receive buffer
     * @param [out] bytesReceived Number of bytes received
     * @return true - if the operation was processed successfully
     */
    bool receive(unsigned char* buf, size_t maxLen, size_t& bytesReceived);

    /**
     * @brief Get the description of the last error
     * @return Error description string
     */
    const std::string& getLastErrStr() const;

private:
    int socket_ = -1;

    std::string errStr_;
};
