/**
 * @file client_channel.h
 * @brief Client-side file transfer channel
 */

#pragma once

#include <string>
#include <utility>

#include "cmd_line.h"
#include "client_tcp.h"
#include "client_handshake.h"
#include "client_task.h"
#include "file_receive.h"
#include "file_send.h"
#include "session_ctx.h"
#include "timer.h"

class ClientChannel;

/**
 * @enum ClientState
 * @brief Client channel state (stage)
 */
enum class ClientState
{
    NotOpen,
    SendHandshakeReq,
    WaitHandshakeRep,
    SendTaskReq,
    WaitTaskRep,
    SendConfirm,
    SendFile,
    ReceiveFile,
    Finished
};

/**
 * @enum ClientTag
 * @brief Client channel tag for use with epoll
 */
struct ClientTag
{
    enum Type
    {
        Socket,
        Timer
    };

    ClientChannel* channel;
    Type type;
};

/**
 * @class ClientChannel
 * @brief Implements a sequence of actions when receiving (or sending) a file
 */
class ClientChannel
{
public:
    /**
     * @brief Constructor
     */
    ClientChannel();

    /**
     * @brief Destructor
     */
    ~ClientChannel();

    // Assignment and transfer are prohibited
    ClientChannel(const ClientChannel&) = delete;
    ClientChannel& operator=(const ClientChannel&) = delete;
    ClientChannel(ClientChannel&&) = delete;
    ClientChannel& operator=(ClientChannel&&) = delete;

    /**
     * @brief Open a client channel
     * @param [in] workMode Application operating mode
     * @param [in] fPath File path
     * @param [in] fName File name
     * @param [in] fSize File size
     * @param [in] serverStaticPubKey Server's public key
     * @param [in] tcp TCP client
     * @param [in] epollFd Epoll descriptor
     * @return true - on success
     */
    bool open(WorkMode workMode,
              const std::string& serverName,
              const std::string& fPath,
              const std::string& fName,
              size_t fSize,
              const std::string& serverStaticPubKey,
              ClientTcp* tcp,
              int epollFd);

    /**
     * @brief Close the client channel
     */
    void close();

    /**
     * @brief Get channel state (stage)
     * @return channel state (stage)
     */
    ClientState getState() const;

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
    ClientTcp* tcp_;
    ClientHandshake handshake_;
    ClientTask task_;
    FileSend fileSend_;
    FileReceive fileReceive_;

    // Session context
    SessionCtx ctx_;
    Timer timer_;

    // Event management
    ClientTag socketTag_;
    ClientTag timerTag_;
    int epollFd_;

    // Channel state
    ClientState state_;
};
