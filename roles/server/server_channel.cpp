/**
 * @file server_channel.cpp
 * @brief Server-side file transfer channel
 */

#include "server_channel.h"

#include <cstring>
#include <cerrno>
#include <cassert>

#include <sys/epoll.h>

#include "packet.h"

ServerChannel::ServerChannel()
    : handshake_(ctx_),
      task_(ctx_),
      fileSend_(ctx_),
      fileReceive_(ctx_)
{
    ctx_.init();

    state_ = ServerState::NotOpen;
}

ServerChannel::~ServerChannel()
{
    close();
}

bool ServerChannel::open(ServerTcp* tcp, int clientSocket, int epollFd)
{
    assert(state_ == ServerState::NotOpen);

    tcp_ = tcp;
    clientSocket_ = clientSocket;

    epollFd_ = epollFd;

    socketTag_.channel = this;
    socketTag_.type = ServerTag::Type::Socket;

    timerTag_.channel = this;
    timerTag_.type = ServerTag::Type::Timer;

    epoll_event ev = {0};
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    ev.data.ptr = &socketTag_;

    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientSocket_, &ev) == -1)
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
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientSocket_, nullptr);
        return false;
    }

    if (!handshake_.setStaticKeys())
    {
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientSocket_, nullptr);
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, timer_.getTimerFd(), nullptr);
        return false;
    }

    if (!handshake_.setEphemKeys())
    {
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientSocket_, nullptr);
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, timer_.getTimerFd(), nullptr);
        return false;
    }

    state_ = ServerState::WaitHandshakeReq;
    ctx_.inPos = 0;
    ctx_.inBuf.resize(sizeof(HandshakeReq));

    return true;
}

void ServerChannel::close()
{
    if (state_ == ServerState::NotOpen)
        return;

    fileSend_.closeTransmission();
    fileReceive_.closeTransmission();

    // If the channel is open, it is guaranteed to have an epoll descriptor,
    // a socket, and a timer descriptor
    // (the channel must always be closed before the socket is closed)
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientSocket_, nullptr);
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, timer_.getTimerFd(), nullptr);

    state_ = ServerState::NotOpen;
}

ServerState ServerChannel::getState() const
{
    return state_;
}

TaskType ServerChannel::getTaskType() const
{
    if (state_ == ServerState::NotOpen)
        return static_cast<TaskType>(555);

    return ctx_.taskType;
}

Timer& ServerChannel::getTimer()
{
    return timer_;
}

bool ServerChannel::procIncomingData()
{
    assert(state_ != ServerState::NotOpen);

    size_t bytesReceived = 0;

    while (true)
    {
        if (!tcp_->receive(clientSocket_,
                           ctx_.inBuf.data() + ctx_.inPos,
                           ctx_.inBuf.size() - ctx_.inPos,
                           bytesReceived))
        {
            ctx_.errStr = tcp_->getLastErrStr();
            return false;
        }

        // EAGAIN, or the client has closed the connection (see ClientTcp)
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

bool ServerChannel::procOutgoingData()
{
    assert(state_ != ServerState::NotOpen);

    while (!ctx_.outQueue.empty())
    {
        auto& frame = ctx_.outQueue.front();

        size_t bytesSent = 0;

        if (!tcp_->send(clientSocket_, frame.buf.data() + frame.offset, frame.len, bytesSent))
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

std::pair<std::string, bool> ServerChannel::getProgressStr()
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

const std::string& ServerChannel::getLastErrStr() const
{
    return ctx_.errStr;
}

bool ServerChannel::procState()
{
    while (true)
    {
        switch (state_)
        {
        case ServerState::WaitHandshakeReq:
        {
            if (!handshake_.procHandshakeReq())
                return false;

            if (!handshake_.computeSecret())
                return false;

            state_ = ServerState::SendHandshakeRep;

            continue;
        }

        case ServerState::SendHandshakeRep:
        {
            if (!handshake_.prepareHandshakeRep())
                return false;

            if (!procOutgoingData())
                return false;

            state_ = ServerState::WaitTaskReq;
            ctx_.inPos = 0;
            ctx_.inBuf.resize(sizeof(TaskReq));

            return true;
        }

        case ServerState::WaitTaskReq:
        {
            if (!task_.procTaskReq())
                return false;

            state_ = ServerState::SendTaskRep;

            continue;
        }

        case ServerState::SendTaskRep:
        {
            Decision decision;

            if (ctx_.taskType == TaskType::PutFile)
            {
                decision = fileReceive_.allocateReceiveFile();

                if (!task_.prepareTaskRep(decision))
                    return false;

                if (!procOutgoingData())
                    return false;

                if (decision == Decision::Accept)
                {
                    state_ = ServerState::ReceiveFile;
                    ctx_.inPos = 0;
                    ctx_.resizeInBufForNextBlock();

                    return true;
                }
                else
                {
                    return false;
                }
            }

            if (ctx_.taskType == TaskType::GetFile)
            {
                decision = fileSend_.readySendFile();

                if (!task_.prepareTaskRep(decision))
                    return false;

                if (!procOutgoingData())
                    return false;

                if (decision == Decision::Accept)
                {
                    state_ = ServerState::WaitConfirm;
                    ctx_.inPos = 0;
                    ctx_.inBuf.resize(sizeof(Confirm));

                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        case ServerState::WaitConfirm:
        {
            if (!fileSend_.procConfirm())
                return false;

            state_ = ServerState::SendFile;
            ctx_.inPos = 0;
            ctx_.blocksOnAir = 0;

            continue;
        }

        case ServerState::SendFile:
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
                state_ = ServerState::Finished;

                // Nothing else should be received, but the socket must be read from before closing
                // (1 byte - to detect an error or spoofing)
                ctx_.inBuf.resize(1);

                return true;
            }

            // Everything has been sent, but not everything has been confirmed

            return true;
        }

        case ServerState::ReceiveFile:
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

                state_ = ServerState::Finished;

                // Nothing else should be received, but the socket must be read from before closing
                // (1 byte - to detect an error or spoofing)
                ctx_.inBuf.resize(1);

                return true;
            }

            ctx_.resizeInBufForNextBlock();

            return true;
        }

        case ServerState::Finished:
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

        assert(true);
        break;
    }

    return false;
}
