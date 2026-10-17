/**
 * @file file_send.h
 * @brief Implementation of file sending over a transfer channel
 */

#pragma once

#include "packet.h"
#include "session_ctx.h"
#include "fast_vec_uchar.h"

/**
 * @class FileSend
 * @brief Implements file sending over a transfer channel
 */
class FileSend
{
public:
    /**
     * @brief Constructor
     * @param [in] ctx Session context
     */
    FileSend(SessionCtx& ctx);

    /**
     * @brief Destructor
     */
    ~FileSend();

    // Assignment and transfer are prohibited
    FileSend(const FileSend&) = delete;
    FileSend& operator=(const FileSend&) = delete;
    FileSend(FileSend&&) = delete;
    FileSend& operator=(FileSend&&) = delete;

    /**
     * @brief Prepare a file for sending
     * @return A decision on whether the file can be sended
     */
    Decision readySendFile();

    /**
     * @brief Prepare a file block (or blocks) for sending
     * @return true - if the block(s) were successfully prepared
     */
    bool prepareSendBlocks();

    /**
     * @brief Process confirmation of file receipt (or readiness to receive)
     * @return true - if the confirmation was processed successfully
     */
    bool procConfirm();

    /**
     * @brief Close file sending
     */
    void closeTransmission();

private:
    SessionCtx& ctx_;

    int fd_;

    FastVecUchar tag_;
};
