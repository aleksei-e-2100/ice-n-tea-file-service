/**
 * @file file_receive.h
 * @brief Implementation of file receiving over a transfer channel
 */

#pragma once

#include <string>

#include "packet.h"
#include "session_ctx.h"
#include "fast_vec_uchar.h"

/**
 * @class FileReceive
 * @brief Implements file receiving over a transfer channel
 */
class FileReceive
{
public:
    /**
     * @brief Constructor
     * @param [in] ctx Session context
     */
    FileReceive(SessionCtx& ctx);

    /**
     * @brief Destructor
     */
    ~FileReceive();

    // Assignment and transfer are prohibited
    FileReceive(const FileReceive&) = delete;
    FileReceive& operator=(const FileReceive&) = delete;
    FileReceive(FileReceive&&) = delete;
    FileReceive& operator=(FileReceive&&) = delete;

    /**
    * @brief Prepare space to receive a file
    * @return Decision on whether the file can be received
    */
    Decision allocateReceiveFile();

    /**
     * @brief Prepare confirmation of file receipt (or readiness to receive)
     * @return true - if the confirmation was prepared successfully
     */
    bool prepareConfirm();

    /**
     * @brief Process the received file block
     * @return true - if the block was processed successfully
     */
    bool procReceiveBlock();

    /**
     * @brief Save the received file
     * @return true - if the file was saved successfully
     */
    bool commitReceiveFile();

    /**
     * @brief Close file receiving
     */
    void closeTransmission();

private:
    const std::string generateRandomName() const;

private:
    SessionCtx& ctx_;

    int fd_;
    std::string tmpFpath_;

    FastVecUchar tag_;
};
