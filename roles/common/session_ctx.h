/**
 * @file session_ctx.cpp
 * @brief Session context for the file transfer channel
 */

#pragma once

#include <string>
#include <queue>

#include "packet.h"
#include "fast_vec_uchar.h"

/**
 * @class Frame
 * @brief Data buffer for network transmission
 */
class Frame
{
public:
    /**
     * @brief Constructor
     * @param [in] bufLen Buffer length
     */
    Frame(size_t bufLen);

public:
    FastVecUchar buf;
    size_t len;
    size_t offset;
};

/**
 * @struct SessionCtx
 * @brief Implement session context for the file transfer channel
 */
class SessionCtx
{
public:
    /**
     * @brief Initialize the session context
     */
    void init();

    /**
     * @brief Get the next incoming nonce
     * @return The value of the next incoming nonce
     */
    const FastVecUchar& inNonce();

    /**
     * @brief Get the next outgoing nonce
     * @return The value of the next outgoing nonce
     */
    const FastVecUchar& outNonce();

    /**
      * @brief Adjust the input buffer to the size required for the next block of the file
      */
    void resizeInBufForNextBlock();

public:
    // Task being performed
    TaskType taskType;

    // Peer
    std::string peerName;

    // Encryption secret
    FastVecUchar secret;

    // File transfer context
    std::string fPath;
    std::string fName;
    size_t fSize;
    size_t fSizeDone;
    size_t percentDone;
    size_t blocksOnAir;

    // Network buffers
    std::queue<Frame> outQueue;
    FastVecUchar inBuf;
    size_t inPos;

    // Error status
    std::string errStr;

private:
    // Nonce
    FastVecUchar inNonce_;
    FastVecUchar outNonce_;
};
