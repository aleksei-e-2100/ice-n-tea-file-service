/**
 * @file client_manager.cpp
 * @brief Client manager. Manages the file transfer channel.
 */

#include "client_manager.h"

#include <iostream>
#include <filesystem>
#include <cstring>
#include <cerrno>

#include <unistd.h>

#include "config.h"
#include "endpoint.h"

ClientManager::ClientManager()
{
    epollFd_ = -1;
}

ClientManager::~ClientManager()
{
    close();
}

bool ClientManager::open(WorkMode workMode,
                         const std::string& fileArg,
                         const std::string& serverName)
{
    // Check the file argument
    std::string fPath;
    std::string fName;
    size_t fSize;

    if (workMode == WorkMode::SendFile)
    {
        try
        {
            std::filesystem::path path(fileArg);
            std::filesystem::file_status status = std::filesystem::symlink_status(path);

            if (!std::filesystem::exists(status))
            {
                showErrStr("[Argument] Specified file not found");
                return false;
            }

            if (std::filesystem::is_symlink(status))
            {
                showErrStr("[Argument] Specified file is a symbolic link");
                return false;
            }

            if (!std::filesystem::is_regular_file(path))
            {
                showErrStr("[Argument] Specified path is not a regular file");
                return false;
            }

            fSize = std::filesystem::file_size(path);

            if (fSize == 0)
            {
                showErrStr("[Argument] Specified file's size is zero");
                return false;
            }

            fPath = std::filesystem::absolute(path).string();
            fName = path.filename().string();
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            showErrStr("[Argument] " + std::string(e.what()));
            return false;
        }
    }

    else if (workMode == WorkMode::ReceiveFile)
    {
        if ((fileArg.empty()) ||
            (fileArg == ".") ||
            (fileArg == "..") ||
            (fileArg.find_first_of('/', 0) != std::string::npos))
        {
            showErrStr("[Argument] Malformed file name");
            return false;
        }

        fName = fileArg;
        fPath = Config::data().at(Config::selfName()).dir + fName;
        fSize = 0;
    }

    // Server check in configuration
    if (serverName == Config::selfName())
    {
        showErrStr("[Argument] Self-connections are not allowed");
        return false;
    }

    if (!Config::data().contains(serverName))
    {
        showErrStr("[Argument] Server configuration not found");
        return false;
    }

    auto& entry = Config::data().at(serverName);

    if (!entry.isServer)
    {
        showErrStr("[Argument] Unknown server");
        return false;
    }

    // Open TCP
    Endpoint endpoint;
    endpoint.setFromStr(entry.ip.c_str(), entry.port);

    if (!tcp_.open(endpoint))
    {
        showErrStr(tcp_.getLastErrStr());
        return false;
    }

    // Adjust epoll
    epollFd_ = epoll_create1(0);

    if (epollFd_ == -1)
    {
        showErrStr("[Epoll] " + std::string(std::strerror(errno)));
        tcp_.close();
        return false;
    }

    // Open channel
    if (!channel_.open(workMode, serverName, fPath, fName, fSize, entry.key, &tcp_, epollFd_))
    {
        showErrStr(channel_.getLastErrStr());
        ::close(epollFd_);
        epollFd_ = -1;
        tcp_.close();
        return false;
    }

    return true;
}

void ClientManager::close()
{
    channel_.close();
    ::close(epollFd_);
    epollFd_ = -1;
    tcp_.close();
}

ClientManagerResult ClientManager::runOnce()
{
    int fdReady = epoll_wait(epollFd_, eventsPool_, maxEvents_, waitTime_);

    if (fdReady == -1)
    {
        if (errno == EINTR)
            return ClientManagerResult::Continue;

        showErrStr("[Epoll] " + std::string(std::strerror(errno)));

        return ClientManagerResult::Error;
    }

    if (fdReady == 0)
        return ClientManagerResult::Continue;

    // First, check only the timer
    for (size_t i = 0; i < fdReady; ++i)
    {
        if (eventsPool_[i].data.ptr == this)
            continue;

        ClientTag* clientTag = reinterpret_cast<ClientTag*>(eventsPool_[i].data.ptr);

        if (clientTag->type != ClientTag::Type::Timer)
            continue;

        ClientChannel* channel = clientTag->channel;

        channel->getTimer().accept();
        showErrStr("[Channel] Server response timeout");

        return ClientManagerResult::Error;
    }

    // There will always be exactly one element in eventsPool_ here, because if only the timer
    // triggered (or both the timer and the socket triggered) the function would have
    // already returned above. If execution reaches this point, it is guaranteed that
    // the socket has triggered (eventsPool_[0]).

    // If there are connection issues
    if (eventsPool_[0].events & (EPOLLHUP | EPOLLERR))
    {
        showErrStr("[Channel] Connection terminated by server");
        return ClientManagerResult::Error;
    }

    // Read the socket completely
    ClientState prevState = channel_.getState();

    if (eventsPool_[0].events & EPOLLIN)
    {
        if (!channel_.procIncomingData())
        {
            showErrStr(channel_.getLastErrStr());
            return ClientManagerResult::Error;
        }
    }

    // If the server has closed the connection
    // (the connection can only be closed by the sender, and only after the receiver
    // has confirmed receipt of the file in its entirety)
    if (eventsPool_[0].events & EPOLLRDHUP)
    {
        // It is important that ClientState::Finished is already set before processing EPOLLIN
        // (otherwise, it would imply that the receiver had not yet sent all confirmations,
        // yet a FIN had already arrived, which indicates an error or spoofing)
        if (prevState == ClientState::Finished &&
            channel_.getTaskType() == TaskType::GetFile)
        {
            return ClientManagerResult::Finished;
        }
        else
        {
            showErrStr("[Channel] Server closed connection before the transfer completed");
            return ClientManagerResult::Error;
        }
    }

    // If there is something waiting to be sent
    if (eventsPool_[0].events & EPOLLOUT)
    {
        if (!channel_.procOutgoingData())
        {
            showErrStr(channel_.getLastErrStr());
            return ClientManagerResult::Error;
        }
    }

    // If it is time to send a FIN
    // (this can only be done by the sender after receiving confirmation
    // that the entire file has been received, i.e., when in the ClientState::Finished state)
    if (channel_.getState() == ClientState::Finished &&
        channel_.getTaskType() == TaskType::PutFile)
    {
        return ClientManagerResult::Finished;
    }

    // Output progress
    if (channel_.getState() == ClientState::SendFile ||
        channel_.getState() == ClientState::ReceiveFile ||
        channel_.getState() == ClientState::Finished)
    {
        auto progress = channel_.getProgressStr();

        if (progress.second)
            showProgressStr(progress.first);
    }

    return ClientManagerResult::Continue;
}

void ClientManager::showProgressStr(const std::string& progressStr) const
{
    std::cout << progressStr << std::endl;
}

void ClientManager::showErrStr(const std::string& errStr) const
{
    std::cout << errStr << std::endl;
}
