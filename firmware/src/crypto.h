/**
 * Cryptographic Functions
 * Signature verification and hashing
 */

#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Maximum signature sizes */
#define CRYPTO_MAX_SIGNATURE_SIZE 512  /* Maximum signature size (RSA-2048 or ECDSA) */
#define CRYPTO_SHA256_HASH_SIZE 32     /* SHA-256 hash size */
#define CRYPTO_MIN_SIGNATURE_SIZE 64   /* Minimum signature size (ECDSA-P256) */

/* Error codes */
#define CRYPTO_SUCCESS 0
#define CRYPTO_ERROR_INVALID_PARAM 1
#define CRYPTO_ERROR_BUFFER_TOO_SMALL 2
#define CRYPTO_ERROR_PLATFORM_FAILED 3
#define CRYPTO_ERROR_SIGNATURE_INVALID 4
#define CRYPTO_ERROR_NOT_INITIALIZED 5

/* Calculate SHA-256 hash
 * Returns: CRYPTO_SUCCESS on success, error code on failure
 * hash must be at least CRYPTO_SHA256_HASH_SIZE bytes
 */
int crypto_sha256(const uint8_t *data, size_t size, uint8_t *hash);

/* Sign data (returns signature)
 * Returns: CRYPTO_SUCCESS on success, error code on failure
 * sig_size must point to buffer size, will be updated with actual signature size
 * signature buffer must be at least CRYPTO_MAX_SIGNATURE_SIZE bytes
 */
int crypto_sign(const uint8_t *data, size_t size, 
                uint8_t *signature, size_t *sig_size);

/* Verify signature
 * Returns: CRYPTO_SUCCESS if valid, CRYPTO_ERROR_SIGNATURE_INVALID if invalid, other error codes on failure
 * sig_size must be between CRYPTO_MIN_SIGNATURE_SIZE and CRYPTO_MAX_SIGNATURE_SIZE
 */
int crypto_verify(const uint8_t *data, size_t size, 
                  const uint8_t *signature, size_t sig_size);

/* Initialize crypto system (load keys, etc.) */
bool crypto_init(void);

#endif /* CRYPTO_H */
