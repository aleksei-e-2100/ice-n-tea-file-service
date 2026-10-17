/**
 * @file main.cpp
 * @brief Application entry point and general logic
*/

#include <iostream>
#include <atomic>
#include <csignal>

#include "cmd_line.h"
#include "config.h"
#include "crypto.h"
#include "server_manager.h"
#include "client_manager.h"

std::atomic<bool> isRunning(true);

void signalHandler(const int signum)
{
    isRunning.store(false);
}

int main(const int argc, const char* argv[])
{
    signal(SIGINT, signalHandler);  // Ctrl-C
    signal(SIGTERM, signalHandler); // systemctl stop

    WorkMode workMode = WorkMode::Unknown;

    std::cout << "iCE && tEA File Service\n" << std::endl;

    if (!CmdLine::checkArgs(argc, argv, workMode))
    {
        CmdLine::showUsage();
    }

    switch (workMode)
    {
    case WorkMode::KeysGenerate:
    {
        std::string privKey;
        std::string pubKey;

        if (!Crypto::keygen(KeyType::ED25519, privKey, pubKey))
        {
            std::cout << "Error: can't generate keys" << std::endl;
            break;
        }

        std::cout << "Generating ED25519 keys pair (Base64 of 32 bits):\n" << std::endl;
        std::cout << "  Private key: " << privKey << std::endl;
        std::cout << "  Public key:  " << pubKey << std::endl;

        break;
    }

    case WorkMode::HealthCheck:
    {
        if (!Config::checkHealth())
            std::cout << Config::getLastErrStr() << std::endl;

        break;
    }

    case WorkMode::Server:
    {
        std::cout << "Running as server (Ctrl-C to break)...\n" << std::endl;

        if (!Config::prepare())
        {
            std::cout << Config::getLastErrStr() << std::endl;
            break;
        }

        ServerManager manager;

        if (!manager.open())
            break;

        isRunning.store(true);

        while (isRunning.load())
        {
            ServerManagerResult res = manager.runOnce();

            if (res != ServerManagerResult::Continue)
                break;
        }

        // Closing the manager will trigger the closing of channels, file transfers,
        // the epoll descriptor, and sockets.
        manager.close();

        break;
    }

    case WorkMode::SendFile:
    case WorkMode::ReceiveFile:
    {
        std::cout << "Running as client (Ctrl-C to break)...\n" << std::endl;

        if (!Config::prepare())
        {
            std::cout << Config::getLastErrStr() << std::endl;
            break;
        }

        std::string serverName = "";
        CmdLine::getServerArg(argc, argv, serverName);

        std::string fileArg = "";
        CmdLine::getFileArg(argc, argv, fileArg);

        ClientManager manager;

        if (!manager.open(workMode, fileArg, serverName))
            break;

        isRunning.store(true);

        while (isRunning.load())
        {
            ClientManagerResult res = manager.runOnce();

            if (res != ClientManagerResult::Continue)
                break;
        }

        // Closing the manager will trigger a chain reaction: closing the channel,
        // the receive (or send) operation, the epoll descriptor, and the socket.
        // If the channel is in the Finished state, closing the socket will
        // result in the graceful sending of a FIN.
        manager.close();

        break;
    }

    default:
    {
        break;
    }
    }

    std::cout << std::endl;

    return 0;
}
