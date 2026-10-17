/**
 * @file cmd_line.h
 * @brief Processing command-line arguments
 */

#pragma once

#include <string>

/**
 * @enum WorkMode
 * @brief Application operating mode
 */
enum class WorkMode
{
    KeysGenerate,
    HealthCheck,
    Server,
    SendFile,
    ReceiveFile,
    Unknown
};

/**
 * @class CmdLine
 * @brief A set of methods for working with command-line arguments
 */
class CmdLine
{
public:
    /**
     * @brief Check command-line arguments and determine the operating mode
     * @param [in] argc The number of command-line arguments
     * @param [in] argv The array of command-line arguments
     * @param [out] workMode The determined operating mode of the application
     * @return true - if the arguments are valid
     */
    static bool checkArgs(int argc, const char* argv[], WorkMode& workMode);

    /**
     * @brief Retrieve the server name from the command-line arguments
     * @param [in] argc The number of command-line arguments
     * @param [in] argv The array of command-line arguments
     * @param [out] serverArg Server name.
     * @return true - if the argument was successfully retrieved
     */
    static bool getServerArg(int argc, const char* argv[], std::string& serverArg);

    /**
     * @brief Retrieves the filename (or path) from the command-line arguments
     * @param [in] argc The number of command-line arguments
     * @param [in] argv The array of command-line arguments
     * @param [out] fileArg Filename or path to the file
     * @return true - if the argument was successfully retrieved
     */
    static bool getFileArg(int argc, const char* argv[], std::string& fileArg);

    /**
     * @brief Show a hint about command-line parameters
     */
    static void showUsage();
};
