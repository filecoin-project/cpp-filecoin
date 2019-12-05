/* libbls_signatures Header Version 0.1.0 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define DIGEST_BYTES 96

#define PRIVATE_KEY_BYTES 32

#define PUBLIC_KEY_BYTES 48

#define SIGNATURE_BYTES 96

typedef uint8_t BLSSignature[SIGNATURE_BYTES];

/**
 * AggregateResponse
 */
typedef struct {
  BLSSignature signature;
} AggregateResponse;

typedef uint8_t BLSDigest[DIGEST_BYTES];

/**
 * HashResponse
 */
typedef struct {
  BLSDigest digest;
} HashResponse;

typedef uint8_t BLSPrivateKey[PRIVATE_KEY_BYTES];

/**
 * PrivateKeyGenerateResponse
 */
typedef struct {
  BLSPrivateKey private_key;
} PrivateKeyGenerateResponse;

typedef uint8_t BLSPublicKey[PUBLIC_KEY_BYTES];

/**
 * PrivateKeyPublicKeyResponse
 */
typedef struct {
  BLSPublicKey public_key;
} PrivateKeyPublicKeyResponse;

/**
 * PrivateKeySignResponse
 */
typedef struct {
  BLSSignature signature;
} PrivateKeySignResponse;

/**
 * Aggregate signatures together into a new signature
 * # Arguments
 *  `flattened_signatures_ptr` - pointer to a byte array containing signatures
 *  `flattened_signatures_len` - length of the byte array (multiple of SIGNATURE_BYTES)
 * Returns `NULL` on error. Result must be freed using `destroy_aggregate_response`.
 */
AggregateResponse *aggregate(const uint8_t *flattened_signatures_ptr,
                             size_t flattened_signatures_len);

void destroy_aggregate_response(AggregateResponse *ptr);

void destroy_hash_response(HashResponse *ptr);

void destroy_private_key_generate_response(PrivateKeyGenerateResponse *ptr);

void destroy_private_key_public_key_response(PrivateKeyPublicKeyResponse *ptr);

void destroy_private_key_sign_response(PrivateKeySignResponse *ptr);

/**
 * Compute the digest of a message
 * # Arguments
 *  `message_ptr` - pointer to a message byte array
 *  `message_len` - length of the byte array
 */
HashResponse *hash(const uint8_t *message_ptr, size_t message_len);

/**
 * Generate a new private key
 * # Arguments
 *  `raw_seed_ptr` - pointer to a seed byte array
 */
PrivateKeyGenerateResponse *private_key_generate(void);

/**
 * Generate the public key for a private key
 * # Arguments
 *  `raw_private_key_ptr` - pointer to a private key byte array
 * Returns `NULL` when passed invalid arguments.
 */
PrivateKeyPublicKeyResponse *private_key_public_key(const uint8_t *raw_private_key_ptr);

/**
 * Sign a message with a private key and return the signature
 * # Arguments
 *  `raw_private_key_ptr` - pointer to a private key byte array
 *  `message_ptr` - pointer to a message byte array
 *  `message_len` - length of the byte array
 * Returns `NULL` when passed invalid arguments.
 */
PrivateKeySignResponse *private_key_sign(const uint8_t *raw_private_key_ptr,
                                         const uint8_t *message_ptr,
                                         size_t message_len);

/**
 * Verify that a signature is the aggregated signature of hashes - pubkeys
 * # Arguments
 *  `signature_ptr`             - pointer to a signature byte array (SIGNATURE_BYTES long)
 *  `flattened_digests_ptr`     - pointer to a byte array containing digests
 *  `flattened_digests_len`     - length of the byte array (multiple of DIGEST_BYTES)
 *  `flattened_public_keys_ptr` - pointer to a byte array containing public keys
 */
int verify(const uint8_t *signature_ptr,
           const uint8_t *flattened_digests_ptr,
           size_t flattened_digests_len,
           const uint8_t *flattened_public_keys_ptr,
           size_t flattened_public_keys_len);
