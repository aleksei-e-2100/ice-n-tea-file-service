/**
 * @file server_task.cpp
 * @brief Server-side component for negotiating a file transfer task
 */

#include "server_task.h"

#include <cstring>
#include <cassert>

#include "packet.h"
#include "crypto.h"
#include "config.h"

ServerTask::ServerTask(SessionCtx& ctx)
    : ctx_(ctx)
{
    tag_.resize(Crypto::tagLen);
}

bool ServerTask::procTaskReq()
{
    assert(ctx_.inBuf.size() == sizeof(TaskReq));

    TaskReq* request = reinterpret_cast<TaskReq*>(ctx_.inBuf.data());

    std::memcpy(tag_.data(), request->tag, sizeof(TaskRep::tag));

    if (!Crypto::decryptChaCha20Poly1305(ctx_.secret, ctx_.inNonce(), ctx_.inBuf, taskReqMsgLen, tag_))
    {
        ctx_.errStr = "[Task] Failed to decrypt request";
        return false;
    }

    std::memcpy(&ctx_.taskType, &request->taskType, sizeof(TaskReq::taskType));

    if (ctx_.taskType != TaskType::PutFile && ctx_.taskType != TaskType::GetFile)
    {
        ctx_.errStr = "[Task] Unexpected task type in request";
        return false;
    }

    if (request->fName[sizeof(TaskReq::fName) - 1] != '\0')
    {
        ctx_.errStr = "[Task] Malformed file name in request";
        return false;
    }

    ctx_.fName = request->fName;

    if ((ctx_.fName.empty()) ||
        (ctx_.fName == ".") ||
        (ctx_.fName == "..") ||
        (ctx_.fName.find_first_of('/', 0) != std::string::npos))
    {
        ctx_.errStr = "[Task] Malformed file name in request";
        return false;
    }

    std::memcpy(&ctx_.fSize, &request->fSize, sizeof(TaskReq::fSize));

    if (ctx_.taskType == TaskType::PutFile && ctx_.fSize == 0)
    {
        ctx_.errStr = "[Task] Malformed file size value in request";
        return false;
    }

    ctx_.fPath = Config::data().at(Config::selfName()).dir + ctx_.fName;

    return true;
}

bool ServerTask::prepareTaskRep(Decision decision)
{
    Frame frame(sizeof(TaskRep));

    TaskRep* reply = reinterpret_cast<TaskRep*>(frame.buf.data());
    std::memset(reply, 0, taskRepMsgLen);

    std::memcpy(&reply->decision, &decision, sizeof(decision));

    if (decision == Decision::Accept && ctx_.taskType == TaskType::GetFile)
        std::memcpy(&reply->fSize, &ctx_.fSize, sizeof(reply->fSize));

    if (!Crypto::encryptChaCha20Poly1305(ctx_.secret, ctx_.outNonce(), frame.buf, taskRepMsgLen, tag_))
    {
        ctx_.errStr = "[Task] Failed to encrypt reply";
        return false;
    }

    std::memcpy(reply->tag, tag_.data(), tag_.size());

    ctx_.outQueue.push(std::move(frame));

    return true;
}
