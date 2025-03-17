#pragma once

#include <cstdint>
#include <unordered_map>

#include "file.h"
#include "tcpchannel.h"
#include "timer.h"

class Manager
{
public:
    Manager(uint16_t port);

    ~Manager();

    // Не допускаем перемещение и копирование
    Manager(Manager&) = delete;
    Manager& operator=(Manager&) = delete;
    Manager(Manager&&) = delete;
    Manager& operator=(Manager&&) = delete;

public:
    void runOnce();

private:
    void checkServer();

    void checkNewClient();

    void procTransmissions();

    std::unordered_map<int, File*>::iterator
    closeClient(std::unordered_map<int, File*>::iterator it);

    void closeServer();

private:
    TcpChannel _tcpChannel;
    uint16_t _port;

    Timer _serverValidateTimer;
    const int _serverValidateInterval = 180000;

    std::unordered_map<int, File*> _files;
};
