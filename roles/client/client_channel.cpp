/**
 * @file client_channel.cpp
 * @brief Client-side file transfer channel
 */

#include "client_channel.h"

#include <cstring>
#include <cerrno>
#include <cassert>

#include <sys/epoll.h>

#include "packet.h"

ClientChannel::ClientChannel()
    : handshake_(ctx_),
      task_(ctx_),
      fileSend_(ctx_),
      fileReceive_(ctx_)
{
    ctx_.init();

    state_ = ClientState::NotOpen;
}

ClientChannel::~ClientChannel()
{
    close();
}

bool ClientChannel::open(WorkMode workMode,
                         const std::string &serverName,
                         const std::string& fPath,
                         const std::string& fName,
                         size_t fSize,
                         const std::string& serverStaticPubKey,
                         ClientTcp* tcp,
                         int epollFd)
{
    assert(state_ == ClientState::NotOpen);

    if (workMode == WorkMode::SendFile)
        ctx_.taskType = TaskType::PutFile;
    else if (workMode == WorkMode::ReceiveFile)
        ctx_.taskType = TaskType::GetFile;

    ctx_.peerName = serverName;

    ctx_.fPath = fPath;
    ctx_.fName = fName;
    ctx_.fSize = fSize;

    tcp_ = tcp;

    epollFd_ = epollFd;

    socketTag_.channel = this;
    socketTag_.type = ClientTag::Type::Socket;

    timerTag_.channel = this;
    timerTag_.type = ClientTag::Type::Timer;

    epoll_event ev = {0};
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    ev.data.ptr = &socketTag_;

    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, tcp_->getSocket(), &ev) == -1)
    {
        ctx_.errStr = "[Epoll] " + std::string(std::strerror(errno));
        return false;
    }

    ev = {0};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = &timerTag_;

    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, timer_.getTimerFd(), &ev) == -1)
    {
        ctx_.errStr = "[Epoll] " + std::string(std::strerror(errno));
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, tcp_->getSocket(), nullptr);
        return false;
    }

    if (!handshake_.setStaticKeys(serverStaticPubKey))
    {
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, tcp_->getSocket(), nullptr);
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, timer_.getTimerFd(), nullptr);
        return false;
    }

    if (!handshake_.setEphemKeys())
    {
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, tcp_->getSocket(), nullptr);
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, timer_.getTimerFd(), nullptr);
        return false;
    }

    state_ = ClientState::SendHandshakeReq;

    return procState();
}

void ClientChannel::close()
{
    if (state_ == ClientState::NotOpen)
        return;

    fileSend_.closeTransmission();
    fileReceive_.closeTransmission();

    // If the channel is open, it is guaranteed to have an epoll descriptor,
    // a socket, and a timer descriptor
    // (the channel must always be closed before the socket is closed)
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, tcp_->getSocket(), nullptr);
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, timer_.getTimerFd(), nullptr);

    state_ = ClientState::NotOpen;
}

ClientState ClientChannel::getState() const
{
    return state_;
}

TaskType ClientChannel::getTaskType() const
{
    if (state_ == ClientState::NotOpen)
        return static_cast<TaskType>(555);

    return ctx_.taskType;
}

Timer& ClientChannel::getTimer()
{
    return timer_;
}

bool ClientChannel::procIncomingData()
{
    assert(state_ != ClientState::NotOpen);

    size_t bytesReceived = 0;

    while (true)
    {
        if (!tcp_->receive(ctx_.inBuf.data() + ctx_.inPos,
                           ctx_.inBuf.size() - ctx_.inPos,
                           bytesReceived))
        {
            ctx_.errStr = tcp_->getLastErrStr();
            return false;
        }

        // EAGAIN, or the server has closed the connection (see ClientTcp)
        if (bytesReceived == 0)
            break;

        ctx_.inPos += bytesReceived;

        if (ctx_.inPos < ctx_.inBuf.size())
            break;

        if (!procState())
            return false;
    }

    return true;
}

bool ClientChannel::procOutgoingData()
{
    assert(state_ != ClientState::NotOpen);

    while (!ctx_.outQueue.empty())
    {
        auto& frame = ctx_.outQueue.front();

        size_t bytesSent = 0;

        if (!tcp_->send(frame.buf.data() + frame.offset, frame.len, bytesSent))
        {
            ctx_.errStr = tcp_->getLastErrStr();
            return false;
        }

        frame.offset += bytesSent;
        frame.len -= bytesSent;

        if (frame.len == 0)
        {
            ctx_.outQueue.pop();

            timer_.accept();
            timer_.oneShot(ansTimeout);

            continue;
        }

        // If frame.len > 0, it means the send operation was error-free but incomplete
        // (the socket cannot handle more right now, so will send the rest next time)
        break;
    }

    return true;
}

std::pair<std::string, bool> ClientChannel::getProgressStr()
{
    assert (ctx_.fSize != 0);

    size_t tmpPercentDone = (ctx_.fSizeDone * 100 / ctx_.fSize) / 10 * 10;

    if (tmpPercentDone > ctx_.percentDone)
    {
        ctx_.percentDone = tmpPercentDone;

        std::string progressStr =
            "[Peer \"" + ctx_.peerName + "\" " + ctx_.fName + "] Done " +
            std::to_string(ctx_.percentDone) + "%";

        return {progressStr, true};
    }

    return {"", false};
}

const std::string& ClientChannel::getLastErrStr() const
{
    return ctx_.errStr;
}

bool ClientChannel::procState()
{
    while (true)
    {
        switch (state_)
        {
        case ClientState::SendHandshakeReq:
        {
            if (!handshake_.prepareHandshakeReq())
                return false;

            if (!procOutgoingData())
                return false;

            state_ = ClientState::WaitHandshakeRep;
            ctx_.inPos = 0;
            ctx_.inBuf.resize(sizeof(HandshakeRep));

            return true;
        }

        case ClientState::WaitHandshakeRep:
        {
            if (!handshake_.procHandshakeRep())
                return false;

            if (!handshake_.computeSecret())
                return false;

            state_ = ClientState::SendTaskReq;

            continue;
        }

        case ClientState::SendTaskReq:
        {
            if (!task_.prepareTaskReq())
                return false;

            if (!procOutgoingData())
                return false;

            state_ = ClientState::WaitTaskRep;
            ctx_.inPos = 0;
            ctx_.inBuf.resize(sizeof(TaskRep));

            return true;
        }

        case ClientState::WaitTaskRep:
        {
            if (!task_.procTaskRep())
                return false;

            if (ctx_.taskType == TaskType::PutFile)
            {
                if (fileSend_.readySendFile() != Decision::Accept)
                    return false;

                state_ = ClientState::SendFile;
                ctx_.inPos = 0;
                ctx_.inBuf.resize(sizeof(Confirm));
                ctx_.blocksOnAir = 0;

                continue;
            }

            if (ctx_.taskType == TaskType::GetFile)
            {
                if (fileReceive_.allocateReceiveFile() != Decision::Accept)
                    return false;

                state_ = ClientState::SendConfirm;

                continue;
            }
        }

        case ClientState::SendConfirm:
        {
            if (!fileReceive_.prepareConfirm())
                return false;

            if (!procOutgoingData())
                return false;

            state_ = ClientState::ReceiveFile;
            ctx_.inPos = 0;
            ctx_.resizeInBufForNextBlock();

            return true;
        }

        case ClientState::SendFile:
        {
            // If a confirmation of receipt has been received
            if (ctx_.inPos == ctx_.inBuf.size())
            {
                if (!fileSend_.procConfirm())
                    return false;

                ctx_.inPos = 0;
                --ctx_.blocksOnAir;
            }

            // If not everything has been sent and there is an available window
            if (ctx_.fSizeDone < ctx_.fSize && ctx_.blocksOnAir < packetsWindow)
            {
                if (!fileSend_.prepareSendBlocks())
                    return false;

                if (!procOutgoingData())
                    return false;

                return true;;
            }

            // If everything has been sent and confirmed
            if (ctx_.fSizeDone == ctx_.fSize && ctx_.blocksOnAir == 0)
            {
                state_ = ClientState::Finished;

                // Nothing else should be received, but the socket must be read from before closing
                // (1 byte - to detect an error or spoofing)
                ctx_.inBuf.resize(1);

                return true;
            }

            // Everything has been sent, but not everything has been confirmed

            return true;
        }

        case ClientState::ReceiveFile:
        {
            if (!fileReceive_.procReceiveBlock())
                return false;

            if (!fileReceive_.prepareConfirm())
                return false;

            if (!procOutgoingData())
                return false;

            ctx_.inPos = 0;

            // If everything has received
            if (ctx_.fSizeDone == ctx_.fSize)
            {
                if (!fileReceive_.commitReceiveFile())
                    return false;

                state_ = ClientState::Finished;

                // Nothing else should be received, but the socket must be read from before closing
                // (1 byte - to detect an error or spoofing)
                ctx_.inBuf.resize(1);

                return true;
            }

            ctx_.resizeInBufForNextBlock();

            return true;
        }

        case ClientState::Finished:
        {
            // If there is still data in the socket at this state
            if (ctx_.inPos == ctx_.inBuf.size())
                return false;
        }

        default:
        {
            break;
        }
        }

        break;
    }

    return false;
}
