/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      Header that exposes the platform agnostic CHIP crypto primitives
 */

#ifndef _CHIP_CRYPTO_PAL_H_
#define _CHIP_CRYPTO_PAL_H_

#if CHIP_SEPARATE_CONFIG_H
#include <crypto/CryptoBuildConfig.h>
#endif

#include <core/CHIPError.h>
#include <stddef.h>
#include <string.h>

namespace chip {
namespace Crypto {

const size_t kP256_FE_Length     = 32;
const size_t kP256_Point_Length  = (2 * kP256_FE_Length + 1);
const size_t kSHA256_Hash_Length = 32;

const size_t kMax_ECDH_Secret_Length     = kP256_FE_Length;
const size_t kMax_ECDSA_Signature_Length = 72;
const size_t kMAX_FE_Length              = kP256_FE_Length;
const size_t kMAX_Point_Length           = kP256_Point_Length;
const size_t kMAX_Hash_Length            = kSHA256_Hash_Length;
const size_t kMAX_CSR_Length             = 512;

const size_t kP256_PrivateKey_Length = 32;
const size_t kP256_PublicKey_Length  = 65;

/* These sizes are hardcoded here to remove header dependency on underlying crypto library
 * in a public interface file. The validity of these sizes is verified by static_assert in
 * the implementation files.
 */
const size_t kMAX_Spake2p_Context_Size     = 1024;
const size_t kMAX_Hash_SHA256_Context_Size = 256;

/**
 * Spake2+ parameters for P256
 * Defined in https://www.ietf.org/id/draft-bar-cfrg-spake2plus-01.html#name-ciphersuites
 */
const uint8_t spake2p_M_p256[65] = {
    0x04, 0x88, 0x6e, 0x2f, 0x97, 0xac, 0xe4, 0x6e, 0x55, 0xba, 0x9d, 0xd7, 0x24, 0x25, 0x79, 0xf2, 0x99,
    0x3b, 0x64, 0xe1, 0x6e, 0xf3, 0xdc, 0xab, 0x95, 0xaf, 0xd4, 0x97, 0x33, 0x3d, 0x8f, 0xa1, 0x2f, 0x5f,
    0xf3, 0x55, 0x16, 0x3e, 0x43, 0xce, 0x22, 0x4e, 0x0b, 0x0e, 0x65, 0xff, 0x02, 0xac, 0x8e, 0x5c, 0x7b,
    0xe0, 0x94, 0x19, 0xc7, 0x85, 0xe0, 0xca, 0x54, 0x7d, 0x55, 0xa1, 0x2e, 0x2d, 0x20,
};
const uint8_t spake2p_N_p256[65] = {
    0x04, 0xd8, 0xbb, 0xd6, 0xc6, 0x39, 0xc6, 0x29, 0x37, 0xb0, 0x4d, 0x99, 0x7f, 0x38, 0xc3, 0x77, 0x07,
    0x19, 0xc6, 0x29, 0xd7, 0x01, 0x4d, 0x49, 0xa2, 0x4b, 0x4f, 0x98, 0xba, 0xa1, 0x29, 0x2b, 0x49, 0x07,
    0xd6, 0x0a, 0xa6, 0xbf, 0xad, 0xe4, 0x50, 0x08, 0xa6, 0x36, 0x33, 0x7f, 0x51, 0x68, 0xc6, 0x4d, 0x9b,
    0xd3, 0x60, 0x34, 0x80, 0x8c, 0xd5, 0x64, 0x49, 0x0b, 0x1e, 0x65, 0x6e, 0xdb, 0xe7,
};

/**
 * Spake2+ state machine to ensure proper execution of the protocol.
 */
enum class CHIP_SPAKE2P_STATE : uint8_t
{
    PREINIT = 0, // Before any initialization
    INIT,        // First initialization
    STARTED,     // Prover & Verifier starts
    R1,          // Round one complete
    R2,          // Round two complete
    KC,          // Key confirmation complete
};

/**
 * Spake2+ role.
 */
enum class CHIP_SPAKE2P_ROLE : uint8_t
{
    VERIFIER = 0, // Accessory
    PROVER   = 1, // Commissioner
};

enum class SupportedECPKeyTypes : uint8_t
{
    ECP256R1 = 0,
};

class ECPKey
{
public:
    virtual SupportedECPKeyTypes Type() = 0;
    virtual size_t Length()             = 0;
    virtual operator uint8_t *()        = 0;
};

class P256PrivateKey : public ECPKey
{
public:
    SupportedECPKeyTypes Type() override { return SupportedECPKeyTypes::ECP256R1; }
    size_t Length() override { return kP256_PrivateKey_Length; }
    operator uint8_t *() override { return bytes; }

private:
    uint8_t bytes[kP256_PrivateKey_Length];
};

class P256PublicKey : public ECPKey
{
public:
    SupportedECPKeyTypes Type() override { return SupportedECPKeyTypes::ECP256R1; }
    size_t Length() override { return kP256_PublicKey_Length; }
    operator uint8_t *() override { return bytes; }

private:
    uint8_t bytes[kP256_PublicKey_Length];
};

/**
 * @brief A function that implements AES-CCM encryption
 * @param plaintext Plaintext to encrypt
 * @param plaintext_length Length of plain_text
 * @param aad Additional authentication data
 * @param aad_length Length of additional authentication data
 * @param key Encryption key
 * @param key_length Length of encryption key (in bytes)
 * @param iv Initial vector
 * @param iv_length Length of initial vector
 * @param ciphertext Buffer to write ciphertext into. Caller must ensure this is large enough to hold the ciphertext
 * @param tag Buffer to write tag into. Caller must ensure this is large enough to hold the tag
 * @param tag_length Expected length of tag
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 * */
CHIP_ERROR AES_CCM_encrypt(const uint8_t * plaintext, size_t plaintext_length, const uint8_t * aad, size_t aad_length,
                           const uint8_t * key, size_t key_length, const uint8_t * iv, size_t iv_length, uint8_t * ciphertext,
                           uint8_t * tag, size_t tag_length);

/**
 * @brief A function that implements AES-CCM decryption
 * @param ciphertext Ciphertext to decrypt
 * @param ciphertext_length Length of ciphertext
 * @param aad Additional authentical data.
 * @param aad_length Length of additional authentication data
 * @param tag Tag to use to decrypt
 * @param tag_length Length of tag
 * @param key Decryption key
 * @param key_length Length of Decryption key (in bytes)
 * @param iv Initial vector
 * @param iv_length Length of initial vector
 * @param plaintext Buffer to write plaintext into
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/

CHIP_ERROR AES_CCM_decrypt(const uint8_t * ciphertext, size_t ciphertext_length, const uint8_t * aad, size_t aad_length,
                           const uint8_t * tag, size_t tag_length, const uint8_t * key, size_t key_length, const uint8_t * iv,
                           size_t iv_length, uint8_t * plaintext);

/**
 * @brief A function that implements SHA-256 hash
 * @param data The data to hash
 * @param data_length Length of the data
 * @param out_buffer Pointer to buffer to write output into
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/

CHIP_ERROR Hash_SHA256(const uint8_t * data, const size_t data_length, uint8_t * out_buffer);

/**
 * @brief A class that defines stream based implementation of SHA-256 hash
 **/

struct HashSHA256OpaqueContext
{
    uint8_t mOpaque[kMAX_Hash_SHA256_Context_Size];
};

class Hash_SHA256_stream
{
public:
    Hash_SHA256_stream(void);
    ~Hash_SHA256_stream(void);

    CHIP_ERROR Begin(void);
    CHIP_ERROR AddData(const uint8_t * data, const size_t data_length);
    CHIP_ERROR Finish(uint8_t * out_buffer);
    void Clear(void);

private:
    HashSHA256OpaqueContext mContext;
};

/**
 * @brief A function that implements SHA-256 based HKDF
 * @param secret The secret to use as the key to the HKDF
 * @param secret_length Length of the secret
 * @param salt Optional salt to use as input to the HKDF
 * @param salt_length Length of the salt
 * @param info Optional info to use as input to the HKDF
 * @param info_length Length of the info
 * @param out_buffer Pointer to buffer to write output into.
 * @param out_length Resulting length of out_buffer
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/

CHIP_ERROR HKDF_SHA256(const uint8_t * secret, const size_t secret_length, const uint8_t * salt, const size_t salt_length,
                       const uint8_t * info, const size_t info_length, uint8_t * out_buffer, size_t out_length);

/**
 * @brief A cryptographically secure random number generator based on NIST SP800-90A
 * @param out_buffer Buffer to write random bytes into
 * @param out_length Number of random bytes to generate
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR DRBG_get_bytes(uint8_t * out_buffer, const size_t out_length);

/**
 * @brief A function to sign a msg using ECDSA
 * @param msg Message that needs to be signed
 * @param msg_length Length of message
 * @param private_key Key to use to sign the message. Private keys are ASN.1 DER encoded as padded big-endian field elements as
 *described in SEC 1: Elliptic Curve Cryptography [https://www.secg.org/sec1-v2.pdf]
 * @param private_key_length Length of private key
 * @param out_signature Buffer that will hold the output signature. The signature consists of: 2 EC elements (r and s), represented
 *as ASN.1 DER integers, plus the ASN.1 sequence Header
 * @param out_signature_length Length of out buffer
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR ECDSA_sign_msg(const uint8_t * msg, const size_t msg_length, const uint8_t * private_key,
                          const size_t private_key_length, uint8_t * out_signature, size_t & out_signature_length);

/**
 * @brief A function to sign a msg using ECDSA
 * @param msg Message that needs to be signed
 * @param msg_length Length of message
 * @param public_key Key to use to verify the message signature. Public keys are ASN.1 DER encoded as uncompressed points as
 *described in SEC 1: Elliptic Curve Cryptography [https://www.secg.org/sec1-v2.pdf]
 * @param public_key_length Length of public key
 * @param signature Signature to use for verification. The signature consists of: 2 EC elements (r and s), represented as ASN.1 DER
 *integers, plus the ASN.1 sequence Header
 * @param signature_length Length of signature
 * @return Returns a CHIP_NO_ERROR on successful verification, a CHIP_ERROR otherwise
 **/
CHIP_ERROR ECDSA_validate_msg_signature(const uint8_t * msg, const size_t msg_length, const uint8_t * public_key,
                                        const size_t public_key_length, const uint8_t * signature, const size_t signature_length);

/** @brief A function to derive a shared secret using ECDH
 * @param remote_public_key Public key of remote peer with which we are trying to establish secure channel. remote_public_key is
 *ASN.1 DER encoded as padded big-endian field elements as described in SEC 1: Elliptic Curve Cryptography
 *[https://www.secg.org/sec1-v2.pdf]
 * @param remote_public_key_length Length of remote_public_key
 * @param local_private_key Local private key. local_private_key is ASN.1 DER encoded as padded big-endian field elements as
 *described in SEC 1: Elliptic Curve Cryptography [https://www.secg.org/sec1-v2.pdf]
 * @param local_private_key_length Length of private_key_length
 * @param out_secret Buffer to write out secret into. This is a byte array representing the x coordinate of the shared secret.
 * @param out_secret_length Length of out_secret
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR ECDH_derive_secret(const uint8_t * remote_public_key, const size_t remote_public_key_length,
                              const uint8_t * local_private_key, const size_t local_private_key_length, uint8_t * out_secret,
                              size_t & out_secret_length);

/** @brief Entropy callback function
 * @param data Callback-specific data pointer
 * @param output Output data to fill
 * @param len Length of output buffer
 * @param olen The actual amount of data that was written to output buffer
 * @return 0 if success
 */
typedef int (*entropy_source)(void * data, uint8_t * output, size_t len, size_t * olen);

/** @brief A function to add entropy sources to crypto library
 * @param fn_source Function pointer to the entropy source
 * @param p_source  Data that should be provided when fn_source is called
 * @param threshold Minimum required from source before entropy is released
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR add_entropy_source(entropy_source fn_source, void * p_source, size_t threshold);

/** @brief Function to derive key using password. SHA256 hashing algorithm is used for calculating hmac.
 * @param password password used for key derivation
 * @param plen length of buffer containing password
 * @param salt salt to use as input to the KDF
 * @param slen length of salt
 * @param iteration_count number of iterations to run
 * @param key_length length of output key
 * @param output output buffer where the key will be written
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR pbkdf2_sha256(const uint8_t * password, size_t plen, const uint8_t * salt, size_t slen, unsigned int iteration_count,
                         uint32_t key_length, uint8_t * output);

/** @brief Generate a new ECP keypair.
 * @param pubkey Generated public key
 * @param privkey Generated private key
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR NewECPKeypair(ECPKey & pubkey, ECPKey & privkey);

/** @brief Generate a new Certificate Signing Request (CSR).
 * @param pubkey public key that'll be inserted in the CSR
 * @param privkey private key to sign the CSR
 * @param csr Newly generated CSR
 * @param csr_length The caller provides the length of input buffer (csr). The function returns the actual length of generated CSR.
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR NewCertificateSigningRequest(ECPKey & pubkey, ECPKey & privkey, uint8_t * csr, size_t & csr_length);

/**
 * The below class implements the draft 01 version of the Spake2+ protocol as
 * defined in https://www.ietf.org/id/draft-bar-cfrg-spake2plus-01.html.
 *
 * The following describes the protocol flows:
 *
 *     Commissioner                     Accessory
 *     ------------                     ---------
 *
 *     Init
 *     BeginProver
 *     ComputeRoundOne  ------------->
 *                                      Init
 *                                      BeginVerifier
 *                                  /-  ComputeRoundOne
 *                      <-------------  ComputeRoundTwo
 *     ComputeRoundTwo  ------------->
 *     KeyConfirm                       KeyConfirm
 *     GetKeys                          GetKeys
 *
 **/
class Spake2p
{
public:
    Spake2p(size_t fe_size, size_t point_size, size_t hash_size);
    virtual ~Spake2p(void){};

    /**
     * @brief Initialize Spake2+ with some context specific information.
     *
     * @param context     The context is arbitrary but should include information about the
     *                    protocol being run, contain the transcript for negotiation, include
     *                    the PKBDF parameters, etc.
     * @param context_len The length of the context.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR Init(const uint8_t * context, size_t context_len);

    /**
     * @brief Start the Spake2+ process as a verifier (i.e. an accessory being provisioned).
     *
     * @param my_identity       The verifier identity. May be NULL if identities are not established.
     * @param my_identity_len   The verifier identity length.
     * @param peer_identity     The peer identity. May be NULL if identities are not established.
     * @param peer_identity_len The peer identity length.
     * @param w0in              The input w0 (an output from the PBKDF).
     * @param w0in_len          The input w0 length.
     * @param Lin               The input L (a parameter baked into the device or computed with ComputeL).
     * @param Lin_len           The input L length.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR BeginVerifier(const uint8_t * my_identity, size_t my_identity_len, const uint8_t * peer_identity,
                             size_t peer_identity_len, const uint8_t * w0in, size_t w0in_len, const uint8_t * Lin, size_t Lin_len);

    /**
     * @brief Start the Spake2+ process as a prover (i.e. a commisioner).
     *
     * @param my_identity       The prover identity. May be NULL if identities are not established.
     * @param my_identity_len   The prover identity length.
     * @param peer_identity     The peer identity. May be NULL if identities are not established.
     * @param peer_identity_len The peer identity length.
     * @param w0in              The input w0 (an output from the PBKDF).
     * @param w0in_len          The input w0 length.
     * @param w1in              The input w1 (an output from the PBKDF).
     * @param w1in_len          The input w1 length.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR BeginProver(const uint8_t * my_identity, size_t my_identity_len, const uint8_t * peer_identity,
                           size_t peer_identity_len, const uint8_t * w0in, size_t w0in_len, const uint8_t * w1in, size_t w1in_len);

    /**
     * @brief Compute the first round of the protocol.
     *
     * @param out     The output first round Spake2+ contribution.
     * @param out_len The output first round Spake2+ contribution length.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR ComputeRoundOne(uint8_t * out, size_t * out_len);

    /**
     * @brief Compute the second round of the protocol.
     *
     * @param in      The peer first round Spake2+ contribution.
     * @param in_len  The peer first round Spake2+ contribution length.
     * @param out     The output second round Spake2+ contribution.
     * @param out_len The output second round Spake2+ contribution length.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR ComputeRoundTwo(const uint8_t * in, size_t in_len, uint8_t * out, size_t * out_len);

    /**
     * @brief Confirm that each party computed the same keys.
     *
     * @param in     The peer second round Spake2+ contribution.
     * @param in_len The peer second round Spake2+ contribution length.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR KeyConfirm(const uint8_t * in, size_t in_len);

    /**
     * @brief Return the shared secret.
     *
     * @param out     The output secret.
     * @param out_len The output secret length.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR GetKeys(uint8_t * out, size_t * out_len);

    CHIP_ERROR InternalHash(const uint8_t * in, size_t in_len);
    CHIP_ERROR WriteMN(void);
    CHIP_ERROR GenerateKeys(void);

    /**
     * @brief Load a field element.
     *
     *  @param in     The input big endian field element.
     *  @param in_len The size of the input buffer in bytes.
     *  @param fe     A pointer to an initialized implementation dependant field element.
     *
     *  @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR FELoad(const uint8_t * in, size_t in_len, void * fe) = 0;

    /**
     * @brief Write a field element in big-endian format.
     *
     *  @param fe      The field element to write.
     *  @param out     The output buffer.
     *  @param out_len The length of the output buffer.
     *
     *  @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR FEWrite(const void * fe, uint8_t * out, size_t out_len) = 0;

    /**
     * @brief Generate a field element.
     *
     *  @param fe  A pointer to an initialized implementation dependant field element.
     *
     *  @requirements The implementation must generate a random element from [0, q) where q is the curve order.
     *
     *  @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR FEGenerate(void * fe) = 0;

    /**
     * @brief Multiply two field elements, fer = fe1 * fe2.
     *
     *  @param fer   A pointer to an initialized implementation dependant field element.
     *  @param fe1  A pointer to an initialized implementation dependant field element.
     *  @param fe2  A pointer to an initialized implementation dependant field element.
     *
     *  @requirements The result must be a field element (i.e. reduced by the curve order).
     *
     *  @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR FEMul(void * fer, const void * fe1, const void * fe2) = 0;

    /**
     * @brief Load a point from 0x04 || X || Y format
     *
     * @param in     Input buffer
     * @param in_len Input buffer length
     * @param R      A pointer to an initialized implementation dependant point.
     *
     *  @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR PointLoad(const uint8_t * in, size_t in_len, void * R) = 0;

    /**
     * @brief Write a point in 0x04 || X || Y format
     *
     * @param R       A pointer to an initialized implementation dependant point.
     * @param out     Output buffer
     * @param out_len Length of the output buffer
     *
     *  @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR PointWrite(const void * R, uint8_t * out, size_t out_len) = 0;

    /**
     * @brief Scalar multiplication, R = fe1 * P1.
     *
     * @param R   Resultant point
     * @param P1  Input point
     * @param fe1  Input field element.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR PointMul(void * R, const void * P1, const void * fe1) = 0;

    /**
     * @brief Scalar multiplication with addition, R = fe1 * P1 + fe2 * P2.
     *
     * @param R   Resultant point
     * @param P1  Input point
     * @param fe1  Input field element.
     * @param P2  Input point
     * @param fe2  Input field element.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR PointAddMul(void * R, const void * P1, const void * fe1, const void * P2, const void * fe2) = 0;

    /**
     * @brief Point inversion.
     *
     * @param R   Input/Output point to point_invert
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR PointInvert(void * R) = 0;

    /**
     * @brief Multiply a point by the curve cofactor.
     *
     * @param R   Input/Output point to point_invert
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR PointCofactorMul(void * R) = 0;

    /*
     *   @synopsis Check if a point is on the curve.
     *
     *   @param R   Input point to check.
     *
     *   @return CHIP_NO_ERROR if the point is valid, CHIP_ERROR otherwise.
     */
    virtual CHIP_ERROR PointIsValid(void * R) = 0;

    /*
     *   @synopsis Compute w1in*G
     *
     *   @param Lout        Output point in 0x04 || X || Y format.
     *   @param L_len       Output point length
     *   @param w1in        Input field element
     *   @param w1in_len    Input field element size
     *
     *   @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR ComputeL(uint8_t * Lout, size_t * L_len, const uint8_t * w1in, size_t w1in_len) = 0;

    void * M;
    void * N;
    void * G;
    void * X;
    void * Y;
    void * L;
    void * Z;
    void * V;
    void * w0;
    void * w1;
    void * xy;
    void * order;
    void * tempbn;

protected:
    /**
     * @brief Initialize underlying implementation curve, points, field elements, etc.
     *
     * @details The implementation needs to:
     *     1. Initialize each of the points below and set the relevant pointers on the class:
     *        a. M
     *        b. N
     *        c. G
     *        d. X
     *        e. Y
     *        f. L
     *        g. Z
     *        h. V
     *
     *        As an example:
     *           this.M = implementation_alloc_point();
     *     2. Initialize each of the field elements below and set the relevant pointers on the class:
     *        a. w0
     *        b. w1
     *        c. xy
     *        d. tempbn
     *     3. The hashing context should be initialized
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR InitImpl() = 0;

    /**
     * @brief Hash in_len bytes of in into the internal hash context.
     *
     * @param in     The input buffer.
     * @param in_len Size of the input buffer in bytes.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR Hash(const uint8_t * in, size_t in_len) = 0;

    /**
     * @brief Return the hash.
     *
     * @param out    Output buffer. The size is implicit and is determined by the hash used.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR HashFinalize(uint8_t * out) = 0;

    /**
     * @brief Generate a message authentication code.
     *
     * @param key     The MAC key buffer.
     * @param key_len The size of the MAC key in bytes.
     * @param in      The input buffer.
     * @param in_len  The size of the input data to MAC in bytes.
     * @param out     The output MAC buffer. Size is implicit and is determined by the hash used.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    virtual CHIP_ERROR Mac(const uint8_t * key, size_t key_len, const uint8_t * in, size_t in_len, uint8_t * out) = 0;

    /**
     * @brief Verify a message authentication code.
     *
     *  @param key     The MAC key buffer.
     *  @param key_len The size of the MAC key in bytes.
     *  @param mac     The input MAC buffer.
     *  @param mac_len The size of the MAC in bytes.
     *  @param in      The input buffer to verify.
     *  @param in_len  The size of the input data to verify in bytes.
     *
     *  @return Returns a CHIP_ERROR when the MAC doesn't validate, CHIP_NO_ERROR otherwise.
     **/
    virtual CHIP_ERROR MacVerify(const uint8_t * key, size_t key_len, const uint8_t * mac, size_t mac_len, const uint8_t * in,
                                 size_t in_len) = 0;

    /**
     * @brief Derive an key of length out_len.
     *
     * @param ikm      The input key material buffer.
     * @param ikm_len  The input key material length.
     * @param salt     The optional salt. This may be NULL.
     * @param salt_len The size of the salt in bytes.
     * @param info     The info.
     * @param info_len The size of the info in bytes.
     * @param out      The output key
     * @param out_len  The output key length
     *
     * @return Returns a CHIP_ERROR when the MAC doesn't validate, CHIP_NO_ERROR otherwise.
     **/
    virtual CHIP_ERROR KDF(const uint8_t * ikm, const size_t ikm_len, const uint8_t * salt, const size_t salt_len,
                           const uint8_t * info, const size_t info_len, uint8_t * out, size_t out_len) = 0;

    CHIP_SPAKE2P_ROLE role;
    CHIP_SPAKE2P_STATE state;
    size_t fe_size;
    size_t hash_size;
    size_t point_size;
    uint8_t Kcab[kMAX_Hash_Length];
    uint8_t Kae[kMAX_Hash_Length];
    uint8_t * Kca;
    uint8_t * Kcb;
    uint8_t * Ka;
    uint8_t * Ke;
};

struct Spake2pOpaqueContext
{
    uint8_t mOpaque[kMAX_Spake2p_Context_Size];
};

class Spake2p_P256_SHA256_HKDF_HMAC : public Spake2p
{
public:
    Spake2p_P256_SHA256_HKDF_HMAC(void) : Spake2p(kP256_FE_Length, kP256_Point_Length, kSHA256_Hash_Length)
    {
        memset(&mSpake2pContext, 0, sizeof(mSpake2pContext));
    }

    virtual ~Spake2p_P256_SHA256_HKDF_HMAC(void) { FreeImpl(); }

    CHIP_ERROR Mac(const uint8_t * key, size_t key_len, const uint8_t * in, size_t in_len, uint8_t * out);
    CHIP_ERROR MacVerify(const uint8_t * key, size_t key_len, const uint8_t * mac, size_t mac_len, const uint8_t * in,
                         size_t in_len);
    CHIP_ERROR FELoad(const uint8_t * in, size_t in_len, void * fe);
    CHIP_ERROR FEWrite(const void * fe, uint8_t * out, size_t out_len);
    CHIP_ERROR FEGenerate(void * fe);
    CHIP_ERROR FEMul(void * fer, const void * fe1, const void * fe2);

    CHIP_ERROR PointLoad(const uint8_t * in, size_t in_len, void * R);
    CHIP_ERROR PointWrite(const void * R, uint8_t * out, size_t out_len);
    CHIP_ERROR PointMul(void * R, const void * P1, const void * fe1);
    CHIP_ERROR PointAddMul(void * R, const void * P1, const void * fe1, const void * P2, const void * fe2);
    CHIP_ERROR PointInvert(void * R);
    CHIP_ERROR PointCofactorMul(void * R);
    CHIP_ERROR PointIsValid(void * R);
    CHIP_ERROR ComputeL(uint8_t * Lout, size_t * L_len, const uint8_t * w1in, size_t w1in_len);

protected:
    CHIP_ERROR InitImpl();
    CHIP_ERROR Hash(const uint8_t * in, size_t in_len);
    CHIP_ERROR HashFinalize(uint8_t * out);
    CHIP_ERROR KDF(const uint8_t * secret, const size_t secret_length, const uint8_t * salt, const size_t salt_length,
                   const uint8_t * info, const size_t info_length, uint8_t * out, size_t out_length);

private:
    /**
     * @brief Free any underlying implementation curve, points, field elements, etc.
     **/
    void FreeImpl();

    CHIP_ERROR InitInternal();
    Hash_SHA256_stream sha256_hash_ctx;

    Spake2pOpaqueContext mSpake2pContext;
};

/** @brief Clears the first `len` bytes of memory area `buf`.
 * @param buf Pointer to a memory buffer holding secret data that should be cleared.
 * @param len Specifies secret data size in bytes.
 **/
void ClearSecretData(uint8_t * buf, uint32_t len);

} // namespace Crypto
} // namespace chip

#endif
