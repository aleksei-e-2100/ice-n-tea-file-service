/**
 * @file session_ctx.cpp
 * @brief Session context for the file transfer channel
 */

#include "session_ctx.h"

#include <cstring>

#include "crypto.h"

Frame::Frame(size_t bufLen)
{
    buf.resize(bufLen);
    len = bufLen;
    offset = 0;
}

void SessionCtx::init()
{
    inNonce_.resize(Crypto::nonceLen);
    std::memset(inNonce_.data(), 0, inNonce_.size());

    outNonce_.resize(Crypto::nonceLen);
    std::memset(outNonce_.data(), 0, outNonce_.size());
}

const FastVecUchar& SessionCtx::inNonce()
{
    size_t len = inNonce_.size();
    size_t byte = 0;

    while (byte < len)
    {
        if (inNonce_[byte] == 0xFF)
        {
            inNonce_[byte] = 0;
            ++byte;
        }
        else
        {
            ++inNonce_[byte];
            break;
        }
    }

    // If byte == len, it means the entire nonce consisted of 0xFF;
    // it will end up becoming 0
    // (this poses a risk to encryption, but it will never happen,
    // given the immense length of the number)

    return inNonce_;
}

const FastVecUchar& SessionCtx::outNonce()
{
    size_t len = outNonce_.size();
    size_t byte = 0;

    while (byte < len)
    {
        if (outNonce_[byte] == 0xFF)
        {
            outNonce_[byte] = 0;
            ++byte;
        }
        else
        {
            ++outNonce_[byte];
            break;
        }
    }

    return outNonce_;
}

void SessionCtx::resizeInBufForNextBlock()
{
    size_t restLen = fSize - fSizeDone;
    size_t nextBlockLen = (restLen > maxBlockLen) ? maxBlockLen : restLen;
    size_t requiredBufLen = nextBlockLen + sizeof(BlockSuffix);

    if (inBuf.size() != requiredBufLen)
        inBuf.resize(requiredBufLen);
}

