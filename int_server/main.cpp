#include <atomic>
#include <csignal>
#include <iostream>

#include "manager.h"

std::atomic<bool> isRunning(true);

void signalHandler(int signum)
{
    isRunning.store(false);
}

int main(int argc, char* argv[])
{
    if (argc != 2 || atoi(argv[1]) < 65000 || atoi(argv[1]) > 65535)
    {
        std::cout << "Command line arguments error" << std::endl;
        std::cout << "Usage: int_server [server port] (65000-65535)"
                  << std::endl;

        return 1;
    }

    signal(SIGINT, signalHandler);  // Ctrl-C
    signal(SIGTERM, signalHandler); // systemctl stop

    Manager manager(atoi(argv[1]));

    std::cout << "iCE && tEA File Server is running" << std::endl;
    // std::cout << "Press Ctrl-C to break" << std::endl;

    while (isRunning.load())
    {
        manager.runOnce();
    }

    std::cout << "Server is stopped" << std::endl;

    return 0;
}
