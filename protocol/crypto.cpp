/**
 * @file crypto.cpp
 * @brief Cryptographic operations
 */

#include "crypto.h"

#include <cassert>

#include <openssl/evp.h>
#include <openssl/pem.h>

bool Crypto::keygen(KeyType keyType, std::string& privKey, std::string& pubKey)
{
    assert(keyType == KeyType::ED25519 || keyType == KeyType::X25519);

    // Create context
    EVP_PKEY_CTX* ctx = nullptr;

    if (keyType == KeyType::ED25519)
        ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
    else if (keyType == KeyType::X25519)
        ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr);

    if (!ctx)
        return false;

    if (EVP_PKEY_keygen_init(ctx) <= 0)
        return false;

    // Create EVP_PKEY
    EVP_PKEY* pkey = nullptr;

    if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        return false;
    }

    EVP_PKEY_CTX_free(ctx);

    // ED25519/X25519: private and public keys are always 32 bytes
    unsigned char privKeyRaw[keyRawLen] = {0};
    size_t privateRawLen = keyRawLen;

    unsigned char pubKeyRaw[keyRawLen] = {0};
    size_t publicRawLen = keyRawLen;

    if (EVP_PKEY_get_raw_private_key(pkey, privKeyRaw, &privateRawLen) <= 0 ||
        EVP_PKEY_get_raw_public_key(pkey, pubKeyRaw, &publicRawLen) <= 0)
    {
        EVP_PKEY_free(pkey);
        return false;
    }

    EVP_PKEY_free(pkey);

    // Base64-encoded ED25519/X25519: 44 characters plus a null terminator
    unsigned char privKeyB64[keyB64Len + 1] = {0};
    unsigned char pubKeyB64[keyB64Len + 1] = {0};

    if (EVP_EncodeBlock(privKeyB64, privKeyRaw, privateRawLen) != keyB64Len ||
        EVP_EncodeBlock(pubKeyB64, pubKeyRaw, publicRawLen) != keyB64Len)
    {
        return false;
    }

    // Save
    privKey.assign(reinterpret_cast<char*>(privKeyB64), keyB64Len);
    pubKey.assign(reinterpret_cast<char*>(pubKeyB64), keyB64Len);

    return true;
}

bool Crypto::getPubKeyED25519(const std::string& privKey, std::string& pubKey)
{
    assert(privKey.length() == keyB64Len);

    // Get the raw private key from Base64 (44 bytes)
    FastVecUchar privKeyRaw;

    if (DecodeB64(privKey, privKeyRaw) != keyRawLen)
        return false;

    // Create an EVP_PKEY from a raw private key
    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519,
        nullptr,
        privKeyRaw.data(),
        keyRawLen);

    if (!pkey)
        return false;

    // Get the public key; in Ed25519, it is always 32 bytes
    unsigned char pubKeyRaw[keyRawLen] = {0};
    size_t publicRawLen = keyRawLen;

    if (EVP_PKEY_get_raw_public_key(pkey, pubKeyRaw, &publicRawLen) <= 0)
    {
        EVP_PKEY_free(pkey);
        return false;
    }

    EVP_PKEY_free(pkey);

    // Encode in Base64; Ed5519 is always 44 bytes (allow 1 byte for the null terminator)
    unsigned char pubKeyB64[keyB64Len + 1] = {0};

    if (EVP_EncodeBlock(pubKeyB64, pubKeyRaw, publicRawLen) != keyB64Len)
        return false;

    // Save
    pubKey.assign(reinterpret_cast<char*>(pubKeyB64), keyB64Len);

    return true;
}

bool Crypto::signED25519(const std::string& privKey,
                         const FastVecUchar& msg,
                         size_t msgLen,
                         std::string& signature)
{
    assert(privKey.length() == keyB64Len);
    assert(msg.size() > 0);
    assert(msg.size() >= msgLen);

    // Obtain the raw (32-byte) private key from the Base64 (44-byte) string
    FastVecUchar privKeyRaw;

    if (DecodeB64(privKey, privKeyRaw) != keyRawLen)
        return false;

    // Create an EVP_PKEY from a raw private key
    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519,
        nullptr,
        privKeyRaw.data(),
        keyRawLen);

    if (!pkey)
        return false;

    // Create a context for the signature
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    if (!ctx)
    {
        EVP_PKEY_free(pkey);
        return false;
    }

    if (EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey) <= 0)
    {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    // ED25519: signature is always 64 bytes
    unsigned char signatureRaw[signatureRawLen] = {0};
    size_t signatireLen = signatureRawLen;

    if (EVP_DigestSign(ctx, signatureRaw, &signatireLen, msg.data(), msgLen) <= 0)
    {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    // Encode in Base64 (88 characters plus a null terminator)
    unsigned char sigresB64[signatureB64Len + 1] = {0};

    if (EVP_EncodeBlock(sigresB64, signatureRaw, signatureRawLen) != signatureB64Len)
        return false;

    signature.assign(reinterpret_cast<char*>(sigresB64), signatureB64Len);

    return true;
}

bool Crypto::verifyED25519(const std::string& pubKey,
                           const FastVecUchar& msg,
                           size_t msgLen,
                           const std::string& signature)
{
    assert(pubKey.length() == keyB64Len);
    assert(signature.length() == signatureB64Len);
    assert(msg.size() > 0);
    assert(msg.size() >= msgLen);

    // Obtain the raw (32-byte) public key from the Base64 (44-byte) string
    FastVecUchar pubKeyRaw;

    if (DecodeB64(pubKey, pubKeyRaw) != keyRawLen)
        return false;

    // Obtain a raw (64-byte) signature from a Base64 (88-byte) string
    FastVecUchar signatureRaw;

    if (DecodeB64(signature, signatureRaw) != signatureRawLen)
        return false;

    // Create an EVP_PKEY from a raw public key
    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519,
        nullptr,
        pubKeyRaw.data(),
        keyRawLen);

    if (!pkey)
        return false;

    // Create a context for signature verification
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();

    if (!mdctx)
    {
        EVP_PKEY_free(pkey);
        return false;
    }

    if (EVP_DigestVerifyInit(mdctx, nullptr, nullptr, nullptr, pkey) <= 0)
    {
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    if (EVP_DigestVerify(
            mdctx,
            signatureRaw.data(),
            signatureRawLen,
            msg.data(),
            msgLen) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);

    return true;
}

bool Crypto::computeSecretX25519(const std::string& selfPrivKey,
                                 const std::string& peerPubKey,
                                 FastVecUchar& secret)
{
    assert(selfPrivKey.length() == keyB64Len && peerPubKey.length() == keyB64Len);

    // Obtain the raw (32-byte) private key from its Base64-encoded (44-byte) representation
    FastVecUchar privKeyRaw;

    if (DecodeB64(selfPrivKey, privKeyRaw) != keyRawLen)
        return false;

    // Obtain the raw (32-byte) peer public key from Base64 (44 bytes)
    FastVecUchar pubKeyRaw;

    if (DecodeB64(peerPubKey, pubKeyRaw) != keyRawLen)
        return false;

    // Create an EVP_PKEY for the private key
    EVP_PKEY* privatePkey =
        EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, nullptr, privKeyRaw.data(), keyRawLen);

    if (!privatePkey)
        return false;

    // Create EVP_PKEY for public
    EVP_PKEY* publicPkey =
        EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, nullptr, pubKeyRaw.data(), keyRawLen);

    if (!publicPkey)
    {
        EVP_PKEY_free(privatePkey);
        return false;
    }

    // Set up derive
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privatePkey, nullptr);

    if (!ctx)
    {
        EVP_PKEY_free(publicPkey);
        EVP_PKEY_free(privatePkey);
        return false;
    }

    if (EVP_PKEY_derive_init(ctx) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(publicPkey);
        EVP_PKEY_free(privatePkey);
        return false;
    }

    if (EVP_PKEY_derive_set_peer(ctx, publicPkey) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(publicPkey);
        EVP_PKEY_free(privatePkey);
        return false;
    }

    // Calculate the secret, always 32 bytes
    size_t secretLen = secretRawLen;
    secret.resize(secretLen);

    if (EVP_PKEY_derive(ctx, secret.data(), &secretLen) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(publicPkey);
        EVP_PKEY_free(privatePkey);
        return false;
    }

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(publicPkey);
    EVP_PKEY_free(privatePkey);

    return true;
}

bool Crypto::encryptChaCha20Poly1305(const FastVecUchar& secret,
                                     const FastVecUchar& nonce,
                                     FastVecUchar& msg,
                                     size_t msgLen,
                                     FastVecUchar& tag)
{
    assert(secret.size() == secretRawLen);
    assert(nonce.size() == nonceLen);
    assert(msg.size() > 0);
    assert(msg.size() >= msgLen);
    assert(tag.size() == tagLen);

    // Create context for encryption
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if (!ctx)
        return false;

    // Set the algorithm
    if (EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) <= 0)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Set the secret and nonce
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, secret.data(), nonce.data()) <= 0)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Do not use AAD

    // Encrypt (in-place)
    int len = 0;

    if (EVP_EncryptUpdate(ctx, msg.data(), &len, msg.data(), msgLen) <= 0)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Finalize (ChaCha20-Poly1305 does not append any data,
    // so pass nullptr instead of a pointer with an offset)
    if (EVP_EncryptFinal_ex(ctx, nullptr, &len) <= 0)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Tag
    if (EVP_CIPHER_CTX_ctrl(
            ctx,
            EVP_CTRL_AEAD_GET_TAG,
            tagLen,
            tag.data()) <= 0)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    EVP_CIPHER_CTX_free(ctx);

    return true;
}

bool Crypto::decryptChaCha20Poly1305(const FastVecUchar& secret,
                                     const FastVecUchar& nonce,
                                     FastVecUchar& msg,
                                     size_t msgLen,
                                     const FastVecUchar& tag)
{
    assert(secret.size() == secretRawLen);
    assert(nonce.size() == nonceLen);
    assert(msg.size() > 0);
    assert(msg.size() >= msgLen);
    assert(tag.size() == tagLen);

    // Create context for decryption
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if (!ctx)
        return false;

    // Set the algorithm
    if (EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) <= 0)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Set the secret and nonce
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, secret.data(), nonce.data()) <= 0)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Decrypt (in-place)
    int len = 0;

    if (EVP_DecryptUpdate(ctx, msg.data(), &len, msg.data(), msgLen) <= 0)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // A tag must be set before finalization
    if (EVP_CIPHER_CTX_ctrl(
            ctx,
            EVP_CTRL_AEAD_SET_TAG,
            tagLen,
            const_cast<unsigned char*>(tag.data())) <= 0)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Verify the tag (ChaCha20-Poly1305 does not append data,
    // so pass nullptr instead of a pointer with an offset)
    if (EVP_DecryptFinal_ex(ctx, nullptr, &len) <= 0)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    EVP_CIPHER_CTX_free(ctx);

    return true;
}

int Crypto::DecodeB64(const std::string& b64, FastVecUchar& raw)
{
    assert(b64.length() >= 4 && b64.length() % 4 == 0);

    size_t lastNotEq = b64.find_last_not_of('=');

    // If the entire string consists of '='
    if (lastNotEq == std::string::npos)
        return -1;

    size_t padding = b64.length() - lastNotEq - 1;

    // If the string is invalid
    if (padding > 2)
        return -1;

    // In Base64, every 4 characters represent 3 bytes of the original string
    raw.resize(b64.length() / 4 * 3);

    // Decode the string
    int res = EVP_DecodeBlock(
        raw.data(),
        reinterpret_cast<const unsigned char*>(b64.c_str()),
        b64.length());

    // If the string is invalid
    if (res < 0)
        return -1;

    res -= padding;
    raw.resize(res);

    return res;
}
