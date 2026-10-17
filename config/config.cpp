/**
 * @file config_data.cpp
 * @brief The single access point for application configuration.
 *        Manages the loading, parsing, validation, and provision of configuration.
 */

#include "config.h"

#include <iostream>
#include <vector>
#include <set>

const std::map<std::string, ConfigEntry>& Config::data()
{
    return instance().validator_.getConfig();
}

const std::string Config::selfName()
{
    auto& validator = Config::instance().validator_;

    return validator.getSelfSectionName();
}

bool Config::prepare()
{
    auto& parser = Config::instance().parser_;
    auto& validator = Config::instance().validator_;
    auto& errStr = Config::instance().errStr_;

    errStr = "";

    if (!parser.parse())
    {
        errStr = parser.getLastErrStr();
        return false;
    }

    if (!validator.validate(parser.getRawData(), true))
    {
        errStr = validator.getLastErrStr();
        return false;
    }

    Config::instance().isInited_ = true;

    return true;
}

bool Config::checkHealth()
{
    auto& parser = Config::instance().parser_;
    auto& validator = Config::instance().validator_;
    auto& errStr = Config::instance().errStr_;

    errStr = "";

    std::cout << "Checking configuration health:" << std::endl;

    if (!parser.parse())
    {
        errStr = parser.getLastErrStr();
        return false;
    }

    bool res = validator.validate(parser.getRawData(), false);

    auto& rawData = parser.getRawData();
    auto& config = validator.getConfig();

    bool selfClient = false;
    bool selfServer = false;

    std::vector<std::string> clients;
    std::vector<std::string> servers;

    std::set<std::string> seenSections;

    // Output configuration errors in chronological order
    // (while preparing data for the final result)
    if (config.contains(validator.getNoSectionName()))
    {
        auto& entry = config.at(validator.getNoSectionName());

        std::cout << std::endl;

        for (auto& err : entry.errors)
        {
            std::cout << "  " << err << std::endl;
        }
    }

    for (auto i = 0; i < rawData.size(); ++i)
    {
        if (rawData[i].type == RawType::RawSection || i == rawData.size() - 1)
        {
            if (seenSections.contains(rawData[i].section))
                continue;

            auto& section = rawData[i].section;

            seenSections.insert(section);

            auto& entry = config.at(section);

            if (entry.isValid && section == validator.getSelfSectionName())
            {
                selfClient = true;
                selfServer = entry.isServer;
            }
            else if (entry.isValid)
            {
                clients.push_back(section);

                if (entry.isServer)
                    servers.push_back(section);
            }

            if (!entry.errors.empty())
                std::cout << std::endl;

            for (auto& err : entry.errors)
            {
                std::cout << "  " << err << std::endl;
            }
        }
    }

    // Output the final result
    if (res)
    {
        std::cout << "\n  Result: configuration is ready:" << std::endl;

        if (selfServer && !servers.empty())
            std::cout << "    self mode: server and client" << std::endl;
        else if (selfServer && servers.empty())
            std::cout << "    self mode: server only" << std::endl;
        else
            std::cout << "    self mode: client only" << std::endl;


        if (selfServer)
        {
            std::cout << "    available clients: ";

            for (auto i = 0; i < clients.size(); ++i)
            {
                std::cout << clients[i];

                if (i < clients.size() - 1)
                    std::cout << ", ";
            }

            std::cout << std::endl;
        }

        if (!servers.empty())
        {
            std::cout << "    available servers: ";

            for (auto i = 0; i < servers.size(); ++i)
            {
                std::cout << servers[i];

                if (i < servers.size() - 1)
                    std::cout << ", ";
            }

            std::cout << std::endl;
        }

        return true;
    }

    std::cout << "\n  Result: configuration error:" << std::endl;

    if (!selfClient)
        std::cout << "    section \"~\" is not configured" << std::endl;

    if (clients.empty() || (selfClient && !selfServer && servers.empty()))
        std::cout << "    peers are not configured" << std::endl;

    return true;
}

const std::string& Config::getLastErrStr()
{
    return Config::instance().errStr_;
}

Config::Config()
{
    isInited_ = false;
}

Config& Config::instance()
{
    static Config instance;
    return instance;
}
