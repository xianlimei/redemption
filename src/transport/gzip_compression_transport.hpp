/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2013
    Author(s): Christophe Grosjean, Raphael Zhou
*/


#pragma once

#include <zlib.h>

#include "transport.hpp"
#include "utils/stream.hpp"

static const size_t GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH = 1024 * 64;

/*****************************
* GZipCompressionInTransport
*/

// TODO -Wold-style-cast is ignored
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

class GZipCompressionInTransport : public Transport {
    Transport & source_transport;

    z_stream compression_stream;

    uint8_t compressed_data_buf[GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH];

    uint8_t * uncompressed_data;
    size_t    uncompressed_data_length;
    uint8_t   uncompressed_data_buffer[GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH];

    bool inflate_pending;

public:
    explicit GZipCompressionInTransport(Transport & st, uint32_t verbose = 0)
    : Transport()
    , source_transport(st)
    , compression_stream()
    , uncompressed_data(nullptr)
    , uncompressed_data_length(0)
    , uncompressed_data_buffer()
    , inflate_pending(false) {
        this->verbose = verbose;
        int ret = ::inflateInit(&this->compression_stream);
(void)ret;
    }

    ~GZipCompressionInTransport() override {
        ::inflateEnd(&this->compression_stream);
    }

private:
    void do_recv(uint8_t ** pbuffer, size_t len) override {
        uint8_t * temp_data        = *pbuffer;
        size_t    temp_data_length = len;

        while (temp_data_length) {
            if (this->uncompressed_data_length) {
                REDASSERT(this->uncompressed_data);

                const size_t data_length = std::min<size_t>(temp_data_length, this->uncompressed_data_length);

                ::memcpy(temp_data, this->uncompressed_data, data_length);

                this->uncompressed_data        += data_length;
                this->uncompressed_data_length -= data_length;

                temp_data        += data_length;
                temp_data_length -= data_length;
            }
            else {
                if (!this->inflate_pending) {
                    auto end = this->compressed_data_buf;
                    this->source_transport.recv(
                          &end
                        , 5                 // reset_decompressor(1) + compressed_data_length(4)
                        );

                    InStream compressed_data(this->compressed_data_buf);

                    if (compressed_data.in_uint8() == 1) {
                        REDASSERT(this->inflate_pending == false);

                        if (this->verbose) {
                            LOG(LOG_INFO, "GZipCompressionInTransport::do_recv: Decompressor reset");
                        }
                        ::inflateEnd(&this->compression_stream);

                        ::memset(&this->compression_stream, 0, sizeof(this->compression_stream));

                        int ret = ::inflateInit(&this->compression_stream);
(void)ret;
                    }

                    const size_t compressed_data_length = compressed_data.in_uint32_le();
                    if (this->verbose) {
                        LOG( LOG_INFO, "GZipCompressionInTransport::do_recv: compressed_data_length=%zu"
                           , compressed_data_length);
                    }

                    end = this->compressed_data_buf;
                    this->source_transport.recv(&end, compressed_data_length);

                    this->compression_stream.avail_in = compressed_data_length;
                    this->compression_stream.next_in  = this->compressed_data_buf;
                }

                this->uncompressed_data = this->uncompressed_data_buffer;

                const size_t uncompressed_data_capacity = sizeof(this->uncompressed_data_buffer);

                this->compression_stream.avail_out = uncompressed_data_capacity;
                this->compression_stream.next_out  = this->uncompressed_data;

                int ret = ::inflate(&this->compression_stream, Z_NO_FLUSH);
                if (this->verbose & 0x2) {
                    LOG(LOG_INFO, "GZipCompressionInTransport::do_recv: inflate return %d", ret);
                }
(void)ret;

                if (this->verbose & 0x2) {
                    LOG( LOG_INFO, "GZipCompressionInTransport::do_recv: uncompressed_data_capacity=%zu avail_out=%u"
                       , uncompressed_data_capacity, this->compression_stream.avail_out);
                }
                this->uncompressed_data_length = uncompressed_data_capacity - this->compression_stream.avail_out;
                if (this->verbose) {
                    LOG( LOG_INFO, "GZipCompressionInTransport::do_recv: uncompressed_data_length=%zu"
                       , this->uncompressed_data_length);
                }

                this->inflate_pending = ((ret == 0) && (this->compression_stream.avail_out == 0));
            }
        }

        (*pbuffer) = (*pbuffer) + len;
    }
};  // class GZipCompressionInTransport


/******************************
* GZipCompressionOutTransport
*/

class GZipCompressionOutTransport : public Transport {
    Transport & target_transport;

    z_stream compression_stream;

    bool reset_compressor;

    uint8_t uncompressed_data[GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH];
    size_t  uncompressed_data_length;

    uint8_t compressed_data[GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH];
    size_t  compressed_data_length;

public:
    explicit GZipCompressionOutTransport(Transport & tt, uint32_t verbose = 0)
    : Transport()
    , target_transport(tt)
    , compression_stream()
    , reset_compressor(false)
    , uncompressed_data()
    , uncompressed_data_length(0)
    , compressed_data()
    , compressed_data_length(0) {
        this->verbose = verbose;
        int ret = ::deflateInit(&this->compression_stream, Z_DEFAULT_COMPRESSION);
(void)ret;
    }

    ~GZipCompressionOutTransport() override {
        if (this->uncompressed_data_length) {
            if (this->verbose & 0x4) {
                LOG(LOG_INFO, "GZipCompressionOutTransport::~GZipCompressionOutTransport: Compress");
            }
            this->compress(this->uncompressed_data, this->uncompressed_data_length, true);

            this->uncompressed_data_length = 0;
        }

        if (this->compressed_data_length) {
            this->send_to_target();
        }

        ::deflateEnd(&this->compression_stream);
    }

private:
    void compress(const uint8_t * const data, size_t data_length, bool end) {
        if (this->verbose) {
            LOG(LOG_INFO, "GZipCompressionOutTransport::compress: uncompressed_data_length=%zu", data_length);
        }
        if (this->verbose & 0x4) {
            LOG(LOG_INFO, "GZipCompressionOutTransport::compress: end=%s", (end ? "true" : "false"));
        }

        const int flush = (end ? Z_FINISH : Z_NO_FLUSH);

        this->compression_stream.avail_in = data_length;
        this->compression_stream.next_in  = const_cast<uint8_t *>(data);

        uint8_t temp_compressed_data[GZIP_COMPRESSION_TRANSPORT_BUFFER_LENGTH];

        do {
            this->compression_stream.avail_out = sizeof(temp_compressed_data);
            this->compression_stream.next_out  = reinterpret_cast<unsigned char *>(temp_compressed_data);

            int ret = ::deflate(&this->compression_stream, flush);
            if (this->verbose & 0x2) {
                LOG(LOG_INFO, "GZipCompressionOutTransport::compress: deflate return %d", ret);
            }
(void)ret;
            REDASSERT(ret != Z_STREAM_ERROR);

            if (this->verbose & 0x2) {
                LOG( LOG_INFO
                   , "GZipCompressionOutTransport::compress: compressed_data_capacity=%zu avail_out=%u"
                   , sizeof(temp_compressed_data), this->compression_stream.avail_out);
            }
            const size_t temp_compressed_data_length =
                sizeof(temp_compressed_data) - this->compression_stream.avail_out;
            if (this->verbose) {
                LOG( LOG_INFO
                   , "GZipCompressionOutTransport::compress: temp_compressed_data_length=%zu"
                   , temp_compressed_data_length);
            }

            for (size_t number_of_bytes_copied = 0; number_of_bytes_copied < temp_compressed_data_length; ) {
                const size_t number_of_bytes_to_copy =
                    std::min<size_t>( sizeof(this->compressed_data) - this->compressed_data_length
                                    , temp_compressed_data_length - number_of_bytes_copied);

                ::memcpy( this->compressed_data + this->compressed_data_length
                        , temp_compressed_data + number_of_bytes_copied
                        , number_of_bytes_to_copy);

                this->compressed_data_length += number_of_bytes_to_copy;
                number_of_bytes_copied       += number_of_bytes_to_copy;

                if (this->compressed_data_length == sizeof(this->compressed_data)) {
                    this->send_to_target();
                }
            }
        }
        while (this->compression_stream.avail_out == 0);
        REDASSERT(this->compression_stream.avail_in == 0);
    }

    void do_send(const uint8_t * const buffer, size_t len) override {
        if (this->verbose & 0x4) {
            LOG(LOG_INFO, "GZipCompressionOutTransport::do_send: len=%zu", len);
        }

        const uint8_t * temp_data        = buffer;
        size_t          temp_data_length = len;

        while (temp_data_length) {
            if (this->uncompressed_data_length) {
                const size_t data_length = std::min<size_t>(
                      temp_data_length
                    , sizeof(this->uncompressed_data) - this->uncompressed_data_length
                    );

                ::memcpy(this->uncompressed_data + this->uncompressed_data_length, temp_data, data_length);

                this->uncompressed_data_length += data_length;

                temp_data += data_length;
                temp_data_length -= data_length;

                if (this->uncompressed_data_length == sizeof(this->uncompressed_data)) {
                    this->compress(this->uncompressed_data, this->uncompressed_data_length, false);

                    this->uncompressed_data_length = 0;
                }
            }
            else {
                if (temp_data_length >= sizeof(this->uncompressed_data)) {
                    this->compress(temp_data, sizeof(this->uncompressed_data), false);

                    temp_data += sizeof(this->uncompressed_data);
                    temp_data_length -= sizeof(this->uncompressed_data);
                }
                else {
                    ::memcpy(this->uncompressed_data, temp_data, temp_data_length);

                    this->uncompressed_data_length = temp_data_length;

                    temp_data_length = 0;
                }
            }
        }

        if (this->verbose & 0x4) {
            LOG( LOG_INFO, "GZipCompressionOutTransport::do_send: uncompressed_data_length=%zu"
               , this->uncompressed_data_length);
        }
    }

public:
    bool next() override {
        if (this->uncompressed_data_length) {
            if (this->verbose & 0x4) {
                LOG(LOG_INFO, "GZipCompressionOutTransport::next: Compress");
            }
            this->compress(this->uncompressed_data, this->uncompressed_data_length, true);

            this->uncompressed_data_length = 0;
        }

        if (this->compressed_data_length) {
            this->send_to_target();
        }

        if (this->verbose) {
            LOG(LOG_INFO, "GZipCompressionOutTransport::next: Compressor reset");
        }

        ::deflateEnd(&this->compression_stream);

        ::memset(&this->compression_stream, 0, sizeof(this->compression_stream));

        int ret = ::deflateInit(&this->compression_stream, Z_DEFAULT_COMPRESSION);
(void)ret;

        this->reset_compressor = true;

        return this->target_transport.next();
    }

private:
    void send_to_target() {
        if (!this->compressed_data_length)
            return;

        StaticOutStream<128> buffer_stream;

        buffer_stream.out_uint8(this->reset_compressor ? 1 : 0);
        this->reset_compressor = false;

        buffer_stream.out_uint32_le(this->compressed_data_length);
        if (this->verbose) {
            LOG( LOG_INFO
               , "GZipCompressionOutTransport::send_to_target: compressed_data_length=%zu"
               , this->compressed_data_length);
        }

        this->target_transport.send(buffer_stream.get_data(), buffer_stream.get_offset());

        this->target_transport.send(this->compressed_data, this->compressed_data_length);

        this->compressed_data_length = 0;
    }

public:
    void timestamp(timeval now) override {
        this->target_transport.timestamp(now);
    }
};  // class GZipCompressionOutTransport

#pragma GCC diagnostic pop

