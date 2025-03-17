#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>

class TcpChannel
{
public:
    TcpChannel() = default;

    ~TcpChannel();

    // Не допускаем перемещение и копирование
    TcpChannel(TcpChannel&) = delete;
    TcpChannel& operator=(TcpChannel&) = delete;
    TcpChannel(TcpChannel&&) = delete;
    TcpChannel& operator=(TcpChannel&&) = delete;

public:
    bool openServer(const uint16_t port);

    void closeServer();

    bool acceptClient(int& clientSocket);

    bool dropClient(int clientSocket);

    bool sendData(int clientSocket,
                  const unsigned char* buf,
                  uint32_t len);

    bool receiveData(int clientSocket,
                     unsigned char* buf,
                     uint32_t maxLen,
                     uint32_t& len);

    bool serverIsOpen();

    bool validateListenSocket();

    std::string getErrorMsg();

private:
    int _listenSocket = -1;

    std::unordered_set<int> _clients;

    std::string _errorMsg;
};
