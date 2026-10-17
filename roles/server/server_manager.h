/**
 * @file server_manager.h
 * @brief Server manager. Manages the file transfer channels.
 */

#pragma once

#include <map>

#include <sys/epoll.h>

#include "server_tcp.h"
#include "server_channel.h"

/**
 * @enum ServerManagerResult
 * @brief The result of a single execution of the client manager
 */
enum class ServerManagerResult
{
    Continue, ///< Successful; a follow-up is required
    Error     ///< Error; receiving (sending)  not completed (failed); the manager must be closed
};

/**
 * @class SerevrManager
 * @brief Implements the server manager
 */
class ServerManager
{
public:
    /**
     * @brief Constructor
     */
    ServerManager();

    /**
     * @brief Destructor
     */
    ~ServerManager();

    // Assignment and transfer are prohibited
    ServerManager(const ServerManager&) = delete;
    ServerManager& operator=(const ServerManager&) = delete;
    ServerManager(ServerManager&&) = delete;
    ServerManager& operator=(ServerManager&&) = delete;

    /**
     * @brief Open the manager and prepare to accept connections
     * @return true - on success
     */
    bool open();

    /**
     * @brief Close manager (stop the manager's work)
     */
    void close();

    /**
     * @brief Perform one iteration of event processing
     * @return The result of a single execution
     */
    ServerManagerResult runOnce();

private:
    /**
     * @brief Process new client connections
     * @return true - if processing is successful
     */
    bool procNewClients();

    /**
     * @brief Output the progress of file operations
     * @param [in] progressStr A string describing the progress
     */
    void showProgressStr(const std::string& progressStr) const;

    /**
     * @brief Output an error message
     * @param [in] errStr A string containing the error message
     */
    void showErrStr(const std::string& errStr) const;

private:
    // Executor classes
    ServerTcp tcp_;
    std::map<ServerChannel*, int> channelsSockets_;

    // Event management
    static constexpr int maxEvents_ = 128;
    static constexpr int waitTime_ = 500;
    int epollFd_;
    epoll_event eventsPool_[maxEvents_];
};
