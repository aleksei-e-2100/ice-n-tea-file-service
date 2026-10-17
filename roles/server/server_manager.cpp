/**
 * @file server_manager.cpp
 * @brief Server manager. Manages the file transfer channels.
 */

#include "server_manager.h"

#include <iostream>
#include <unordered_set>
#include <cstring>
#include <cerrno>

#include <unistd.h>

#include "config.h"
#include "endpoint.h"

ServerManager::ServerManager()
{
    epollFd_ = -1;
}

ServerManager::~ServerManager()
{
    close();
}

bool ServerManager::open()
{
    auto& entry = Config::data().at(Config::selfName());

    if (!entry.isServer)
    {
        showErrStr("[Argument] Self server mode is not configured");
        return false;
    }

    Endpoint endpoint;
    endpoint.setFromStr(entry.ip.c_str(), entry.port);

    if (!tcp_.open(endpoint))
    {
        showErrStr(tcp_.getLastErrStr());
        return false;
    }

    epollFd_ = epoll_create1(0);

    if (epollFd_ == -1)
    {
        showErrStr("[Epoll] " + std::string(std::strerror(errno)));
        tcp_.close();
        return false;
    }

    epoll_event ev = {0};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = this;

    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, tcp_.getListenSocket(), &ev) == -1)
    {
        showErrStr("[Epoll] " + std::string(std::strerror(errno)));
        ::close(epollFd_);
        tcp_.close();
        return false;
    }

    return true;
}

void ServerManager::close()
{
    for (auto it = channelsSockets_.begin(); it != channelsSockets_.end(); )
    {
        delete it->first;
        tcp_.dropClient(it->second);
        it = channelsSockets_.erase(it);
    }

    ::close(epollFd_);
    epollFd_ = -1;

    tcp_.close();
}

ServerManagerResult ServerManager::runOnce()
{
    int fdReady = epoll_wait(epollFd_, eventsPool_, maxEvents_, waitTime_);

    if (fdReady == -1)
    {
        if (errno == EINTR)
            return ServerManagerResult::Continue;

        showErrStr("[Epoll] " + std::string(std::strerror(errno)));

        return ServerManagerResult::Error;
    }

    if (fdReady == 0)
        return ServerManagerResult::Continue;


    std::unordered_set<ServerChannel*> channelsToClose;

    // First, check only the timers
    for (size_t i = 0; i < fdReady; ++i)
    {
        if (eventsPool_[i].data.ptr == this)
            continue;

        ServerTag* serverTag = reinterpret_cast<ServerTag*>(eventsPool_[i].data.ptr);

        if (serverTag->type != ServerTag::Type::Timer)
            continue;

        ServerChannel* channel = serverTag->channel;

        channel->getTimer().accept();
        showErrStr("[Channel] Client response timeout");

        channelsToClose.insert(channel);
    }

    // Then check sockets
    for (size_t i = 0; i < fdReady; ++i)
    {
        // Listening socket
        if (eventsPool_[i].data.ptr == this)
        {
            if (eventsPool_[i].events & EPOLLIN)
                procNewClients();

            if (eventsPool_[i].events & (EPOLLHUP | EPOLLERR))
            {
                showErrStr("[Epoll] Listening socket failure");
                return ServerManagerResult::Error;
            }

            continue;
        }

        ServerTag* serverTag = reinterpret_cast<ServerTag*>(eventsPool_[i].data.ptr);
        ServerChannel* channel = serverTag->channel;

        // The channel might already be set for closing via a timer
        if (channelsToClose.contains(channel))
            continue;

        bool needClose = false;

        // If there are connection issues
        if (eventsPool_[i].events & (EPOLLHUP | EPOLLERR))
        {
            showErrStr("[Channel] Connection terminated by client");
            needClose = true;
        }

        // Read the socket completely
        ServerState prevState = channel->getState();

        if (!needClose && eventsPool_[i].events & EPOLLIN)
        {
            if (!channel->procIncomingData())
            {
                showErrStr(channel->getLastErrStr());
                needClose = true;
            }
        }

        // If the client has closed the connection
        // (the connection can only be closed by the sender, and only after the receiver
        // has confirmed receipt of the file in its entirety)
        if (!needClose && eventsPool_[i].events & EPOLLRDHUP)
        {
            if (!(prevState == ServerState::Finished &&
                channel->getTaskType() == TaskType::PutFile))
            {
                showErrStr("[Channel] Client closed connection before the transfer completed");
            }

            needClose = true;
        }

        // If there is something waiting to be sent
        if (!needClose && eventsPool_[i].events & EPOLLOUT)
        {
            if (!channel->procOutgoingData())
            {
                showErrStr(channel->getLastErrStr());
                needClose = true;
            }
        }

        // If it is time to send a FIN
        // (this can only be done by the sender after receiving confirmation
        // that the entire file has been received, i.e., when in the ServerState::Finished state)
        if (!needClose &&
            channel->getState() == ServerState::Finished &&
            channel->getTaskType() == TaskType::GetFile)
        {
            needClose = true;
        }

        // Output progress
        if (!needClose &&
            (channel->getState() == ServerState::SendFile ||
             channel->getState() == ServerState::ReceiveFile ||
             channel->getState() == ServerState::Finished))
        {
            auto progress = channel->getProgressStr();

            if (progress.second)
                showProgressStr(progress.first);
        }

        if (needClose)
            channelsToClose.insert(channel);
    }

    for (auto channel : channelsToClose)
    {
        // delete will call close() on the channel
        delete channel;

        // Despite the `delete` call above, the entry remains in the map
        // (a pointer is just a number; and there is no accessing the channel's
        // fields, only the map itself)
        int clientSocket = channelsSockets_[channel];
        tcp_.dropClient(clientSocket);

        // Erase the channel from the map
        channelsSockets_.erase(channel);
    }

    return ServerManagerResult::Continue;
}

bool ServerManager::procNewClients()
{
    int clientSocket = -1;

    while (tcp_.acceptClientent(clientSocket))
    {
        ServerChannel* channel = new ServerChannel();

        if (!channel->open(&tcp_, clientSocket, epollFd_))
        {
            showErrStr(channel->getLastErrStr());
            delete channel;

            continue;
        }

        channelsSockets_[channel] = clientSocket;
    }

    return true;
}

void ServerManager::showProgressStr(const std::string& progressStr) const
{
    std::cout << progressStr << std::endl;
}

void ServerManager::showErrStr(const std::string& errStr) const
{
    std::cout << errStr << std::endl;
}
