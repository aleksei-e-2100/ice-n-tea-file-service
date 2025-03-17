#pragma once

#include <cstdint>
#include <string>

#include "address.h"

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
    bool open(const Address& serverAddress);

    void close();

    bool sendData(unsigned char* buf,
                  uint32_t len);

    bool receiveData(unsigned char* buf,
                     uint32_t maxLen,
                     uint32_t& len);

    bool isOpen();

    std::string getErrorMsg();

private:
    int _socket = -1;

    std::string _errorMsg;
};
