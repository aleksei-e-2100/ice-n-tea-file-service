/**
 * @file server_task.h
 * @brief Server-side component for negotiating a file transfer task
 */

#pragma once

#include "packet.h"
#include "session_ctx.h"
#include "fast_vec_uchar.h"

/**
 * @class ServerTask
 * @brief Implements the processing of a task request and the generation of a server reply.
 */
class ServerTask
{
public:
    /**
     * @brief Constructor
     * @param [in] ctx Session context
     */
    ServerTask(SessionCtx& ctx);

    /**
     * @brief Destructor
     */
    ~ServerTask() = default;

    // Assignment and transfer are prohibited
    ServerTask(const ServerTask&) = delete;
    ServerTask& operator=(const ServerTask&) = delete;
    ServerTask(ServerTask&&) = delete;
    ServerTask& operator=(ServerTask&&) = delete;

    /**
     * @brief Process a task request
     * @return true - if the request is successfully verified
     */
    bool procTaskReq();

    /**
     * @brief Prepare a reply to a task request
     * @param [in] the decision regarding the task
     * @return true - if the reply was prepared successfully
     */
    bool prepareTaskRep(Decision decision);

private:
    SessionCtx& ctx_;

    FastVecUchar tag_;
};
