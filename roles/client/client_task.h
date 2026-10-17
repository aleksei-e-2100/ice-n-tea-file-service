/**
 * @file client_task.h
 * @brief Client-side component for negotiating a file transfer task
 */

#pragma once

#include "fast_vec_uchar.h"

#include "session_ctx.h"

/**
 * @class ClientTask
 * @brief Implements the preparation of the task request and the processing of the server reply
 */
class ClientTask
{
public:
    /**
     * @brief Constructor
     * @param [in] ctx Session context
     */
    ClientTask(SessionCtx& ctx);

    /**
     * @brief Destructor
     */
    ~ClientTask() = default;

    // Assignment and transfer are prohibited
    ClientTask(const ClientTask&) = delete;
    ClientTask& operator=(const ClientTask&) = delete;
    ClientTask(ClientTask&&) = delete;
    ClientTask& operator=(ClientTask&&) = delete;

    /**
     * @brief Prepare task request
     * @return true - if the request is prepared successfully
     */
    bool prepareTaskReq();

    /**
     * @brief Process task reply
     * @return true - on success
     */
    bool procTaskRep();

private:
    SessionCtx& ctx_;

    FastVecUchar tag_;
};
