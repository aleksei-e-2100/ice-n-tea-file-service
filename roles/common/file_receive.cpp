/**
 * @file file_receive.cpp
 * @brief Implementation of file receiving over a transfer channel
 */

#include "file_receive.h"

#include <string_view>
#include <random>
#include <cstring>
#include <cerrno>
#include <cassert>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "crypto.h"
#include "config.h"

namespace
{
    constexpr std::string_view alphabet =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
}

FileReceive::FileReceive(SessionCtx &ctx)
    : ctx_(ctx)
{
    fd_ = -1;
    tag_.resize(Crypto::tagLen);
}

FileReceive::~FileReceive()
{
    closeTransmission();
}

Decision FileReceive::allocateReceiveFile()
{
    assert(fd_ == -1);

    const int maxAttepts = 25;

    for (int i = 0; i < maxAttepts; ++i)
    {
        tmpFpath_ = Config::data().at(Config::selfName()).dir + generateRandomName();

        // Will not overwrite an already existing file
        fd_ = open(tmpFpath_.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);

        if (fd_ < 0)
        {
            if (errno == EEXIST)
            {
                if (i < maxAttepts - 1)
                    continue;
            }

            ctx_.errStr = "[File] " + std::string(std::strerror(errno));

            return Decision::Error;
        }

        break;
    }

    auto res = posix_fallocate(fd_, 0, ctx_.fSize);

    if (res != 0)
    {
        Decision decision = (errno == ENOSPC) ? Decision::DenyNoSpace : Decision::Error;
        ctx_.errStr = "[File] " + std::string(std::strerror(res));
        close(fd_);
        unlink(tmpFpath_.c_str());
        tmpFpath_.clear();

        return decision;
    }

    ctx_.fSizeDone = 0;
    ctx_.percentDone = 0;

    return Decision::Accept;
}

bool FileReceive::procReceiveBlock()
{
    assert(fd_ != -1);
    assert(ctx_.inBuf.size() <= (ctx_.fSize - ctx_.fSizeDone + sizeof(BlockSuffix)));

    size_t msgLen = ctx_.inBuf.size() - sizeof(BlockSuffix);

    BlockSuffix* suffix = reinterpret_cast<BlockSuffix*>(ctx_.inBuf.data() + msgLen);

    std::memcpy(tag_.data(), suffix->tag, sizeof(BlockSuffix::tag));

    if (!Crypto::decryptChaCha20Poly1305(ctx_.secret, ctx_.inNonce(), ctx_.inBuf, msgLen, tag_))
    {
        ctx_.errStr = "[File] Failed to decrypt received block";
        return false;
    }

    size_t bytesLeft = msgLen;
    size_t bytesWritten = 0;

    while (bytesLeft > 0)
    {
        auto res = write(fd_, ctx_.inBuf.data() + bytesWritten, bytesLeft);

        if (res < 0)
        {
            if (errno == EINTR)
                continue;

            ctx_.errStr = "[File] " + std::string(std::strerror(errno));

            return false;
        }

        if (res == 0)
        {
            ctx_.errStr = "[File] Failed to write file";
            return false;
        }

        bytesWritten += static_cast<size_t>(res);
        bytesLeft -= static_cast<size_t>(res);
    }

    ctx_.fSizeDone += msgLen;

    return true;
}

bool FileReceive::prepareConfirm()
{
    Frame frame(sizeof(Confirm));

    Confirm* confirm = reinterpret_cast<Confirm*>(frame.buf.data());
    std::memset(confirm, 0, confirmMsgLen);

    Decision decision = Decision::Accept;
    std::memcpy(&confirm->decision, &decision, sizeof(Confirm::decision));

    if (!Crypto::encryptChaCha20Poly1305(ctx_.secret, ctx_.outNonce(), frame.buf, confirmMsgLen, tag_))
    {
        ctx_.errStr = "[File] Failed to encrypt confirmation message";
        return false;
    }

    std::memcpy(confirm->tag, tag_.data(), tag_.size());

    ctx_.outQueue.push(std::move(frame));

    return true;
}

bool FileReceive::commitReceiveFile()
{
    assert(ctx_.fSize == ctx_.fSizeDone);

    close(fd_);
    fd_ = -1;

    int res = rename(tmpFpath_.c_str(), ctx_.fPath.c_str());

    if (res != 0)
    {
        ctx_.errStr = "[File] " + std::string(std::strerror(errno));
        unlink(tmpFpath_.c_str());
    }

    tmpFpath_.clear();

    return (res == 0);
}

void FileReceive::closeTransmission()
{
    close(fd_);
    fd_ = -1;

    if (!tmpFpath_.empty())
    {
        unlink(tmpFpath_.c_str());
        tmpFpath_.clear();
    }
}

const std::string FileReceive::generateRandomName() const
{
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<size_t> dis(0, alphabet.size() - 1);

    std::string name;
    name.resize(15);

    name[0] = '_';

    for (auto i = 1; i < name.length(); ++i)
    {
        name[i] = alphabet[dis(gen)];
    }

    name += ".recv_tmp";

    // Do not use std::move. The compiler will return the value constructed inside the function
    return name;
}
