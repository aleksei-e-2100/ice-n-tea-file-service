/**
 * @file config_data.h
 * @brief The single access point for application configuration.
 *        Manages the loading, parsing, validation, and provision of configuration.
 */

#pragma once

#include <string>
#include <map>

#include "config_parser.h"
#include "config_validator.h"

/**
 * @class Config
 * @brief Loads, parses, validates, and provides the configuration.
 */
class Config
{
public:
    /**
     * @brief Obtain the validated configuration
     * @return Configuration section map
     */
    static const std::map<std::string, ConfigEntry>& data();

    /**
     * @brief Get the name of the self section
     * @return Self section name
     */
    static const std::string selfName();

    /**
     * @brief Load, parse and validate the configuration
     * @return true - if the configuration is valid and ready for use
     */
    static bool prepare();

    /**
    * @brief Perform a check of the configuration's correctness
    * @return true - if the check was performed
    */
    static bool checkHealth();

    /**
     * @brief Get the description of the last error
     * @return Error description string
     */
    static const std::string& getLastErrStr();

private:
    /**
     * @brief Constructor
     */
    Config();

    /**
     * @brief Destructor
     */
    ~Config() = default;

    // Assignment and transfer are prohibited
    Config(const Config& other) = delete;
    Config& operator=(const Config& other) = delete;
    Config(Config&& other) = delete;
    Config& operator=(Config&& other) = delete;

    /**
     * @brief Get the singleton instance (Meyers' singleton)
     * @return Reference to the Config instance
     */
    static Config& instance();

private:
    ConfigParser parser_;
    ConfigValidator validator_;

    std::string errStr_;

    bool isInited_;
};
