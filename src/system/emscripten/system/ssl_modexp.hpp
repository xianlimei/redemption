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
   Copyright (C) Wallix 2016
   Author(s): Christophe Grosjean

*/

#pragma once

#include "system/ssl_bignum.hpp"

static inline size_t mod_exp(uint8_t * out, size_t out_len,
               const uint8_t * inr, size_t in_len,
               const uint8_t * modulus, size_t modulus_size,
               const uint8_t * exponent, size_t exponent_size)
{
    Bignum mod(modulus, modulus_size);
    Bignum exp(exponent, exponent_size);
    Bignum x(inr, in_len);
    Bignum y = x.mod_exp(exp, mod); 
    return y.get_bin(out, out_len);
}

