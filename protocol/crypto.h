/**
 * @file crypto.h
 * @brief Cryptographic operations
 */

#pragma once

#include <string>

#include "fast_vec_uchar.h"

/**
 * @enum KeyType
 * @brief Cryptographic key type
 */
enum class KeyType
{
    ED25519,
    X25519
};

/**
 * @class Crypto
 * @brief Set of cryptographic operations
 */
class Crypto
{
public:
    /**
     * @brief Generate a key pair (Base64)
     * @param keyType Key type
     * @param [out] privKey Private key
     * @param [out] pubKey Public key
     * @return true - if generation is successful
     */
    static bool keygen(KeyType keyType, std::string& privKey, std::string& pubKey);

    /**
     * @brief Obtain a public key from an ED25519 private key (Base64-encoded 32-byte value)
     * @param [in] privKey Private key
     * @param [out] pubKey Public key
     * @return true - on success
     */
    static bool getPubKeyED25519(const std::string& privKey, std::string& pubKey);

    /**
     * @brief Sign a message using an ED25519 private key
     * @param [in] privKey Private key
     * @param [in] msg Message
     * @param [in] msgLen Length of the data to be signed
     * @param [out] signature Message signature
     * @return true - if signing is successful
     */
    static bool signED25519(const std::string& privKey,
                            const FastVecUchar& msg,
                            size_t msgLen,
                            std::string& signature);

    /**
     * @brief Verify a signature using an ED25519 public key
     * @param [in] pubKey Public key
     * @param [in] msg Message
     * @param [in] msgLen Length of the data to be verified
     * @param [in] signature Message signature
     * @return true - if the signature is valid
     */
    static bool verifyED25519(const std::string& pubKey,
                              const FastVecUchar& msg,
                              size_t msgLen,
                              const std::string& signature);

    /**
     * @brief Compute the shared secret
     * @param [in] selfPrivKey Self private key
     * @param [in] peerPubKey Public key of the peer
     * @param [out] secret Shared secret
     * @return true - if the computation is successful
     */
    static bool computeSecretX25519(const std::string& selfPrivKey,
                                    const std::string& peerPubKey,
                                    FastVecUchar& secret);

    /**
     * @brief Encrypt a message
     * @param [in] secret Shared secret
     * @param [in] nonce Nonce value
     * @param [in, out] msg Message buffer
     * @param [in] msgLen Length of the data to be encrypted
     * @param [out] tag Authentication tag
     * @return true - if encryption is successful
     */
    static bool encryptChaCha20Poly1305(const FastVecUchar& secret,
                                        const FastVecUchar& nonce,
                                        FastVecUchar& msg,
                                        size_t msgLen,
                                        FastVecUchar& tag);

    /**
     * @brief Dencrypt a message
     * @param [in] secret Shared secret
     * @param [in] nonce Nonce value
     * @param [in, out] msg Message buffer
     * @param [in] msgLen Length of the data to be encrypted
     * @param [in] tag Authentication tag
     * @return true - if decryption is successful
     */
    static bool decryptChaCha20Poly1305(const FastVecUchar& secret,
                                        const FastVecUchar& nonce,
                                        FastVecUchar& msg,
                                        size_t msgLen,
                                        const FastVecUchar& tag);

public:
    static constexpr size_t keyRawLen = 32;
    static constexpr size_t keyB64Len = 44;
    static constexpr size_t signatureRawLen = 64;
    static constexpr size_t signatureB64Len = 88;
    static constexpr size_t secretRawLen = 32;
    static constexpr size_t nonceLen = 12;
    static constexpr size_t tagLen = 16;

private:
    /**
     * @brief Decode a Base64 block
     * @param [in] b64 String in Base64 format
     * @param [out] raw The decoded data
     * @return The length of the decoded data, or -1 on error
     */
    static int DecodeB64(const std::string& b64, FastVecUchar& raw);
};
