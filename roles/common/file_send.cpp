/**
 * @file file_send.cpp
 * @brief Implementation of file sending over a transfer channel
 */

#include "file_send.h"

#include <cstring>
#include <cerrno>
#include <cassert>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "crypto.h"

FileSend::FileSend(SessionCtx& ctx)
    : ctx_(ctx)
{
    fd_ = -1;
    tag_.resize(Crypto::tagLen);
}

FileSend::~FileSend()
{
    closeTransmission();
}

Decision FileSend::readySendFile()
{
    assert(fd_ == -1);

    fd_ = open(ctx_.fPath.c_str(), O_RDONLY | O_NOFOLLOW);

    // First open, then do all the other checks, to avoid another process changing the file
    if (fd_ < 0)
    {
        if (errno == ENOENT)
        {
            ctx_.errStr = "[File] Specified file not found";
            return Decision::DenyNoFile;
        }

        if (errno == ELOOP)
        {
            ctx_.errStr = "[File] Specified file is a symbolic link";
            return Decision::DenyNoFile;
        }

        ctx_.errStr = "[File] " + std::string(std::strerror(errno));
        return Decision::Error;
    }

    struct stat fileStat;

    if (fstat(fd_, &fileStat) != 0)
    {
        ctx_.errStr = "[File] " + std::string(std::strerror(errno));
        close(fd_);
        fd_ = -1;
        return Decision::Error;
    }

    if (!S_ISREG(fileStat.st_mode))
    {
        ctx_.errStr = "[File] Specified path is not a regular file";
        close(fd_);
        fd_ = -1;
        return Decision::DenyNoFile;
    }

    if (fileStat.st_size == 0)
    {
        ctx_.errStr = "[File] Specified file's size is zero";
        close(fd_);
        fd_ = -1;
        return Decision::Error;
    }

    ctx_.fSize = fileStat.st_size;
    ctx_.fSizeDone = 0;
    ctx_.percentDone = 0;

    return Decision::Accept;
}

bool FileSend::prepareSendBlocks()
{
    assert(fd_ != -1);
    assert(ctx_.fSizeDone < ctx_.fSize);
    assert(ctx_.blocksOnAir < packetsWindow);

    for (size_t i = 0; i < packetsWindow - ctx_.blocksOnAir; ++i)
    {
        size_t restLen = ctx_.fSize - ctx_.fSizeDone;

        size_t msgLen = (restLen > maxBlockLen) ? maxBlockLen : restLen;

        Frame frame(msgLen + sizeof(BlockSuffix));

        BlockSuffix* suffix = reinterpret_cast<BlockSuffix*>(frame.buf.data() + msgLen);

        size_t bytesLeft = msgLen;
        size_t bytesRead = 0;

        while (bytesLeft > 0)
        {
            auto res = read(fd_, frame.buf.data() + bytesRead, bytesLeft);

            if (res < 0)
            {
                if (errno == EINTR)
                    continue;

                ctx_.errStr = "[File] " + std::string(std::strerror(errno));

                return false;
            }

            if (res == 0)
            {
                ctx_.errStr = "[File] Failed to read file";
                return false;
            }

            bytesRead += static_cast<size_t>(res);
            bytesLeft -= static_cast<size_t>(res);
        }

        if (!Crypto::encryptChaCha20Poly1305(ctx_.secret, ctx_.outNonce(), frame.buf, msgLen, tag_))
        {
            ctx_.errStr = "[File] Failed to encrypt file block";
            return false;
        }

        std::memcpy(suffix->tag, tag_.data(), tag_.size());

        ctx_.outQueue.push(std::move(frame));

        ctx_.fSizeDone += msgLen;
        ++ctx_.blocksOnAir;

        if (ctx_.fSizeDone == ctx_.fSize)
            break;
    }

    return true;
}

bool FileSend::procConfirm()
{
    assert(ctx_.inBuf.size() == sizeof(Confirm));

    Confirm* confirm = reinterpret_cast<Confirm*>(ctx_.inBuf.data());

    std::memcpy(tag_.data(), confirm->tag, sizeof(Confirm::tag));

    if (!Crypto::decryptChaCha20Poly1305(ctx_.secret, ctx_.inNonce(), ctx_.inBuf, confirmMsgLen, tag_))
    {
        ctx_.errStr = "[File] Failed to decrypt confirmation message";
        return false;
    }

    Decision decision;
    std::memcpy(&decision, &confirm->decision, sizeof(Confirm::decision));

    if (decision != Decision::Accept)
    {
        ctx_.errStr = "[File] Wrong confirmation message";
        return false;
    }

    return true;
}

void FileSend::closeTransmission()
{
    close(fd_);
    fd_ = -1;
}
