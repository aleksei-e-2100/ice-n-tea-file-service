/**
 * @file client_manager.h
 * @brief Client manager. Manages the file transfer channel.
 */

#pragma once

#include <sys/epoll.h>

#include "cmd_line.h"
#include "client_tcp.h"
#include "client_channel.h"

/**
 * @enum ClientManagerResult
 * @brief The result of a single execution of the client manager
 */
enum class ClientManagerResult
{
    Continue, ///< Successful; a follow-up is required
    Finished, ///< Successful; receiving (sending) complete; manager requires closure
    Error     ///< Error; receiving (sending)  not completed (failed); the manager must be closed
};

/**
 * @class ClientManager
 * @brief Implements the client manager
 */
class ClientManager
{
public:
    /**
     * @brief Constructor
     */
    ClientManager();

    /**
     * @brief Destructor
     */
    ~ClientManager();

    // Assignment and transfer are prohibited
    ClientManager(const ClientManager&) = delete;
    ClientManager& operator=(const ClientManager&) = delete;
    ClientManager(ClientManager&&) = delete;
    ClientManager& operator=(ClientManager&&) = delete;

    /**
     * @brief Open the manager and prepare the file transfer channel
     * @param [in] workMode Application operating mode
     * @param [in] fileArg The name of or path to the file
     * @param [in] serverName Server name
     * @return true - on success
    */
    bool open(WorkMode workMode,
              const std::string& fileArg,
              const std::string& serverName);

    /**
     * @brief Close manager (stop the manager's work)
     */
    void close();

    /**
     * @brief Perform one iteration of event processing
     * @return The result of a single execution
     */
    ClientManagerResult runOnce();

private:
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
    ClientTcp tcp_;
    ClientChannel channel_;

    // Event management
    static constexpr int maxEvents_ = 2;
    static constexpr int waitTime_ = 500;
    int epollFd_;
    epoll_event eventsPool_[maxEvents_];
};
