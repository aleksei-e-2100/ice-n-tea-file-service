/**
* @file packet.h
* @brief Protocol structures and constants
*/

#pragma once

#include <cstddef>
#include <cstdint>

#include "crypto.h"

/**
 * @brief Maximum length of the transmitted file block
 */
inline constexpr size_t maxBlockLen = 64 * 1024;

/**
 * @brief Maximum number (window) of sent blocks before receipt confirmation
 */
inline constexpr size_t packetsWindow = 62;

/**
 * @brief Timeout waiting for a response from the peer
 */
inline constexpr int ansTimeout = 3000;

/**
 * @enum Decision
 * @brief Decision on readiness to accept (transfer) a file
 */
enum class Decision : uint32_t
{
    Accept,
    DenyNoFile,
    DenyNoSpace,
    Error
};

/**
 * @enum TaskType
 * @brief Server task type
 */
enum class TaskType : uint32_t
{
    PutFile,
    GetFile
};


#pragma pack(push, 1)

/**
 * @struct HandshakeReq
 * @brief Packet: handshake request
 */
struct HandshakeReq
{
    char clientStaticPubKey[Crypto::keyB64Len]; ///< Static client public key (Base64, 44 bytes)
    char clientEphemPubKey[Crypto::keyB64Len];  ///< Ephemeral client public key (Base64, 44 bytes)
    char signature[Crypto::signatureB64Len];    ///< Signature (Base64, 88 bytes)
};

/**
 * @brief Message length in the handshake request
 */
inline constexpr size_t handshakeReqMsgLen = sizeof(HandshakeReq) - sizeof(HandshakeReq::signature);

/**
 * @struct HandshakeRep
 * @brief  Packet: handshake reply
 */
struct HandshakeRep
{
    char serverEphemPubKey[Crypto::keyB64Len]; ///< Ephemeral server public key (Base64, 44 bytes)
    char signature[Crypto::signatureB64Len];   ///< Signature (Base64, 88 bytes)
};

/**
 * @brief Message length in the handshake reply
 */
inline constexpr size_t handshakeRepMsgLen = sizeof(HandshakeRep) - sizeof(HandshakeRep::signature);

/**
 * @struct TaskReq
 * @brief Packet: request for a file put (or get) task
 */
struct TaskReq
{
    TaskType taskType;        ///< Task type (4 bytes)
    char fName[256];          ///< File name (256 bytes), up to the first '\0'; the remainder is zeroed out (!)
    uint64_t fSize;           ///< File size (8 bytes, size_t for 64 bit)
    char tag[Crypto::tagLen]; ///< Authentication tag (16 bytes)
};

/**
 * @brief Message length in a file put (or get) task request
 */
inline constexpr size_t taskReqMsgLen = sizeof(TaskReq) - sizeof(TaskReq::tag);

/**
 * @struct TaskRep
 * @brief Packet: reply to a request regarding a file put (or get) task.
 */
struct TaskRep
{
    Decision decision;        ///< Request result, server reply (4 bytes)
    uint64_t fSize;           ///< File size (8 bytes, size_t for 64-bit), only when get from the server; otherwise 0
    char tag[Crypto::tagLen]; ///< Authentication tag (16 bytes)
};

/**
 * @brief Message length in the reply to a file put (or get) request
 */
inline constexpr size_t taskRepMsgLen = sizeof(TaskRep) - sizeof(TaskRep::tag);

/**
 * @struct Confirm
 * @brief Package: confirmation of file receipt (or readiness to receive)
 */
struct Confirm
{
    Decision decision;        ///< Confirmation code for the acceptance of the next block (4 bytes)
    char tag[Crypto::tagLen]; ///< Authentication tag (16 bytes)
};

/**
 * @brief Length of the message acknowledging file receipt (or readiness to receive the file)
 */
inline constexpr size_t confirmMsgLen = sizeof(Confirm) - sizeof(Confirm::tag);

/**
 * @struct BlockSuffix
 * @brief File block suffix (authentication tag)
 */
struct BlockSuffix
{
    char tag[Crypto::tagLen]; ///< Authentication tag (16 bytes); authenticates the payload
};

#pragma pack(pop)
