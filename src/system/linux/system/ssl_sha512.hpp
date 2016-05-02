/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010-2014
   Author(s): Christophe Grosjean, Javier Caverni, Meng Tan

   openssl headers

   Based on xrdp and rdesktop
   Copyright (C) Jay Sorg 2004-2010
   Copyright (C) Matthew Chapman 1999-2007
*/

#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include "core/error.hpp"
#include "openssl_crypto.hpp"
#include "utils/log.hpp"
#include "utils/bitfu.hpp"



#define SHA512_DIGEST_LENGTH 512

class SslSha512
{
    SHA512_CTX sha512;

    public:
    SslSha512()
    {
        int res = 0;
        res = SHA512_Init(&this->sha512);
        if (res == 0) {
            throw Error(ERR_SSL_CALL_SHA1_INIT_FAILED);
        }
    }

    void update(const uint8_t * const data,  size_t data_size)
    {
        int res = 0;
        res = SHA512_Update(&this->sha512, data, data_size);
        if (res == 0) {
            throw Error(ERR_SSL_CALL_SHA1_UPDATE_FAILED);
        }
    }

    void final(uint8_t * out_data, size_t out_data_size)
    {
        assert(SHA512_DIGEST_LENGTH == out_data_size);
        int res = 0;
        res = SHA512_Final(out_data, &this->sha512);
        if (res == 0) {
            throw Error(ERR_SSL_CALL_SHA1_FINAL_FAILED);
        }
    }
};


#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))


class Sslsha512_direct
{
    /* public domain sha512 implementation based on rfc1321 and libtomcrypt */

    struct sha512 {
	    uint32_t datalen;          /*!< number of bytes processed  */
        uint32_t state[8];          /*!< intermediate digest state  */
        unsigned char data[64];   /*!< data block being processed */
        uint32_t bitlen;
    } sha512;


    //static uint32_t ror(uint32_t n, int k) { return (n >> k) | (n << (32-k)); }

    static const WORD k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };


    static void processblock(struct sha512 *s, const uint8_t *buf)
    {
	    WORD a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

        for (i = 0, j = 0; i < 16; ++i, j += 4)
            m[i] = (buf[j] << 24) | (buf[j + 1] << 16) | (buf[j + 2] << 8) | (buf[j + 3]);
        for ( ; i < 64; ++i)
            m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

        a = s->state[0];
        b = s->state[1];
        c = s->state[2];
        d = s->state[3];
        e = s->state[4];
        f = s->state[5];
        g = s->state[6];
        h = s->state[7];

        for (i = 0; i < 64; ++i) {
            t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
            t2 = EP0(a) + MAJ(a,b,c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        s->state[0] += a;
        s->state[1] += b;
        s->state[2] += c;
        s->state[3] += d;
        s->state[4] += e;
        s->state[5] += f;
        s->state[6] += g;
        s->state[7] += h;
    }

    static void sha512_init(struct sha512 *s)
    {
        s->datalen = 0;
        s->bitlen = 0;
        s->state[0] = 0x6a09e667;
        s->state[1] = 0xbb67ae85;
        s->state[2] = 0x3c6ef372;
        s->state[3] = 0xa54ff53a;
        s->state[4] = 0x510e527f;
        s->state[5] = 0x9b05688c;
        s->state[6] = 0x1f83d9ab;
        s->state[7] = 0x5be0cd19;
    }

    static void sha512_final(struct sha512 *s, uint8_t *md)
    {
        WORD i;

        i = s->datalen;

        // Pad whatever data is left in the buffer.
        if (s->datalen < 56) {
            s->data[i++] = 0x80;
            while (i < 56)
                s->data[i++] = 0x00;
        }
        else {
            s->data[i++] = 0x80;
            while (i < 64)
                s->data[i++] = 0x00;
            this->processblock(s, s->data);
            memset(s->data, 0, 56);
        }

        // Append to the padding the total message's length in bits and transform.
        s->bitlen += s->datalen * 8;
        s->data[63] = s->bitlen;
        s->data[62] = s->bitlen >> 8;
        s->data[61] = s->bitlen >> 16;
        s->data[60] = s->bitlen >> 24;
        s->data[59] = s->bitlen >> 32;
        s->data[58] = s->bitlen >> 40;
        s->data[57] = s->bitlen >> 48;
        s->data[56] = s->bitlen >> 56;
        this->processblock(s, s->data);

        // Since this implementation uses little endian byte ordering and SHA uses big endian,
        // reverse all the bytes when copying the final state to the output hash.
        for (i = 0; i < 4; ++i) {
            md[i]      = (s->state[0] >> (24 - i * 8)) & 0x000000ff;
            md[i + 4]  = (s->state[1] >> (24 - i * 8)) & 0x000000ff;
            md[i + 8]  = (s->state[2] >> (24 - i * 8)) & 0x000000ff;
            md[i + 12] = (s->state[3] >> (24 - i * 8)) & 0x000000ff;
            md[i + 16] = (s->state[4] >> (24 - i * 8)) & 0x000000ff;
            md[i + 20] = (s->state[5] >> (24 - i * 8)) & 0x000000ff;
            md[i + 24] = (s->state[6] >> (24 - i * 8)) & 0x000000ff;
            md[i + 28] = (s->state[7] >> (24 - i * 8)) & 0x000000ff;
        }
    }


    static void sha512_update(struct sha512 *s, const uint8_t *m, unsigned long len)
    {
        WORD i;

        for (i = 0; i < len; ++i) {
            s->data[ctx->datalen] = m[i];
            s->datalen++;
            if (s->datalen == 64) {
                this->processblock(s, s->data);
                s->bitlen += 512;
                s->datalen = 0;
            }
        }
    }


    public:
    Sslsha512_direct()
    {
        Sslsha512_direct::sha512_init(&this->sha512);
    }

    void update_direct(const uint8_t * const data, size_t data_size)
    {
        Sslsha512_direct::sha512_update(&this->sha512, data, data_size);
    }

    void final_direct(uint8_t * out_data, size_t out_data_size)
    {
        assert(sha512_DIGEST_LENGTH == out_data_size);
        Sslsha512_direct::sha512_final(&this->sha512, out_data);
    }
};
