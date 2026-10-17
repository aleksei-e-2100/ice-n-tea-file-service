/**
 * @file server_channel.h
 * @brief Server-side file transfer channel
 */

#pragma once

#include <string>
#include <utility>

#include "server_tcp.h"
#include "server_handshake.h"
#include "server_task.h"
#include "file_receive.h"
#include "file_send.h"
#include "session_ctx.h"
#include "timer.h"

class ServerChannel;

/**
 * @enum ServerState
 * @brief Server channel state (stage)
 */
enum class ServerState
{
    NotOpen,
    WaitHandshakeReq,
    SendHandshakeRep,
    WaitTaskReq,
    SendTaskRep,
    WaitConfirm,
    SendFile,
    ReceiveFile,
    Finished
};

/**
 * @enum ServerTag
 * @brief Server channel tag for use with epoll
 */
struct ServerTag
{
    enum Type
    {
        Socket,
        Timer
    };

    ServerChannel* channel;
    Type type;
};

/**
 * @class ServerChannel
 * @brief Implements a sequence of actions when receiving (or sending) a file
 */
class ServerChannel
{
public:
    /**
     * @brief Constructor
     */
    ServerChannel();

    /**
     * @brief Destructor
     */
    ~ServerChannel();

    // Assignment and transfer are prohibited
    ServerChannel(const ServerChannel&) = delete;
    ServerChannel& operator=(const ServerChannel&) = delete;
    ServerChannel(ServerChannel&&) = delete;
    ServerChannel& operator=(ServerChannel&&) = delete;

    /**
     * @brief Open a server channel
     * @param [in] tcp TCP server
     * @param [in] clientSocket Client socket descriptor
     * @param [in] epollFd Epoll descriptor
     * @return true - on success
     */
    bool open(ServerTcp* tcp, int clientSocket, int epollFd);

    /**
     * @brief Close the server channel
     */
    void close();

    /**
     * @brief Get channel state (stage)
     * @return channel state (stage)
     */
    ServerState getState() const;

    /**
     * @brief Get task being performed
     * @return Task being performed
     */
    TaskType getTaskType() const;

    /**
     * @brief Get channel's timer
     * @return Channel's timer
     */
    Timer& getTimer();

    /**
     * @brief Process incoming data
     * @return true - if processing is successful
     */
    bool procIncomingData();

    /**
     * @brief Process outgoing data
     * @return true - if processing is successful
     */
    bool procOutgoingData();

    /**
     * @brief Get the progress string
     * @return A pair (a string describing the progress, a flag indicating whether
     *         the output needs to be updated
     */
    std::pair<std::string, bool> getProgressStr();

    /**
     * @brief Get the description of the last error
     * @return Error description string
     */
    const std::string& getLastErrStr() const;

private:
    /**
     *  @brief Perform actions dictated by the current state (stage) of the channel
     *  @return true - upon successful perfoming
     */
    bool procState();

private:
    // Executor classes
    ServerTcp* tcp_;
    ServerHandshake handshake_;
    ServerTask task_;
    FileSend fileSend_;
    FileReceive fileReceive_;

    // Client socket descriptor
    int clientSocket_;

    // Session context
    SessionCtx ctx_;

    // Event management
    Timer timer_;
    ServerTag socketTag_;
    ServerTag timerTag_;
    int epollFd_;

    // Channel state
    ServerState state_;
};
