/**
 * @file cmd_line.cpp
 * @brief Processing command-line arguments
 */

#include "cmd_line.h"

#include <iostream>

bool CmdLine::checkArgs(int argc, const char* argv[], WorkMode &workMode)
{
    if (argc < 2)
        return false;

    std::string opt = std::string(argv[1]);

    if (opt == "--keygen")
    {
        if (argc != 2)
            return false;

        workMode = WorkMode::KeysGenerate;

        return true;
    }

    if (opt == "--health")
    {
        if (argc != 2)
            return false;

        workMode = WorkMode::HealthCheck;

        return true;
    }

    if (opt == "--listen")
    {
        if (argc != 2)
            return false;

        workMode = WorkMode::Server;

        return true;
    }

    if (opt == "--send")
    {
        if (argc != 4)
            return false;

        workMode = WorkMode::SendFile;

        return true;
    }

    if (opt == "--recv")
    {
        if (argc != 4)
            return false;

        workMode = WorkMode::ReceiveFile;

        return true;
    }

    workMode = WorkMode::Unknown;

    return false;
}

bool CmdLine::getServerArg(int argc, const char* argv[], std::string& serverArg)
{
    serverArg = std::string(argv[2]);

    return true;
}

bool CmdLine::getFileArg(int argc, const char* argv[], std::string& fileArg)
{
    fileArg = std::string(argv[3]);

    return true;
}

void CmdLine::showUsage()
{
    std::cout <<
    "Usage: filesvc <command> [arguments]\n"
    "\nCommands:\n"
    "  --keygen                            Generate security keys\n"
    "  --health                            Check configuration file health\n"
    "  --listen                            Start filesvc as server to listen for connections\n"
    "  --send <server_name> <file_path>    Send file to a remote server\n"
    "                                      (requires absolute or relative file path on client machine)\n"
    "  --recv <server_name> <file_name>    Receive file from a remote server\n"
    "                                      (requires file name inside server working directory)"
    << std::endl;
}
