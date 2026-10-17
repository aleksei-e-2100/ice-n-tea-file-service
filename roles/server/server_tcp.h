/**
 * @file tcp_server.cpp
 * @brief Non-blocking TCP server for IPv4 and IPv6
 */

#pragma once

#include <string>
#include <unordered_set>

#include "endpoint.h"

/**
 * @class ServerTcp
 * @brief Implements TCP server with non-blocking data exchange
 */
class ServerTcp
{
public:
    /**
     * @brief Constructor
     */
    ServerTcp() = default;

    /**
     * @brief Destructor
     */
    ~ServerTcp();

    // Assignment and transfer are prohibited
    ServerTcp(const ServerTcp&) = delete;
    ServerTcp& operator=(const ServerTcp&) = delete;
    ServerTcp(ServerTcp&&) = delete;
    ServerTcp& operator=(ServerTcp&&) = delete;

    /**
     * @brief Open a listening socket
     * @param [in] endpoint Endpoint (address and port) to listen on
     * @return true - if the server is started successfully
     */
    bool open(const Endpoint& endpoint);

    /**
     * @brief Close the server and all client connections
     */
    void close();

    /**
     * @brief Get the listening socket descriptor
     * @return The socket descriptor, or -1
     */
    int getListenSocket() const;

    /**
     * @brief Accept a new client connection
     * @param [out] clientSocket Socket descriptor of the accepted client
     * @return true - if the client was successfully accepted
     */
    bool acceptClientent(int& clientSocket);

    /**
     * @brief Close the client connection
     * @param [in] clientSocket Client socket descriptor
     * @return true - if the client was successfully disconnected
     */
    bool dropClient(int clientSocket);

    /**
     * @brief Send data to the client
     * @param [in] clientSocket Client socket descriptor
     * @param [in] buf Buffer containing the data
     * @param [in] len Number of bytes to send
     * @param [out] bytesSent Number of bytes sent
     * @return true - if the operation was processed successfully
     */
    bool send(int clientSocket, const unsigned char* buf, size_t len, size_t &bytesSent);

    /**
     * @brief Receive data from a client
     * @param [in] clientSocket Client socket descriptor
     * @param [out] buf Buffer for receiving data
     * @param [in] maxLen Length of the receive buffer
     * @param [out] bytesReceived Number of bytes received
     * @return true - if the operation is processed successfully
     */
    bool receive(const int clientSocket, unsigned char* buf, size_t maxLen, size_t& bytesReceived);

    /**
     * @brief Get the description of the last error
     * @return Error description string
     */
    const std::string& getLastErrStr() const;

private:
    int listenSocket_ = -1;
    std::unordered_set<int> clientSockets_;

    std::string errStr_;
};
