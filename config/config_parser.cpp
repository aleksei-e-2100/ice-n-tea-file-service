/**
 * @file config_parser.cpp
 * @brief Configuration file parser
 */

#include "config_parser.h"

#include "config_validator.h"

namespace
{
    constexpr std::string_view configFname = "filesvc.conf";

    constexpr char delimiter = ':';
    constexpr char sectionBegin = '[';
    constexpr char sectionEnd = ']';

    constexpr std::string_view whitespace = " \t\n\r\f\v";
}

const std::vector<RawEntry>& ConfigParser::getRawData() const
{
    return rawData_;
}

bool ConfigParser::parse()
{
    rawData_.clear();
    errStr_ = "";

    file_.open(std::string(configFname));

    if (!file_.is_open())
    {
        errStr_ = "[Config] Failed to open configuration file";
        return false;
    }

    hasSection_ = false;
    currSection_ = "";
    line_ = "";
    lineNum_ = 1;

    for (; ; ++lineNum_)
    {
        if (!std::getline(file_, line_))
            break;

        trimString(line_);

        if (line_.empty())
            continue;

        if (!checkSection() && !checkKey())
        {
            rawData_.emplace_back(RawEntry(
                lineNum_,
                hasSection_,
                currSection_,
                "",
                "",
                RawType::RawUnknown));
        }
    }

    file_.close();

    return true;
}

const std::string& ConfigParser::getLastErrStr() const
{
    return errStr_;
}

bool ConfigParser::checkSection()
{
    if (line_.front() != sectionBegin || line_.back() != sectionEnd)
        return false;

    // If a section is found at least once, then everything else will be within that section
    hasSection_ = true;

    currSection_.assign(line_.begin() + 1, line_.end() - 1);

    rawData_.emplace_back(RawEntry(
        lineNum_,
        hasSection_,
        currSection_,
        "",
        "",
        RawType::RawSection));

    return true;
}

bool ConfigParser::checkKey()
{
    size_t pos = line_.find(delimiter);

    if (pos == std::string::npos)
        return false;

    std::string key = line_.substr(0, pos);

    std::string value = line_.substr(pos + 1);
    trimString(value);

    rawData_.emplace_back(RawEntry(
        lineNum_,
        hasSection_,
        currSection_,
        key,
        value,
        RawType::RawKeyValue));

    return true;
}

void ConfigParser::trimString(std::string& str)
{
    size_t end = str.find_last_not_of(whitespace);

    if (end != std::string::npos)
    {
        str.erase(end + 1);
    }
    else
    {
        str.clear();
        return;
    }

    size_t start = str.find_first_not_of(whitespace);

    if (start != std::string::npos)
    {
        str.erase(0, start);
    }
}
