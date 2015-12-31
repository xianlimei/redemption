/*
 * Copyright (c) 2013 WALLIX, SARL. All rights reserved.
 * Licensed computer software. Property of WALLIX.
 * Product Name : Wallix Admin Bastion
 * Author(s)    : Tristan de Cacqueray <tdc@wallix.com>
 * Id           : $Id$
 * URL          : $URL$
 */

#ifndef WABCRYPTOFILE_H
#define WABCRYPTOFILE_H

#include "utils/genrandom.hpp"
#include "openssl_crypto.hpp"
#include "openssl_evp.hpp"

enum crypto_file_state {
    CF_EOF = 1
};

#define MIN(x, y)               (((x) > (y) ? (y) : (x)))
#define AES_BLOCK_SIZE          16
#define WABCRYPTOFILE_MAGIC     0x4D464357
#define WABCRYPTOFILE_EOF_MAGIC 0x5743464D
#define WABCRYPTOFILE_VERSION   0x00000001

enum {
    DERIVATOR_LENGTH = 8
};

/* size of salt to protect master key */
#define MKSALT_LEN 8

/* for HMAC calculations */
#define MD_HASH_FUNC   SHA256
#define MD_HASH_NAME   "SHA256"
#define MD_HASH_LENGTH SHA256_DIGEST_LENGTH

#define CRYPTO_BUFFER_SIZE ((4096 * 4))

/* 256 bits key size */
#define CRYPTO_KEY_LENGTH 32
#define HMAC_KEY_LENGTH   CRYPTO_KEY_LENGTH

struct CryptoContext {
    Random & gen;

    CryptoContext(Random & gen) : gen(gen) {}

    unsigned char hmac_key[HMAC_KEY_LENGTH];
    unsigned char crypto_key[CRYPTO_KEY_LENGTH];

    void get_derivator(const char *const_file, unsigned char * derivator, int derivator_len)
    {
         // generate key derivator as SHA256(basename)
        char * file = strdupa(const_file);
        char * file_basename = basename(file);
        char tmp_derivated[SHA256_DIGEST_LENGTH];
        if (SHA256((unsigned char *)file_basename, 
            strlen(file_basename), 
            (unsigned char *)tmp_derivated) == nullptr)
        {
            std::printf("[CRYPTO_ERROR][%d]: Could not derivate trace crypto key, SHA256 from=%s!\n", 
                getpid(), file_basename);
            return;
        }
        memcpy(derivator, tmp_derivated, MIN(derivator_len, SHA256_DIGEST_LENGTH));
    }

    void random(void * dest, size_t size) 
    {
        this->gen.random(dest, size);
    }
};

#ifdef __cplusplus
extern "C" {
#endif

/* Standard unbase64, store result in buffer. Returns written bytes
 */
size_t unbase64(char *buffer, size_t bufsiz, const char *txt);

int compute_hmac(unsigned char * hmac, const unsigned char * key, const unsigned char * derivator);
void get_derivator(const char * file, unsigned char * derivator, int derivator_len);

#ifdef __cplusplus
}
#endif

#endif
