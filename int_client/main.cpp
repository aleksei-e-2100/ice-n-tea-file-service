#include <iostream>

#include "address.h"
#include "file.h"
#include "packet.h"
#include "tcpchannel.h"

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cout << "Command line arguments error" << std::endl;
        std::cout << "Usage: int_client [server ip] [server port] (65000-65535) [file path]"
                  << std::endl;

        return 1;
    }

    if (atoi(argv[2]) < 65000 || atoi(argv[2]) > 65535)
    {
        std::cout << "Server port error" << std::endl;

        return 1;
    }

    Address serverAddress;
    serverAddress.setFromString(argv[1], atoi(argv[2]));

    if (!serverAddress.valid())
    {
        std::cout << "Server ip error" << std::endl;

        return 0;
    }

    std::cout << "iCE && tEA File Client is running" << std::endl;
    std::cout << "Press Ctrl-C to break" << std::endl;

    TcpChannel tcpChannel;
    tcpChannel.open(serverAddress);

    if (!tcpChannel.isOpen())
    {
        std::cout << "Client socket error: "
                  << tcpChannel.getErrorMsg()
                  << std::endl;

        std::cout << "Client is stopped" << std::endl;

        return 0;
    }

    std::string msg;

    File file;

    if (!file.prepare(argv[3]))
    {
        tcpChannel.close();

        if (file.getMsg(msg))
            std::cout << msg << std::endl;

        return 0;
    }

    if (!tcpChannel.sendData(file.outPtr(), packetPutLen))
    {
        std::cout << "Client socket send error: "
                  << tcpChannel.getErrorMsg()
                  << std::endl;

        std::cout << "Client is stopped" << std::endl;
    }
    else
    {
        file.startTimer();
    }

    int sent = 0;

    while (true)
    {
        if (file.procPacket())
        {
            if (!tcpChannel.sendData(file.outPtr(), packetPutLen))
            {
                std::cout << "Client socket send error: "
                          << tcpChannel.getErrorMsg()
                          << std::endl;

                break;
            }
        }

        uint32_t ansLen;

        if (!tcpChannel.receiveData(file.inPtr(), packetAnsLen, ansLen))
        {
            std::cout << "Client socket receive error: "
                      << tcpChannel.getErrorMsg()
                      << std::endl;

            break;
        }

        if (ansLen > 0)
        {
            file.procAnswer(ansLen);

            file.startTimer();
        }

        auto isComplete = file.checkComplete();

        if (file.getMsg(msg))
            std::cout << msg << std::endl;

        if (isComplete)
            break;
    }

    tcpChannel.close();

    std::cout << "Client is stopped" << std::endl;

    return 0;
}
