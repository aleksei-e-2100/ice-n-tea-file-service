/**
 * @file config_parser.h
 * @brief Configuration file parser
 */

#pragma once

#include <string>
#include <fstream>
#include <vector>

struct RawEntry;

/**
 * @class ConfigParser
 * @brief Configuration file parser
 */
class ConfigParser
{
public:
    /**
     * @brief Retrieve raw data
     * @return Vector of RawEntry records
     */
    const std::vector<RawEntry>& getRawData() const;

    /**
     * @brief Parse the configuration file
     * @return true - on success
     */
    bool parse();

    /**
     * @brief Get the description of the last error
     * @return Error description string
     */
    const std::string& getLastErrStr() const;

private:
    /**
    * @brief Check whether the current line is a section
    * @return true - if the string contains a section.
    */
    bool checkSection();

    /**
     * @brief Check whether the string is a key or a key-value pair
     * @return true - if the string contains a key or a key-value pair.
     */
    bool checkKey();

    /**
     * @brief Remove whitespace from the ends of the string
     * @param [in, out] str String being processed
     */
    void trimString(std::string& str);

private:
    std::vector<RawEntry> rawData_;

    std::ifstream file_;

    bool hasSection_;
    std::string currSection_;
    std::string line_;
    size_t lineNum_;

    std::string errStr_;
};
