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

   header file for use with libxrdp.so / xrdp.dll

*/

#ifndef _REDEMPTION_CORE_CLIENT_INFO_HPP_
#define _REDEMPTION_CORE_CLIENT_INFO_HPP_

#include <string.h>
#include "stream.hpp"
#include "RDP/logon.hpp"
#include "get_printable_password.hpp"

struct ClientInfo {
    int bpp = 0;
    uint16_t width = 0;
    uint16_t height = 0;

    /* bitmap cache info */
    uint8_t number_of_cache = 0;
    uint32_t cache1_entries = 600;
    bool     cache1_persistent = false;
    uint32_t cache1_size = 256;
    uint32_t cache2_entries = 300;
    bool     cache2_persistent = false;
    uint32_t cache2_size = 1024;
    uint32_t cache3_entries = 262;
    bool     cache3_persistent = false;
    uint32_t cache3_size = 4096;
    uint32_t cache4_entries = 262;
    bool     cache4_persistent = false;
    uint32_t cache4_size = 4096;
    uint32_t cache5_entries = 262;
    bool     cache5_persistent = false;
    uint32_t cache5_size = 4096;
    int cache_flags = 0;
    int bitmap_cache_version = 0; /* 0 = original version, 2 = v2 */

    /* pointer info */
    int pointer_cache_entries = 0;
    /* other */
    int use_bitmap_comp;
    int use_bitmap_cache;
    int op1; /* use smaller bitmap header, non cache */
    uint32_t desktop_cache = 0;
    bool use_compact_packets = false; /* rdp5 smaller packets */
    char hostname[512] = {};
    int build = 0;
    int keylayout = 0;
    char username[512] = {};
    char password[512] = {};
    char domain[512] = {};
    char program[512] = {};
    char directory[512] = {};

    int rdp_compression = 0;
    int rdp_compression_type = 0;
    int rdp_autologin = 0;
    int encryptionLevel; /* 1, 2, 3 = low, medium, high */
    int sound_code = 0; /* 1 = leave sound at server */
    int is_mce = 0;
    uint32_t rdp5_performanceflags = 0;
    int brush_cache_code = 0; /* 0 = no cache 1 = 8x8 standard cache
                           2 = arbitrary dimensions */
    bool console_session = false;

    InfoPacket infoPacket;

    TODO("as encryption_level, bitmap_compression and bitmap_cache are not really in client_info RDP structure we should probably not keep them here either")

    ClientInfo(const int encryptionLevel, const bool bitmap_compression, const bool bitmap_cache)
    : use_bitmap_comp(bitmap_compression?1:0)
    , use_bitmap_cache(bitmap_cache?1:0)
    /*encryptionLevel: 1, 2, 3 = low, medium, high */
    , encryptionLevel(encryptionLevel + 1) // ini->globals.encryptionLevel + 1;
    {}

    void process_logon_info( Stream & stream
                           , bool ignore_logon_password
                           , uint32_t performance_flags_default
                           , uint32_t performance_flags_force_present
                           , uint32_t performance_flags_force_not_present
                           , uint32_t password_printing_mode
                           , bool verbose
                           )
    {
        this->infoPacket.recv(stream);
        if (verbose) {
            this->infoPacket.log("Receiving from client", password_printing_mode);
        }

        memcpy(this->domain, this->infoPacket.Domain, sizeof(this->infoPacket.Domain));
        memcpy(this->username, this->infoPacket.UserName, sizeof(this->infoPacket.UserName));
        if (!ignore_logon_password){
            memcpy(this->password, this->infoPacket.Password, sizeof(this->infoPacket.Password));
        }
        else{
            if (verbose){
                LOG(LOG_INFO, "client info: logon password %s ignored",
                    ::get_printable_password(this->password, password_printing_mode));
            }
        }
        memcpy(this->program, this->infoPacket.AlternateShell, sizeof(this->infoPacket.AlternateShell));
        memcpy(this->directory, this->infoPacket.WorkingDir, sizeof(this->infoPacket.WorkingDir));

        this->rdp5_performanceflags = this->infoPacket.extendedInfoPacket.performanceFlags;

        if (this->rdp5_performanceflags == 0){
            this->rdp5_performanceflags = performance_flags_default;
        }
        this->rdp5_performanceflags |= performance_flags_force_present;
        this->rdp5_performanceflags &= ~performance_flags_force_not_present;

        if (verbose){
            LOG(LOG_INFO,
                "client info: performance flags before=0x%08X after=0x%08X default=0x%08X present=0x%08X not-present=0x%08X",
                this->infoPacket.extendedInfoPacket.performanceFlags, this->rdp5_performanceflags, performance_flags_default,
                performance_flags_force_present, performance_flags_force_not_present);
        }

        const uint32_t mandatory_flags = INFO_MOUSE
                                       | INFO_DISABLECTRLALTDEL
                                       | INFO_UNICODE
                                       | INFO_MAXIMIZESHELL
                                       ;

        if ((this->infoPacket.flags & mandatory_flags) != mandatory_flags){
            throw Error(ERR_SEC_PROCESS_LOGON_UNKNOWN_FLAGS);
        }
        if (this->infoPacket.flags & INFO_REMOTECONSOLEAUDIO) {
            this->sound_code = 1;
        }
        TODO("for now not allowing both autologon and mce");
        if ((this->infoPacket.flags & INFO_AUTOLOGON) && (!this->is_mce)){
            this->rdp_autologin = 1;
        }
        if (this->infoPacket.flags & INFO_COMPRESSION){
            this->rdp_compression      = 1;
            this->rdp_compression_type = ((this->infoPacket.flags & CompressionTypeMask) >> 9);
        }
    }
};

#endif
