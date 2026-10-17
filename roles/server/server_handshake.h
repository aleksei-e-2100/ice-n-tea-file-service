/**
 * @file server_handshake.h
 * @brief Server-side of the handshake procedure
 */

#pragma once

#include <string>

#include "session_ctx.h"

/**
 * @class ServerHandshake
 * @brief Implements the server-side of the handshake procedure and
 *        the computing of the shared secret
 */
class ServerHandshake
{
public:
    /**
     * @brief Constructor
     * @param [in] ctx Session context
     */
    ServerHandshake(SessionCtx& ctx);

    /**
     * @brief Destructor
     */
    ~ServerHandshake() = default;

    // Assignment and transfer are prohibited
    ServerHandshake(const ServerHandshake&) = delete;
    ServerHandshake& operator=(const ServerHandshake&) = delete;
    ServerHandshake(ServerHandshake&&) = delete;
    ServerHandshake& operator=(ServerHandshake&&) = delete;

    /**
     * @brief Save the server's static key pair
     * @return true - on success
     */
    bool setStaticKeys();

    /**
     * @brief Save the server's ephemeral key pair
     * @return true - if generation and saving are successful
     */
    bool setEphemKeys();

    /**
     * @brief Process the handshake request
     * @return true - if the request is successfully verified
     */
    bool procHandshakeReq();

    /**
     * @brief Prepare a handshake reply
     * @return true - if the reply was prepared successfully
     */
    bool prepareHandshakeRep();

    /**
     * @brief Compute the shared session secret
     * @return true - if the secret was successfully computed
     */
    bool computeSecret();

private:
    SessionCtx& ctx_;

    std::string serverStaticPrivKey_;
    std::string serverStaticPubKey_;

    std::string serverEphemPrivKey_;
    std::string serverEphemPubKey_;

    std::string clientStaticPubKey_;
    std::string clientEphemPubKey_;
};
