/**
 * @file config_validator.h
 * @brief Configuration validation (after parsing)
 */

#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <map>

/**
 * @enum RawType
 * @brief Record type after parsing
 */
enum RawType
{
    RawUnknown,  ///< Unknown recording
    RawSection,  ///< Section record
    RawKeyValue, ///< Key-value record (key-value pair)
};

/**
 * @struct RawEntry
 * @brief The result of the first analysis of a single configuration line
 */
struct RawEntry
{
    size_t lineNum;      ///< Line number in the configuration file
    bool hasSection;     ///< Whether the line is within a section
    std::string section; ///< Name of the section to which the entry belongs
    std::string key;     ///< Parameter key (if present)
    std::string value;   ///< Parameter value (if present)
    RawType type;        ///< Type of the entry
};

/**
 * @struct ConfigEntry
 * @brief  Describes a single configuration section and all its parameters
 */
struct ConfigEntry
{
    std::string key = "";            ///< Private or public key for the section
    std::string dir = "";            ///< Directory for files
    std::string ip = "";             ///< IP-address
    uint16_t port = 0;               ///< Port
    bool isServer = false;           ///< Flag: section can act as a server
    bool isClient = false;           ///< Flag: section can act as a client
    bool isSpecified = false;        ///< Flag: section has been overridden
    bool isValid = true;             ///< Flag: section is correctly populated
    std::vector<std::string> errors; ///< Errors associated with the section
};

/**
 * @class ConfigValidator
 * @brief Checks the configuration and generates the final section structure
 */
class ConfigValidator
{
public:
    /**
     * @brief Constructor
     */
    ConfigValidator();

    /**
     * @brief Obtain the validated configuration
     * @return Configuration section map
     */
    const std::map<std::string, ConfigEntry>& getConfig() const;

    /**
     * @brief Get the name of the self section
     * @return Self section name
     */
    const std::string getSelfSectionName() const;

    /**
     * @brief Get the pseudo-section name for data outside of sections
     * @return Pseudo-section name
     */
    const std::string getNoSectionName() const;

    /**
     * @brief Validate the configuration
     * @param [in] rawData Raw data
     * @param [in] sanitize Remove invalid sections from the result
     * @return true - if the configuration is valid and complete
     */
    bool validate(const std::vector<RawEntry>& rawData, bool sanitize);

    /**
     * @brief Get the description of the last error
     * @return Error description string
     */
    const std::string& getLastErrStr() const;

private:
    /**
     * @brief Perform final actions for the configuration check
     */
    void finalize();

    /**
     * @brief Remove invalid sections
     */
    void removeInvalidEntries();

    /**
     * @brief Process a record that does not belong to the section
     * @param [in] rawEntry Record under review
     * @return true - if the record has been processed
     */
    bool procNoSectionData(const RawEntry& rawEntry);

    /**
     * @brief Process the section record
     * @param [in] rawEntry Record under review
     * @return true - if the record is a section and has been processed
     */
    bool procSection(const RawEntry& rawEntry);

    /**
     * @brief Process the private key record
     * @param [in] rawEntry Record under review
     * @return true - if the record is a private key and has been processed
     */
    bool procPrivKey(const RawEntry& rawEntry);

    /**
     * @brief Process the public key record
     * @param [in] rawEntry Record under review
     * @return true - if the record is a public key has been processed
     */
    bool procPubKey(const RawEntry& rawEntry);

    /**
     * @brief Process the record containing the file directory
     * @param [in] rawEntry Record under review
     * @return true - if the record contains file directory and has been processed
     */
    bool procDir(const RawEntry& rawEntry);

    /**
     * @brief Process IP record
     * @param [in] rawEntry Record under review
     * @return true - if the record is IP address and has been processed
     */
    bool procIp(const RawEntry& rawEntry);

    /**
     * @brief Process the port record
     * @param [in] rawEntry Record under review
     * @return true - if the record is port number and has been processed
     */
    bool procPort(const RawEntry& rawEntry);

    /**
     * @brief Process the record containing a junk string
     * @param [in] rawEntry Record under review
     * @return true - if the record has been processed
     */
    void procGarbage(const RawEntry& rawEntry);

    /**
     * @brief Check the validity of the section name
     * @param [in] str Section name
     * @return true - if the name is valid
     */
    bool checkSectionName(const std::string& sectionName) const;

private:
    std::map<std::string, ConfigEntry> config_;

    bool selfClient_;
    bool selfServer_;

    bool hasPeerClient_;
    bool hasPeerServer_;

    std::string errStr_;
};
