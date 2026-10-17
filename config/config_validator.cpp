/**
 * @file config_validator.cpp
 * @brief Configuration validation (after parsing)
 */

#include "config_validator.h"

#include <string_view>
#include <filesystem>
#include <cassert>

#include "endpoint.h"

namespace
{
    constexpr std::string_view selfSection = "~";
    constexpr std::string_view noSection = "!";

    constexpr std::string_view ipKey = "ip";
    constexpr std::string_view portKey = "port";
    constexpr std::string_view dirKey = "dir";
    constexpr std::string_view pubKey = "public_key";
    constexpr std::string_view privKey = "private_key";

    constexpr std::string_view wrongStr = "???";
    constexpr uint16_t wrongPort = 1;
}

ConfigValidator::ConfigValidator()
{
    selfClient_ = false;
    selfServer_ = false;
    hasPeerClient_ = false;
    hasPeerServer_ = false;
}

const std::map<std::string, ConfigEntry>& ConfigValidator::getConfig() const
{
    return config_;
}

const std::string ConfigValidator::getSelfSectionName() const
{
    return std::string(selfSection);
}

const std::string ConfigValidator::getNoSectionName() const
{
    return std::string(noSection);
}

bool ConfigValidator::validate(const std::vector<RawEntry>& rawData, bool sanitize)
{
    config_.clear();
    errStr_ = "";

    selfClient_ = false;
    selfServer_ = false;
    hasPeerClient_ = false;
    hasPeerServer_ = false;

    // Add an empty section, as there may be junk data outside of any section
    // (from this point on, any access to config_[...] adds a key to the map)
    config_[""].isValid = false;

    for (auto& rawEntry : rawData)
    {
        if (!procNoSectionData(rawEntry) &&
            !procSection(rawEntry) &&
            !procPrivKey(rawEntry) &&
            !procPubKey(rawEntry) &&
            !procDir(rawEntry) &&
            !procIp(rawEntry) &&
            !procPort(rawEntry))
        {
            procGarbage(rawEntry);
        }
    }

    finalize();

    if (sanitize)
        removeInvalidEntries();


    if ((selfClient_ && hasPeerServer_) ||
        (selfServer_ && hasPeerClient_))
    {
        return true;
    }

    errStr_ = "[Config] Configuration is not complete";

    return false;
}

const std::string& ConfigValidator::getLastErrStr() const
{
    return errStr_;
}

void ConfigValidator::finalize()
{
    for (auto it = config_.begin(); it != config_.end(); ++it)
    {
        // Skip data that lacks a section and may appear at the beginning
        if (it->first == noSection)
            continue;

        // Check the minimum configuration required to function at least as a client
        if (it->second.key.empty())
        {
            std::string err = "section \"" + it->first + "\" has no key";
            it->second.isValid = false;
        }

        if (it->first == noSection && it->second.dir.empty())
        {
            std::string err = "section \"" + it->first + "\" has no dir";
            it->second.isValid = false;
        }

        if (!it->second.isValid)
        {
            std::string err = "section \"" + it->first + "\" is not valid";
            it->second.errors.push_back(std::move(err));
            continue;
        }

        // A valid configuration (containing a key and a directory) effectively constitutes a client
        it->second.isClient = true;

        if (it->first == selfSection)
            selfClient_ = true;
        else
            hasPeerClient_ = true;

        // Check configuration for acting as a server
        if (!it->second.ip.empty() && it->second.port != 0)
        {
            if (it->first == selfSection && !it->second.dir.empty())
            {
                it->second.isServer = true;
                selfServer_ = true;
            }
            else if (it->first != selfSection)
            {
                it->second.isServer = true;
                hasPeerServer_ = true;
            }
        }
    }
}

void ConfigValidator::removeInvalidEntries()
{
    if (config_.contains(std::string(noSection)))
        config_.erase(std::string(noSection));

    for (auto it = config_.begin(); it != config_.end(); )
    {
        if (!it->second.isValid)
            it = config_.erase(it);
        else
            ++it;
    }
}

bool ConfigValidator::procNoSectionData(const RawEntry &rawEntry)
{
    if (rawEntry.hasSection)
        return false;

    std::string err = "line " + std::to_string(rawEntry.lineNum) + ": no section data";
    config_[std::string(noSection)].errors.push_back(std::move(err));
    config_[std::string(noSection)].isValid = false;

    return true;
}

bool ConfigValidator::procSection(const RawEntry &rawEntry)
{
    if (rawEntry.type != RawType::RawSection)
        return false;

    // First definition (if there is no entry in the map, such a call will create one)
    if (!config_[rawEntry.section].isSpecified)
    {
        config_[rawEntry.section].isSpecified = true;
    }
    // Redefinition
    else
    {
        std::string err = "line " + std::to_string(rawEntry.lineNum) + ": re-specifying section";
        config_[rawEntry.section].errors.push_back(std::move(err));
        config_[rawEntry.section].isValid = false;
    }

    if (rawEntry.section != selfSection && !checkSectionName(rawEntry.section))
    {
        std::string err = "line " + std::to_string(rawEntry.lineNum) + ": wrong section name";
        config_[rawEntry.section].errors.push_back(std::move(err));
        config_[rawEntry.section].isValid = false;
    }

    return true;
}

bool ConfigValidator::procPrivKey(const RawEntry& rawEntry)
{
    assert(rawEntry.hasSection);

    if (rawEntry.type != RawType::RawKeyValue || rawEntry.key != privKey)
        return false;

    if (rawEntry.section != selfSection)
        return false;

    bool ok = true;

    if (!config_[rawEntry.section].key.empty())
    {
        std::string err = "line " + std::to_string(rawEntry.lineNum) + ": re-specifying private key";
        config_[rawEntry.section].errors.push_back(std::move(err));

        ok = false;
    }

    if (rawEntry.value.length() != 44)
    {
        std::string err = "line " + std::to_string(rawEntry.lineNum) + ": wrong private key";
        config_[rawEntry.section].errors.push_back(std::move(err));

        ok = false;
    }

    if (ok)
    {
        config_[rawEntry.section].key = rawEntry.value;
    }
    else
    {
        config_[rawEntry.section].key = wrongStr;
        config_[rawEntry.section].isValid = false;
    }

    return true;
}

bool ConfigValidator::procPubKey(const RawEntry& rawEntry)
{
    assert(rawEntry.hasSection);

    if (rawEntry.type != RawType::RawKeyValue || rawEntry.key != pubKey)
        return false;

    if (rawEntry.section == selfSection)
        return false;

    bool ok = true;

    if (!config_[rawEntry.section].key.empty())
    {
        std::string err = "line " + std::to_string(rawEntry.lineNum) + ": re-specifying public key";
        config_[rawEntry.section].errors.push_back(std::move(err));

        ok = false;
    }

    if (rawEntry.value.length() != 44)
    {
        std::string err = "line " + std::to_string(rawEntry.lineNum) + ": wrong public key";
        config_[rawEntry.section].errors.push_back(std::move(err));

        ok = false;
    }

    if (ok)
    {
        config_[rawEntry.section].key = rawEntry.value;
    }
    else
    {
        config_[rawEntry.section].key = wrongStr;
        config_[rawEntry.section].isValid = false;
    }

    return true;
}

bool ConfigValidator::procDir(const RawEntry& rawEntry)
{
    assert(rawEntry.hasSection);

    if (rawEntry.type != RawType::RawKeyValue || rawEntry.key != dirKey)
        return false;

    if (rawEntry.section != selfSection)
        return false;

    bool ok = true;

    if (!config_[rawEntry.section].dir.empty())
    {
        std::string err = "line " + std::to_string(rawEntry.lineNum) + ": re-specifying directory";
        config_[rawEntry.section].errors.push_back(std::move(err));

        ok = false;
    }

    // Check, among other things, for the existence of a directory
    std::filesystem::path dir(rawEntry.value);

    if (!std::filesystem::is_directory(dir) || !dir.is_absolute())
    {
        std::string err = "line " + std::to_string(rawEntry.lineNum) + ": wrong directory";
        config_[rawEntry.section].errors.push_back(std::move(err));

        ok = false;
    }

    if (ok)
    {
        config_[rawEntry.section].dir = rawEntry.value;

        if (config_[rawEntry.section].dir.back() != '/')
            config_[rawEntry.section].dir.push_back('/');
    }
    else
    {
        config_[rawEntry.section].dir = wrongStr;
        config_[rawEntry.section].isValid = false;
    }

    return true;
}

bool ConfigValidator::procIp(const RawEntry& rawEntry)
{
    assert(rawEntry.hasSection);

    if (rawEntry.type != RawType::RawKeyValue || rawEntry.key != ipKey)
        return false;

    bool ok = true;

    if (!config_[rawEntry.section].ip.empty())
    {
        std::string err = "line " + std::to_string(rawEntry.lineNum) + ": re-specifying ip";
        config_[rawEntry.section].errors.push_back(std::move(err));

        ok = false;
    }

    if (!Endpoint::checkIpFromStr(rawEntry.value.c_str()))
    {
        std::string err = "line " + std::to_string(rawEntry.lineNum) + ": wrong ip";
        config_[rawEntry.section].errors.push_back(std::move(err));

        ok = false;
    }

    if (ok)
    {
        config_[rawEntry.section].ip = rawEntry.value;
    }
    else
    {
        config_[rawEntry.section].ip = wrongStr;
        config_[rawEntry.section].isValid = false;
    }

    return true;
}

bool ConfigValidator::procPort(const RawEntry& rawEntry)
{
    assert(rawEntry.hasSection);

    if (rawEntry.type != RawType::RawKeyValue || rawEntry.key != portKey)
        return false;

    bool ok = true;

    if (config_[rawEntry.section].port != 0)
    {
        std::string err = "line " + std::to_string(rawEntry.lineNum) + ": re-specifying port";
        config_[rawEntry.section].errors.push_back(std::move(err));

        ok = false;
    }

    size_t pos = 0;
    uint64_t portU64 = 0;

    if (!rawEntry.value.empty() &&
        rawEntry.value.length() <= 5 &&
        std::isdigit(static_cast<unsigned char>(rawEntry.value[0])))
    {
        portU64 = std::stoul(rawEntry.value, &pos);
    }

    if (portU64 == 0 ||
        pos != rawEntry.value.length() ||
        !Endpoint::checkPortFromU64(portU64))
    {
        std::string err = "line " + std::to_string(rawEntry.lineNum) + ": wrong port";
        config_[rawEntry.section].errors.push_back(std::move(err));

        ok = false;
    }

    if (ok)
    {
        config_[rawEntry.section].port = static_cast<uint16_t>(portU64);
    }
    else
    {
        config_[rawEntry.section].port = wrongPort;
        config_[rawEntry.section].isValid = false;
    }

    return true;
}

void ConfigValidator::procGarbage(const RawEntry& rawEntry)
{
    assert(rawEntry.hasSection);

    std::string err = "line " + std::to_string(rawEntry.lineNum) + ": garbage";
    config_[rawEntry.section].errors.push_back(std::move(err));
    config_[rawEntry.section].isValid = false;
}

bool ConfigValidator::checkSectionName(const std::string& sectionName) const
{
    if (sectionName.empty())
        return false;

    for (auto& ch : sectionName)
    {
        if (!(ch >= '0' && ch <= '9') &&
            !(ch >= 'A' && ch <= 'Z') &&
            !(ch >= 'a' && ch <= 'z') &&
            ch != '_' &&
            ch != '-')
        {
            return false;
        }
    }

    return true;
}
