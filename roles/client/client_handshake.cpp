/**
 * @file client_handshake.cpp
 * @brief Client-side of the handshake procedure
 */

#include "client_handshake.h"

#include <cstring>
#include <cassert>

#include "packet.h"
#include "crypto.h"
#include "config.h"

ClientHandshake::ClientHandshake(SessionCtx& ctx)
    : ctx_(ctx)
{
}

bool ClientHandshake::setStaticKeys(const std::string& serverStaticPubKey)
{
    assert(!serverStaticPubKey.empty());

    clientStaticPrivKey_ = Config::data().at(Config::selfName()).key;

    if (!Crypto::getPubKeyED25519(clientStaticPrivKey_, clientStaticPubKey_))
    {
        ctx_.errStr = "[Handshake] Failed to set ED25519 key pair";
        return false;
    }

    serverStaticPubKey_ = serverStaticPubKey;

    return true;
}

bool ClientHandshake::setEphemKeys()
{
    if (!Crypto::keygen(KeyType::X25519, clientEphemPrivKey_, clientEphemPubKey_))
    {
        ctx_.errStr = "[Handshake] Failed to set X25519 ephemeral key pair";
        return false;
    }

    return true;
}

bool ClientHandshake::prepareHandshakeReq()
{
    assert(clientStaticPubKey_.length() == sizeof(HandshakeReq::clientStaticPubKey));
    assert(clientEphemPubKey_.length() == sizeof(HandshakeReq::clientEphemPubKey));

    Frame frame(sizeof(HandshakeReq));

    HandshakeReq* request = reinterpret_cast<HandshakeReq*>(frame.buf.data());

    std::memcpy(request->clientStaticPubKey, clientStaticPubKey_.data(), clientStaticPubKey_.length());
    std::memcpy(request->clientEphemPubKey, clientEphemPubKey_.data(), clientEphemPubKey_.length());

    std::string signature;

    if (!Crypto::signED25519(clientStaticPrivKey_, frame.buf, handshakeReqMsgLen, signature))
    {
        ctx_.errStr = "[Handshake] Failed to sign request";
        return false;
    }

    std::memcpy(request->signature, signature.data(), signature.length());

    ctx_.outQueue.push(std::move(frame));

    return true;
}

bool ClientHandshake::procHandshakeRep()
{
    assert(!serverStaticPubKey_.empty() && ctx_.inBuf.size() == sizeof(HandshakeRep));

    HandshakeRep* reply = reinterpret_cast<HandshakeRep*>(ctx_.inBuf.data());

    serverEphemPubKey_.assign(reply->serverEphemPubKey, sizeof(HandshakeRep::serverEphemPubKey));
    std::string signature(reply->signature, sizeof(HandshakeRep::signature));

    if (!Crypto::verifyED25519(serverStaticPubKey_, ctx_.inBuf, handshakeRepMsgLen, signature))
    {
        ctx_.errStr = "[Handshake] Reply signature verification failed";
        return false;
    }

    return true;
}

bool ClientHandshake::computeSecret()
{
    if (!Crypto::computeSecretX25519(clientEphemPrivKey_, serverEphemPubKey_, ctx_.secret))
    {
        ctx_.errStr = "[Handshake] Failed to compute shared secret";
        return false;
    }

    return true;
}
