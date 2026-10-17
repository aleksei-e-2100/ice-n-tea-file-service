/**
 * @file client_task.cpp
 * @brief Client-side component for negotiating a file transfer task
 */

#include "client_task.h"

#include <cstring>
#include <cassert>

#include "packet.h"
#include "crypto.h"

ClientTask::ClientTask(SessionCtx& ctx)
    : ctx_(ctx)
{
    tag_.resize(Crypto::tagLen);
}

bool ClientTask::prepareTaskReq()
{
    assert(ctx_.fName.length() < sizeof(TaskReq::fName));

    Frame frame(sizeof(TaskReq));

    TaskReq* request = reinterpret_cast<TaskReq*>(frame.buf.data());
    std::memset(request, 0, taskReqMsgLen);

    std::memcpy(&request->taskType, &ctx_.taskType, sizeof(ctx_.taskType));
    std::memcpy(request->fName, ctx_.fName.data(), ctx_.fName.length());
    std::memcpy(&request->fSize, &ctx_.fSize, sizeof(ctx_.fSize));

    if (!Crypto::encryptChaCha20Poly1305(ctx_.secret, ctx_.outNonce(), frame.buf, taskReqMsgLen, tag_))
    {
        ctx_.errStr = "[Task] Failed to encrypt request";
        return false;
    }

    std::memcpy(request->tag, tag_.data(), tag_.size());

    ctx_.outQueue.push(std::move(frame));

    return true;
}

bool ClientTask::procTaskRep()
{
    assert(ctx_.inBuf.size() == sizeof(TaskRep));

    TaskRep* reply = reinterpret_cast<TaskRep*>(ctx_.inBuf.data());

    std::memcpy(tag_.data(), reply->tag, sizeof(TaskRep::tag));

    if (!Crypto::decryptChaCha20Poly1305(ctx_.secret, ctx_.inNonce(), ctx_.inBuf, taskRepMsgLen, tag_))
    {
        ctx_.errStr = "[Task] Failed to decrypt reply";
        return false;
    }

    Decision decision;
    std::memcpy(&decision, &reply->decision, sizeof(TaskRep::decision));

    if (!(decision == Decision::Accept) &&
        !(decision == Decision::Error) &&
        !(ctx_.taskType == TaskType::PutFile && decision == Decision::DenyNoSpace) &&
        !(ctx_.taskType == TaskType::GetFile && decision == Decision::DenyNoFile))
    {
        ctx_.errStr = "[Task] Unexpected server decision in reply";
        return false;
    }

    if (ctx_.taskType == TaskType::GetFile)
    {
        if (decision == Decision::DenyNoFile)
        {
            ctx_.errStr = "[Task] Requested file not found on server";
            return false;
        }

        if (decision == Decision::Error)
        {
            ctx_.errStr = "[Task] Server failed to download file";
            return false;
        }

        std::memcpy(&ctx_.fSize, &reply->fSize, sizeof(TaskRep::fSize));

        if (ctx_.fSize == 0)
        {
            ctx_.errStr = "[Task] Server reported an invalid file size";
            return false;
        }
    }

    else if (ctx_.taskType == TaskType::PutFile)
    {
        if (decision == Decision::DenyNoSpace)
        {
            ctx_.errStr = "[Task] Insufficient disk space on server";
            return false;
        }

        if (decision == Decision::Error)
        {
            ctx_.errStr = "[Task] Server failed to upload file";
            return false;
        }
    }

    return true;
}
