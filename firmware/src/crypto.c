/**
 * Cryptographic Implementation
 * Uses ECDSA or RSA for signatures, SHA-256 for hashing
 * SECURITY: Hardened with buffer size checks and error handling
 */

#include "crypto.h"
#include "platform.h"
#include <string.h>
#include <limits.h>

static bool crypto_initialized = false;

bool crypto_init(void) {
    if (crypto_initialized) {
        return true;
    }
    
    /* Platform-specific crypto initialization */
    if (!platform_crypto_init()) {
        return false;
    }
    
    crypto_initialized = true;
    return true;
}

int crypto_sha256(const uint8_t *data, size_t size, uint8_t *hash) {
    /* SECURITY: Validate all parameters */
    if (!data || !hash) {
        return CRYPTO_ERROR_INVALID_PARAM;
    }
    
    if (size == 0 || size > SIZE_MAX / 2) {
        /* Prevent integer overflow and reject empty data */
        return CRYPTO_ERROR_INVALID_PARAM;
    }
    
    if (!crypto_initialized) {
        return CRYPTO_ERROR_NOT_INITIALIZED;
    }
    
    /* Platform-specific SHA-256 implementation */
    platform_sha256(data, size, hash);
    
    /* SECURITY: Verify hash was actually computed
     * Check that hash buffer is not all zeros (unlikely for real hash) */
    bool all_zeros = true;
    for (int i = 0; i < CRYPTO_SHA256_HASH_SIZE; i++) {
        if (hash[i] != 0) {
            all_zeros = false;
            break;
        }
    }
    
    /* If platform_sha256 failed silently, we'd get zeros - reject this */
    if (all_zeros && size > 0) {
        return CRYPTO_ERROR_PLATFORM_FAILED;
    }
    
    return CRYPTO_SUCCESS;
}

int crypto_sign(const uint8_t *data, size_t size, 
                uint8_t *signature, size_t *sig_size) {
    /* SECURITY: Validate all parameters */
    if (!data || !signature || !sig_size) {
        return CRYPTO_ERROR_INVALID_PARAM;
    }
    
    if (size == 0 || size > SIZE_MAX / 2) {
        return CRYPTO_ERROR_INVALID_PARAM;
    }
    
    if (!crypto_initialized) {
        return CRYPTO_ERROR_NOT_INITIALIZED;
    }
    
    /* SECURITY: Enforce buffer size limits */
    if (*sig_size < CRYPTO_MAX_SIGNATURE_SIZE) {
        return CRYPTO_ERROR_BUFFER_TOO_SMALL;
    }
    
    /* Calculate hash first */
    uint8_t hash[CRYPTO_SHA256_HASH_SIZE];
    int hash_result = crypto_sha256(data, size, hash);
    if (hash_result != CRYPTO_SUCCESS) {
        return hash_result;
    }
    
    /* Platform-specific signing */
    size_t actual_sig_size = *sig_size;
    bool sign_success = platform_sign(hash, CRYPTO_SHA256_HASH_SIZE, 
                                       signature, &actual_sig_size);
    
    if (!sign_success) {
        return CRYPTO_ERROR_PLATFORM_FAILED;
    }
    
    /* SECURITY: Validate signature size */
    if (actual_sig_size < CRYPTO_MIN_SIGNATURE_SIZE || 
        actual_sig_size > CRYPTO_MAX_SIGNATURE_SIZE) {
        return CRYPTO_ERROR_PLATFORM_FAILED;
    }
    
    /* Update signature size */
    *sig_size = actual_sig_size;
    
    return CRYPTO_SUCCESS;
}

int crypto_verify(const uint8_t *data, size_t size, 
                  const uint8_t *signature, size_t sig_size) {
    /* SECURITY: Validate all parameters */
    if (!data || !signature) {
        return CRYPTO_ERROR_INVALID_PARAM;
    }
    
    if (size == 0 || size > SIZE_MAX / 2) {
        return CRYPTO_ERROR_INVALID_PARAM;
    }
    
    /* SECURITY: Enforce signature size limits */
    if (sig_size < CRYPTO_MIN_SIGNATURE_SIZE || 
        sig_size > CRYPTO_MAX_SIGNATURE_SIZE) {
        return CRYPTO_ERROR_INVALID_PARAM;
    }
    
    if (!crypto_initialized) {
        return CRYPTO_ERROR_NOT_INITIALIZED;
    }
    
    /* Calculate hash first */
    uint8_t hash[CRYPTO_SHA256_HASH_SIZE];
    int hash_result = crypto_sha256(data, size, hash);
    if (hash_result != CRYPTO_SUCCESS) {
        return hash_result;
    }
    
    /* Platform-specific verification */
    bool verify_success = platform_verify(hash, CRYPTO_SHA256_HASH_SIZE, 
                                         signature, sig_size);
    
    if (!verify_success) {
        return CRYPTO_ERROR_SIGNATURE_INVALID;
    }
    
    return CRYPTO_SUCCESS;
}
