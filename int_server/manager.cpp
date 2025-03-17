#include "manager.h"

#include <unistd.h>

#include <cstdint>

#include "packet.h"

// Раскомментировать для отладки и логирования --------
// #include <iostream>
// ----------------------------------------------------

Manager::Manager(uint16_t port)
{
    _port = port;
}

Manager::~Manager()
{
    closeServer();
}

void Manager::runOnce()
{
    checkServer();

    checkNewClient();

    procTransmissions();
}

void Manager::checkServer()
{
    if (!_tcpChannel.serverIsOpen())
    {
        _tcpChannel.openServer(_port);
        _serverValidateTimer.oneShot(_serverValidateInterval);
    }

    // Раскомментировать для отладки и логирования --------
    // if (!_tcpChannel.serverIsOpen())
    // {
    //     std::cout << "Server socket error: "
    //               << _tcpChannel.getErrorMsg()
    //               << std::endl;
    // }
    // ----------------------------------------------------

    if (_serverValidateTimer.accept() == 0)
        return;

    if (_tcpChannel.validateListenSocket())
    {
        _serverValidateTimer.oneShot(_serverValidateInterval);

        return;
    }

    // Раскомментировать для отладки и логирования --------
    // std::cout << "Server socket error: "
    //           << _tcpChannel.getErrorMsg()
    //           << std::endl;
    // ----------------------------------------------------
}

void Manager::checkNewClient()
{
    int newClientSocket = -1;

    while (_tcpChannel.acceptClient(newClientSocket))
    {
        _files[newClientSocket] = new File();
        _files[newClientSocket]->startTimer();

        // Раскомментировать для отладки и логирования --------
        // std::cout << "New client accepted" << std::endl;
        // ----------------------------------------------------
    }

    // Если нет ни одного подключенного клиента (в т.ч. ранее
    // подключенного, усыпляем поток на некоторое время.
    // Использование epoll, poll или select в данном приложении
    // нецелесообразно
    if (_files.empty())
        usleep(1000000);
}

void Manager::procTransmissions()
{
    for (auto it = _files.begin(); it != _files.end();)
    {
        uint32_t putLen;

        if (!_tcpChannel.receiveData(
                it->first, it->second->inPtr(), packetPutLen, putLen))
        {
            // Раскомментировать для отладки и логирования --------
            // std::cout << "Server socket receive error: "
            //           << _tcpChannel.getErrorMsg()
            //           << std::endl;
            // ----------------------------------------------------

            it->second->dropFile();
            it = closeClient(it);

            continue;
        }

        if (putLen > 0)
        {
            if (it->second->procData(putLen))
            {
                if (!_tcpChannel.sendData(
                        it->first, it->second->outPtr(), packetAnsLen))
                {
                    // Раскомментировать для отладки и логирования --------
                    // std::cout << "Server socket send error: "
                    //           << _tcpChannel.getErrorMsg()
                    //           << std::endl;
                    // ----------------------------------------------------

                    it->second->dropFile();
                    it = closeClient(it);

                    continue;
                }
            }

            it->second->startTimer();
        }

        auto isComplete = it->second->checkComplete();

        // Раскомментировать для отладки и логирования --------
        // std::string msg;

        // if (it->second->getMsg(msg))
        //     std::cout << msg << std::endl;
        // ----------------------------------------------------

        if (it->second->checkComplete())
        {
            it = closeClient(it);

            continue;
        }

        ++it;
    }
}

std::unordered_map<int, File*>::iterator
Manager::closeClient(std::unordered_map<int, File*>::iterator it)
{
    delete it->second;
    it->second = nullptr;

    _tcpChannel.dropClient(it->first);

    return _files.erase(it);
}

void Manager::closeServer()
{
    for (auto& file : _files)
    {
        delete file.second;
        file.second = nullptr;

        _tcpChannel.dropClient(file.first);
    }

    _files.clear();

    _tcpChannel.closeServer();
}
