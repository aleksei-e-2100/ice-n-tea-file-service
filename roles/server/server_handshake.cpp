/**
 * @file server_handshake.cpp
 * @brief Server-side of the handshake procedure
 */

#include "server_handshake.h"

#include <cstring>
#include <cassert>

#include "packet.h"
#include "crypto.h"
#include "config.h"

ServerHandshake::ServerHandshake(SessionCtx& ctx)
    : ctx_(ctx)
{
}

bool ServerHandshake::setStaticKeys()
{
    serverStaticPrivKey_ = Config::data().at(Config::selfName()).key;

    if (!Crypto::getPubKeyED25519(serverStaticPrivKey_, serverStaticPubKey_))
    {
        ctx_.errStr = "[Handshake] Failed to set ED25519 key pair";
        return false;
    }

    clientStaticPubKey_ = "";

    return true;
}

bool ServerHandshake::setEphemKeys()
{
    if (!Crypto::keygen(KeyType::X25519, serverEphemPrivKey_, serverEphemPubKey_))
    {
        ctx_.errStr = "[Handshake] Failed to set X25519 ephemeral key pair";
        return false;
    }

    return true;
}

bool ServerHandshake::procHandshakeReq()
{
    assert(ctx_.inBuf.size() == sizeof(HandshakeReq));

    HandshakeReq* request = reinterpret_cast<HandshakeReq*>(ctx_.inBuf.data());

    clientStaticPubKey_.assign(request->clientStaticPubKey, sizeof(HandshakeReq::clientStaticPubKey));

    // Search for a peer in the settings
    bool isFound = false;

    for (auto& entry : Config::data())
    {
        if (entry.second.key == clientStaticPubKey_ &&
            entry.first != Config::selfName())
        {
            ctx_.peerName = entry.first;
            isFound = true;
            break;
        }
    }

    if (!isFound)
    {
        ctx_.errStr = "[Handshake] Client not found in configuration";
        return false;
    }

    // Continuation, if the peer is found
    clientEphemPubKey_.assign(request->clientEphemPubKey, sizeof(HandshakeReq::clientEphemPubKey));
    std::string signature(request->signature, sizeof(HandshakeReq::signature));

    if (!Crypto::verifyED25519(clientStaticPubKey_, ctx_.inBuf, handshakeReqMsgLen, signature))
    {
        ctx_.errStr = "[Handshake] Request signature verification failed";
        return false;
    }

    return true;
}

bool ServerHandshake::prepareHandshakeRep()
{
    assert(serverEphemPubKey_.length() == sizeof(HandshakeRep::serverEphemPubKey));

    Frame frame(sizeof(HandshakeRep));

    HandshakeRep* reply = reinterpret_cast<HandshakeRep*>(frame.buf.data());

    std::memcpy(reply->serverEphemPubKey, serverEphemPubKey_.data(), serverEphemPubKey_.length());

    std::string signature;

    if (!Crypto::signED25519(serverStaticPrivKey_, frame.buf, handshakeRepMsgLen, signature))
    {
        ctx_.errStr = "[Handshake] Failed to sign reply";
        return false;
    }

    std::memcpy(reply->signature, signature.data(), signature.length());

    ctx_.outQueue.push(std::move(frame));

    return true;
}

bool ServerHandshake::computeSecret()
{
    if (!Crypto::computeSecretX25519(serverEphemPrivKey_, clientEphemPubKey_, ctx_.secret))
    {
        ctx_.errStr = "[Handshake] Failed to compute shared secret";
        return false;
    }

    return true;
}
