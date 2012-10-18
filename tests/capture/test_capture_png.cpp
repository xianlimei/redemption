/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

   Unit test to writing RDP orders to file and rereading them

*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestPNGCapture
#include <boost/test/auto_unit_test.hpp>

#define LOGPRINT
#include "test_orders.hpp"

#include "transport.hpp"
#include "image_capture.hpp"
#include "staticcapture.hpp"
#include "constants.hpp"
#include "RDP/caches/bmpcache.hpp"
#include <png.h>

    const char expected_red[] = 
    /* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
    /* 0000 */ "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
    /* 0000 */ "\x00\x00\x03\x20\x00\x00\x02\x58\x08\x02\x00\x00\x00"             //... ...X.....
    /* 0000 */ "\x15\x14\x15\x27"                                                 //...'
    /* 0000 */ "\x00\x00\x0a\xa9\x49\x44\x41\x54"                                 //....IDAT
    /* 0000 */ "\x78\x9c\xed\xd6\xc1\x09\x00\x20\x10\xc0\x30\x75\xff\x9d\xcf\x25" //x...... ..0u...%
    /* 0010 */ "\x0a\x82\x24\x13\xf4\xd9\x3d\x0b\x00\x80\xd2\x79\x1d\x00\x00\xf0" //..$...=....y....
    /* 0020 */ "\x1b\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //.....3X..1....3X
    /* 0030 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 0040 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 0050 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 0060 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0070 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0080 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 0090 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 00a0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 00b0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 00c0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 00d0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 00e0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 00f0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0100 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0110 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 0120 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 0130 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 0140 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 0150 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 0160 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 0170 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 0180 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0190 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 01a0 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 01b0 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 01c0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 01d0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 01e0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 01f0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 0200 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 0210 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0220 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0230 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 0240 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 0250 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 0260 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 0270 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 0280 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 0290 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 02a0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 02b0 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 02c0 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 02d0 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 02e0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 02f0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 0300 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 0310 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 0320 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 0330 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0340 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0350 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 0360 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 0370 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 0380 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 0390 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 03a0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 03b0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 03c0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 03d0 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 03e0 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 03f0 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 0400 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 0410 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 0420 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 0430 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 0440 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 0450 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0460 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0470 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 0480 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 0490 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 04a0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 04b0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 04c0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 04d0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 04e0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 04f0 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0500 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 0510 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 0520 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 0530 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 0540 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 0550 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 0560 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 0570 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0580 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0590 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 05a0 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 05b0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 05c0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 05d0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 05e0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 05f0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 0600 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0610 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0620 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 0630 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 0640 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 0650 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 0660 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 0670 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 0680 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 0690 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 06a0 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 06b0 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 06c0 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 06d0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 06e0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 06f0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 0700 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 0710 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 0720 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0730 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0740 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 0750 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 0760 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 0770 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 0780 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 0790 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 07a0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 07b0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 07c0 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 07d0 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 07e0 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 07f0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 0800 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 0810 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 0820 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 0830 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 0840 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0850 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0860 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 0870 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 0880 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 0890 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 08a0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 08b0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 08c0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 08d0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 08e0 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 08f0 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 0900 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 0910 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 0920 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 0930 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 0940 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 0950 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 0960 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0970 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0980 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 0990 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 09a0 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 09b0 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 09c0 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 09d0 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 09e0 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 09f0 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0a00 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0a10 */ "\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00" //X..1....3X..1...
    /* 0a20 */ "\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83" //.3X..1....3X..1.
    /* 0a30 */ "\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00" //...3X..1....3X..
    /* 0a40 */ "\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58" //1....3X..1....3X
    /* 0a50 */ "\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10" //..1....3X..1....
    /* 0a60 */ "\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05" //3X..1....3X..1..
    /* 0a70 */ "\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31" //..3X..1....3X..1
    /* 0a80 */ "\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33\x58\x00" //....3X..1....3X.
    /* 0a90 */ "\x00\x31\x83\x05\x00\x10\x33\x58\x00\x00\x31\x83\x05\x00\x10\x33" //.1....3X..1....3
    /* 0aa0 */ "\x58\x00\x00\xb1\x0b\xbb\xfd\x05\xaf"                             //X........
    /* 0000 */ "\x0d\x9d\x5e\xa4"                                                 //..^.
    /* 0000 */ "\x00\x00\x00\x00\x49\x45\x4e\x44"                                 //....IEND
     /* 0000 */ "\xae\x42\x60\x82"                                                 //.B`.
    ;

BOOST_AUTO_TEST_CASE(TestTransportPngOneRedScreen)
{
    // This is how the expected raw PNG (a big flat RED 800x600 screen)
    // will look as a string

    RDPDrawable d(800, 600, RDPDrawableConfig(true));
    Rect screen_rect(0, 0, 800, 600);
    RDPOpaqueRect cmd(Rect(0, 0, 800, 600), RED);
    d.draw(cmd, screen_rect);
    TestTransport trans("TestTransportPNG", "", 0, expected_red, sizeof(expected_red)-1);;
    transport_dump_png24(&trans, d.drawable.data, 800, 600, d.drawable.rowsize);
}

BOOST_AUTO_TEST_CASE(TestImageCapturePngOneRedScreen)
{
    CheckTransport trans(expected_red, sizeof(expected_red)-1);;
    ImageCapture d(trans, 800, 600, true);
    Rect screen_rect(0, 0, 800, 600);
    RDPOpaqueRect cmd(Rect(0, 0, 800, 600), RED);
    d.draw(cmd, screen_rect);
    d.flush();
}


BOOST_AUTO_TEST_CASE(TestImageCaptureToFilePngOneRedScreen)
{
    const char * filename = "test.png";
    OutByFilenameTransport trans(filename);
    ImageCapture d(trans, 800, 600, true);
    Rect screen_rect(0, 0, 800, 600);
    RDPOpaqueRect cmd(Rect(0, 0, 800, 600), RED);
    d.draw(cmd, screen_rect);
    d.flush();
    ::close(trans.fd);
    struct stat sb;
    int status = stat(filename, &sb);
    BOOST_CHECK_EQUAL(0, status);
    BOOST_CHECK_EQUAL(2786, sb.st_size);
    ::unlink(filename);
}


BOOST_AUTO_TEST_CASE(TestImageCaptureToFilePngBlueOnRed)
{
    OutByFilenameSequenceTransport trans("path file count pid extension", "./", "test", "png");
    ImageCapture d(trans, 800, 600, true);
    Rect screen_rect(0, 0, 800, 600);
    RDPOpaqueRect cmd(Rect(0, 0, 800, 600), RED);
    d.draw(cmd, screen_rect);
    d.flush();

    {
        char filename[1024];
        strcpy(filename, trans.path);
        struct stat sb;
        int status = stat(filename, &sb);
        BOOST_CHECK_EQUAL(0, status);
        BOOST_CHECK_EQUAL(2786, sb.st_size);
        ::unlink(filename);
    }

    RDPOpaqueRect cmd2(Rect(50, 50, 100, 50), BLUE);
    d.draw(cmd2, screen_rect);
    trans.next();
    d.flush();

    {
        char filename[1024];
        strcpy(filename, trans.path);
        struct stat sb;
        int status = stat(filename, &sb);
        BOOST_CHECK_EQUAL(0, status);
        BOOST_CHECK_EQUAL(2806, sb.st_size);
        ::unlink(filename);
    }
}

//BOOST_AUTO_TEST_CASE(TestOneRedScreen)
//{
//    Rect screen_rect(0, 0, 800, 600);
//    StaticCapture consumer(800, 600, "TestOneRedScreen.png", true);
//    RDPOpaqueRect cmd(Rect(0, 0, 800, 600), RED);
//    consumer.draw(cmd, screen_rect);
//    consumer.dump_png();
//}

