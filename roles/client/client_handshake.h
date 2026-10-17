/**
 * @file client_handshake.h
 * @brief Client-side of the handshake procedure
 */

#pragma once

#include <string>

#include "session_ctx.h"

/**
 * @class ClientHandshake
 * @brief Implements the client-side of the handshake procedure and
 *        the computing of the shared secret
 */
class ClientHandshake
{
public:
    /**
     * @brief Constructor
     * @param [in] ctx Session context
     */
    ClientHandshake(SessionCtx& ctx);

    /**
     * @brief Destructor
     */
    ~ClientHandshake() = default;

    // Assignment and transfer are prohibited
    ClientHandshake(const ClientHandshake&) = delete;
    ClientHandshake& operator=(const ClientHandshake&) = delete;
    ClientHandshake(ClientHandshake&&) = delete;
    ClientHandshake& operator=(ClientHandshake&&) = delete;

    /**
     * @brief Save the client's static key pair and the server's static public key
     * @param [in] serverStaticPubKey Server's static public key
     * @return true - on success
     */
    bool setStaticKeys(const std::string& serverStaticPubKey);

    /**
     * @brief Save the client's ephemeral key pair
     * @return true - if generation and saving are successful
     */
    bool setEphemKeys();

    /**
     * @brief Prepare a handshake request
     * @return true - if the request was prepared successfully
     */
    bool prepareHandshakeReq();

    /**
     * @brief Process the handshake reply
     * @return true - if the reply is successfully verified
     */
    bool procHandshakeRep();

    /**
     * @brief Compute the shared session secret
     * @return true - if the secret was successfully computed
     */
    bool computeSecret();

private:
    SessionCtx& ctx_;

    std::string clientStaticPrivKey_;
    std::string clientStaticPubKey_;

    std::string clientEphemPrivKey_;
    std::string clientEphemPubKey_;

    std::string serverStaticPubKey_;
    std::string serverEphemPubKey_;
};
