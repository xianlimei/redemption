/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2013
 *   Author(s): Christophe Grosjean, Raphael Zhou, Jonathan Poelen, Meng Tan
 */

#ifndef REDEMPTION_TRANSPORT_IN_META_SEQUENCE_TRANSPORT_HPP
#define REDEMPTION_TRANSPORT_IN_META_SEQUENCE_TRANSPORT_HPP

#include <cerrno>
#include <fcntl.h>
#include <snappy-c.h>
#include <stdint.h>
#include <unistd.h>
#include <memory>
#include "openssl_crypto.hpp"
#include "log.hpp"
#include "transport/detail/meta_opener.hpp"
#include "transport/mixin_transport.hpp"
#include "transport/buffer/file_buf.hpp"

#include "transport/cryptofile.hpp"

#include "urandom_read.hpp"

namespace transfil {

    class decrypt_filter
    {
        char           buf[CRYPTO_BUFFER_SIZE]; //
        EVP_CIPHER_CTX ectx;                    // [en|de]cryption context
        uint32_t       pos;                     // current position in buf
        uint32_t       raw_size;                // the unciphered/uncompressed file size
        uint32_t       state;                   // enum crypto_file_state
        unsigned int   MAX_CIPHERED_SIZE;       // = MAX_COMPRESSED_SIZE + AES_BLOCK_SIZE;

    public:
        decrypt_filter() = default;
        //: pos(0)
        //, raw_size(0)
        //, state(0)
        //, MAX_CIPHERED_SIZE(0)
        //{}

        // transbuf::ifile_buf& src | transbuf::icrypto_filename_buf& src
        template<class Source>
        int open(Source & src, unsigned char * trace_key)
        {
            ::memset(this->buf, 0, sizeof(this->buf));
            ::memset(&this->ectx, 0, sizeof(this->ectx));

            this->pos = 0;
            this->raw_size = 0;
            this->state = 0;
            const size_t MAX_COMPRESSED_SIZE = ::snappy_max_compressed_length(CRYPTO_BUFFER_SIZE);
            this->MAX_CIPHERED_SIZE = MAX_COMPRESSED_SIZE + AES_BLOCK_SIZE;

            unsigned char tmp_buf[40];

            if (const ssize_t err = this->raw_read(src, tmp_buf, 40)) {
                return err;
            }

            // Check magic
            const uint32_t magic = tmp_buf[0] + (tmp_buf[1] << 8) + (tmp_buf[2] << 16) + (tmp_buf[3] << 24);
            if (magic != WABCRYPTOFILE_MAGIC) {
                LOG(LOG_ERR, "[CRYPTO_ERROR][%d]: Wrong file type %04x != %04x\n",
                    ::getpid(), magic, WABCRYPTOFILE_MAGIC);
                return -1;
            }
            const int version = tmp_buf[4] + (tmp_buf[5] << 8) + (tmp_buf[6] << 16) + (tmp_buf[7] << 24);
            if (version > WABCRYPTOFILE_VERSION) {
                LOG(LOG_ERR, "[CRYPTO_ERROR][%d]: Unsupported version %04x > %04x\n",
                    ::getpid(), version, WABCRYPTOFILE_VERSION);
                return -1;
            }

            unsigned char * const iv = tmp_buf + 8;

            const EVP_CIPHER * cipher  = ::EVP_aes_256_cbc();
            const unsigned int salt[]  = { 12345, 54321 };    // suspicious, to check...
            const int          nrounds = 5;
            unsigned char      key[32];
            const int i = ::EVP_BytesToKey(cipher, ::EVP_sha1(), reinterpret_cast<const unsigned char *>(salt),
                                           trace_key, CRYPTO_KEY_LENGTH, nrounds, key, nullptr);
            if (i != 32) {
                LOG(LOG_ERR, "[CRYPTO_ERROR][%d]: EVP_BytesToKey size is wrong\n", ::getpid());
                return -1;
            }

            ::EVP_CIPHER_CTX_init(&this->ectx);
            if(::EVP_DecryptInit_ex(&this->ectx, cipher, nullptr, key, iv) != 1) {
                LOG(LOG_ERR, "[CRYPTO_ERROR][%d]: Could not initialize decrypt context\n", ::getpid());
                return -1;
            }

            return 0;
        }

        template<class Source>
        ssize_t read(Source & src, void * data, size_t len)
        {
            if (this->state & CF_EOF) {
                //printf("cf EOF\n");
                return 0;
            }

            unsigned int requested_size = len;

            while (requested_size > 0) {
                // Check how much we have decoded
                if (!this->raw_size) {
                    // Buffer is empty. Read a chunk from file
                    /*
                     i f (-1 == ::do_chunk_read*(this)) {
                         return -1;
                }
                */
                    // TODO: avoid reading size directly into an integer, performance enhancement is minimal
                    // and it's not portable because of endianness issue => read in a buffer and decode by hand
                    unsigned char tmp_buf[4] = {};
                    if (const int err = this->raw_read(src, tmp_buf, 4)) {
                        return err;
                    }

                    uint32_t ciphered_buf_size = tmp_buf[0] + (tmp_buf[1] << 8) + (tmp_buf[2] << 16) + (tmp_buf[3] << 24);

                    if (ciphered_buf_size == WABCRYPTOFILE_EOF_MAGIC) { // end of file
                        this->state |= CF_EOF;
                        this->pos = 0;
                        this->raw_size = 0;
                    }
                    else {
                        if (ciphered_buf_size > this->MAX_CIPHERED_SIZE) {
                            LOG(LOG_ERR, "[CRYPTO_ERROR][%d]: Integrity error, erroneous chunk size!\n", ::getpid());
                            return -1;
                        }
                        else {
                            uint32_t compressed_buf_size = ciphered_buf_size + AES_BLOCK_SIZE;
                            //char ciphered_buf[ciphered_buf_size];
                            unsigned char ciphered_buf[65536];
                            //char compressed_buf[compressed_buf_size];
                            unsigned char compressed_buf[65536];

                            if (const ssize_t err = this->raw_read(src, ciphered_buf, ciphered_buf_size)) {
                                return err;
                            }

                            if (this->xaes_decrypt(ciphered_buf, ciphered_buf_size, compressed_buf, &compressed_buf_size)) {
                                return -1;
                            }

                            size_t chunk_size = CRYPTO_BUFFER_SIZE;
                            const snappy_status status = snappy_uncompress(reinterpret_cast<char *>(compressed_buf),
                                                                           compressed_buf_size, this->buf, &chunk_size);

                            switch (status)
                            {
                                case SNAPPY_OK:
                                    break;
                                case SNAPPY_INVALID_INPUT:
                                    LOG(LOG_ERR, "[CRYPTO_ERROR][%d]: Snappy decompression failed with status code INVALID_INPUT!\n", getpid());
                                    return -1;
                                case SNAPPY_BUFFER_TOO_SMALL:
                                    LOG(LOG_ERR, "[CRYPTO_ERROR][%d]: Snappy decompression failed with status code BUFFER_TOO_SMALL!\n", getpid());
                                    return -1;
                                default:
                                    LOG(LOG_ERR, "[CRYPTO_ERROR][%d]: Snappy decompression failed with unknown status code (%d)!\n", getpid(), status);
                                    return -1;
                            }

                            this->pos = 0;
                            // When reading, raw_size represent the current chunk size
                            this->raw_size = chunk_size;
                        }
                    }

                    // TODO: check that
                    if (!this->raw_size) { // end of file reached
                        break;
                    }
                }
                // remaining_size is the amount of data available in decoded buffer
                unsigned int remaining_size = this->raw_size - this->pos;
                // Check how much we can copy
                unsigned int copiable_size = MIN(remaining_size, requested_size);
                // Copy buffer to caller
                ::memcpy(static_cast<char*>(data) + (len - requested_size), this->buf + this->pos, copiable_size);
                this->pos      += copiable_size;
                requested_size -= copiable_size;
                // Check if we reach the end
                if (this->raw_size == this->pos) {
                    this->raw_size = 0;
                }
            }
            return len - requested_size;
        }

    private:
        ///\return 0 if success, otherwise a negatif number
        template<class Source>
        ssize_t raw_read(Source & src, void * data, size_t len)
        {
            ssize_t err = src.read(data, len);
            return err < ssize_t(len) ? (err < 0 ? err : -1) : 0;
        }

        int xaes_decrypt(const unsigned char *src_buf, uint32_t src_sz, unsigned char *dst_buf, uint32_t *dst_sz)
        {
            int safe_size = *dst_sz;
            int remaining_size = 0;

            /* allows reusing of ectx for multiple encryption cycles */
            if (EVP_DecryptInit_ex(&this->ectx, nullptr, nullptr, nullptr, nullptr) != 1){
                LOG(LOG_ERR, "[CRYPTO_ERROR][%d]: Could not prepare decryption context!\n", getpid());
                return -1;
            }
            if (EVP_DecryptUpdate(&this->ectx, dst_buf, &safe_size, src_buf, src_sz) != 1){
                LOG(LOG_ERR, "[CRYPTO_ERROR][%d]: Could not decrypt data!\n", getpid());
                return -1;
            }
            if (EVP_DecryptFinal_ex(&this->ectx, dst_buf + safe_size, &remaining_size) != 1){
                LOG(LOG_ERR, "[CRYPTO_ERROR][%d]: Could not finish decryption!\n", getpid());
                return -1;
            }
            *dst_sz = safe_size + remaining_size;
            return 0;
        }
    };

}

namespace transbuf {

    class ibase_file_buf
    {
    public:
        transfil::decrypt_filter decrypt;
        CryptoContext * cctx;

        int fd;

        ibase_file_buf(CryptoContext * cctx) : fd(-1) {}
        
        ~ibase_file_buf()
        {
            this->close();
        }

        int open(const char * filename)
        {
            this->close();
            this->fd = ::open(filename, O_RDONLY);
            return this->fd;
        }

        int open(const char * filename, mode_t /*mode*/)
        {
            TODO("see why mode is ignored even if it's provided as a parameter?");
            this->close();
            this->fd = ::open(filename, O_RDONLY);
            return this->fd;
        }

        int close()
        {
            if (this->is_open()) {
                const int ret = ::close(this->fd);
                this->fd = -1;
                return ret;
            }
            return 0;
        }

        bool is_open() const noexcept
        { return -1 != this->fd; }

        ssize_t read(void * data, size_t len)
        {
            TODO("this is blocking read, add support for timeout reading");
            TODO("add check for O_WOULDBLOCK, as this is is blockig it would be bad");
            size_t remaining_len = len;
            while (remaining_len) {
                ssize_t ret = ::read(this->fd, static_cast<char*>(data) + (len - remaining_len), remaining_len);
                if (ret < 0){
                    if (errno == EINTR){
                        continue;
                    }
                    // Error should still be there next time we try to read
                    if (remaining_len != len){
                        return len - remaining_len;
                    }
                    return ret;
                }
                // We must exit loop or we will enter infinite loop
                if (ret == 0){
                    break;
                }
                remaining_len -= ret;
            }
            return len - remaining_len;        
        }

        off64_t seek(off64_t offset, int whence) const
        { return lseek64(this->fd, offset, whence); }
    };

    class ifile_buf : public ibase_file_buf
    {
        public:
        ifile_buf(CryptoContext * cctx) : ibase_file_buf(cctx) {}
    };

    class ifile_buf_meta  : public ibase_file_buf
    {
        public:
        ifile_buf_meta(CryptoContext * cctx) : ibase_file_buf(cctx) {}
    };

    class ibase_crypto_file_buf
    {
        transfil::decrypt_filter decrypt;
        CryptoContext * cctx;
        ifile_buf file;

    public:
        explicit ibase_crypto_file_buf(CryptoContext * cctx)
        : cctx(cctx)
        , file(cctx)
        {}

        int open(const char * filename, mode_t mode = 0600)
        {
            unsigned char trace_key[CRYPTO_KEY_LENGTH]; // derived key for cipher
            unsigned char derivator[DERIVATOR_LENGTH];

            this->cctx->get_derivator(filename, derivator, DERIVATOR_LENGTH);
            if (-1 == this->cctx->compute_hmac(trace_key, derivator)) {
                return -1;
            }

            int err = this->file.open(filename, mode);
            if (err < 0) {
                return err;
            }

            return this->decrypt.open(this->file, trace_key);
        }

        ssize_t read(void * data, size_t len)
        { return this->decrypt.read(this->file, data, len); }

        int close()
        { return this->file.close(); }

        bool is_open() const noexcept
        { return this->file.is_open(); }

        off64_t seek(off64_t offset, int whence) const
        { return this->file.seek(offset, whence); }
    };

    class icrypto_filename_buf : public ibase_crypto_file_buf
    {
        public:
        icrypto_filename_buf(CryptoContext * cctx) 
            : ibase_crypto_file_buf(cctx) {}
    };

    class icrypto_filename_buf_meta : public ibase_crypto_file_buf
    {
        public:
        icrypto_filename_buf_meta(CryptoContext * cctx) 
            : ibase_crypto_file_buf(cctx) {}
    };

}


namespace detail {
    struct in_meta_sequence_buf_param
    {
        const char * meta_filename;
        CryptoContext * cctx_buf;
        CryptoContext * cctx_meta;
        uint32_t verbose;

        explicit in_meta_sequence_buf_param(const char * meta_filename, uint32_t verbose, CryptoContext * cctx_buf, CryptoContext * cctx_meta)
        : meta_filename(meta_filename)
        , cctx_buf(cctx_buf)
        , cctx_meta(cctx_meta)
        , verbose(verbose)
        {}
    };

    class in_meta_sequence_buf_crypto
    : public transbuf::icrypto_filename_buf
    {
        struct ReaderBuf
        {
            transbuf::icrypto_filename_buf_meta & buf;

            explicit ReaderBuf(transbuf::icrypto_filename_buf_meta & buf)
            : buf(buf)
            {}

            ssize_t read(char * buf, size_t len) const {
                return this->buf.read(buf, len);
            }
        };

        transbuf::icrypto_filename_buf_meta buf_meta;
        ReaderLine<ReaderBuf> reader;
        MetaHeader meta_header;
        MetaLine meta_line;
        char meta_path[2048];
        uint32_t verbose;

    public:
        explicit in_meta_sequence_buf_crypto(const in_meta_sequence_buf_param & params)
        : transbuf::icrypto_filename_buf(params.cctx_buf)
        , buf_meta(params.cctx_meta)
        , reader([&]() {
            if (this->buf_meta.open(params.meta_filename) < 0) {
                throw Error(ERR_TRANSPORT_OPEN_FAILED);
            }
            return ReaderBuf{this->buf_meta};
        }())
        , meta_header(read_meta_headers(this->reader))
        , verbose(params.verbose)
        {
            this->meta_line.start_time = 0;
            this->meta_line.stop_time = 0;

            this->meta_path[0] = 0;

            char basename[1024] = {};
            char extension[256] = {};

            canonical_path( params.meta_filename
                          , this->meta_path, sizeof(this->meta_path)
                          , basename, sizeof(basename)
                          , extension, sizeof(extension)
                          , this->verbose);
        }

        ssize_t read(void * data, size_t len)
        {
            if (!this->is_open()) {
                if (const int e = this->open_next()) {
                    return e;
                }
            }

            ssize_t res = this->transbuf::icrypto_filename_buf::read(data, len);
            if (res < 0) {
                return res;
            }
            if (size_t(res) != len) {
                ssize_t res2 = res;
                do {
                    if (/*const ssize_t err = */this->transbuf::icrypto_filename_buf::close()) {
                        return res;
                    }
                    data = static_cast<char*>(data) + res2;
                    if (/*const int err = */this->open_next()) {
                        return res;
                    }
                    len -= res2;
                    res2 = this->transbuf::icrypto_filename_buf::read(data, len);
                    if (res2 < 0) {
                        return res;
                    }
                    res += res2;
                } while (size_t(res2) != len);
            }
            return res;
        }

        /// \return 0 if success
        int next()
        {
            if (this->is_open()) {
                this->close();
            }

            return this->next_line();
        }

    private:
        int open_next() {
            if (const int e = this->next_line()) {
                return e < 0 ? e : -1;
            }
            const int e = this->transbuf::icrypto_filename_buf::open(this->meta_line.filename);
            return (e < 0) ? e : 0;
        }

        int next_line()
        {
            if (auto err = read_meta_file(this->reader, this->meta_header, this->meta_line)) {
                return err;
            }

            if (!file_exist(this->meta_line.filename)) {
                char original_path[1024] = {};
                char basename[1024] = {};
                char extension[256] = {};
                char filename[2048] = {};

                canonical_path( this->meta_line.filename
                              , original_path, sizeof(original_path)
                              , basename, sizeof(basename)
                              , extension, sizeof(extension)
                              , this->verbose);
                snprintf(filename, sizeof(filename), "%s%s%s", this->meta_path, basename, extension);

                if (file_exist(filename)) {
                    strcpy(this->meta_line.filename, filename);
                }
            }

            return 0;
        }

    public:
        const char * current_path() const noexcept
        { return this->meta_line.filename; }

        time_t get_begin_chunk_time() const noexcept
        { return this->meta_line.start_time; }

        time_t get_end_chunk_time() const noexcept
        { return this->meta_line.stop_time; }
    };
    
    class in_meta_sequence_buf_flat
    : public transbuf::ifile_buf
    {
        struct ReaderBuf
        {
            transbuf::ifile_buf_meta & buf;

            explicit ReaderBuf(transbuf::ifile_buf_meta & buf)
            : buf(buf)
            {}

            ssize_t read(char * buf, size_t len) const {
                return this->buf.read(buf, len);
            }
        };

        transbuf::ifile_buf_meta buf_meta;
        ReaderLine<ReaderBuf> reader;
        MetaHeader meta_header;
        MetaLine meta_line;
        char meta_path[2048];
        uint32_t verbose;

    public:
        explicit in_meta_sequence_buf_flat(const in_meta_sequence_buf_param & params)
        : transbuf::ifile_buf(params.cctx_buf)
        , buf_meta(params.cctx_meta)
        , reader([&]() {
            if (this->buf_meta.open(params.meta_filename) < 0) {
                throw Error(ERR_TRANSPORT_OPEN_FAILED);
            }
            return ReaderBuf{this->buf_meta};
        }())
        , meta_header(read_meta_headers(this->reader))
        , verbose(params.verbose)
        {
            this->meta_line.start_time = 0;
            this->meta_line.stop_time = 0;

            this->meta_path[0] = 0;

            char basename[1024] = {};
            char extension[256] = {};

            canonical_path( params.meta_filename
                          , this->meta_path, sizeof(this->meta_path)
                          , basename, sizeof(basename)
                          , extension, sizeof(extension)
                          , this->verbose);
        }

        ssize_t read(void * data, size_t len)
        {
            if (!this->is_open()) {
                if (const int e = this->open_next()) {
                    return e;
                }
            }

            ssize_t res = this->transbuf::ifile_buf::read(data, len);
            if (res < 0) {
                return res;
            }
            if (size_t(res) != len) {
                ssize_t res2 = res;
                do {
                    if (/*const ssize_t err = */this->transbuf::ifile_buf::close()) {
                        return res;
                    }
                    data = static_cast<char*>(data) + res2;
                    if (/*const int err = */this->open_next()) {
                        return res;
                    }
                    len -= res2;
                    res2 = this->transbuf::ifile_buf::read(data, len);
                    if (res2 < 0) {
                        return res;
                    }
                    res += res2;
                } while (size_t(res2) != len);
            }
            return res;
        }

        /// \return 0 if success
        int next()
        {
            if (this->is_open()) {
                this->close();
            }

            return this->next_line();
        }

    private:
        int open_next() {
            if (const int e = this->next_line()) {
                return e < 0 ? e : -1;
            }
            const int e = this->transbuf::ifile_buf::open(this->meta_line.filename);
            return (e < 0) ? e : 0;
        }

        int next_line()
        {
            if (auto err = read_meta_file(this->reader, this->meta_header, this->meta_line)) {
                return err;
            }

            if (!file_exist(this->meta_line.filename)) {
                char original_path[1024] = {};
                char basename[1024] = {};
                char extension[256] = {};
                char filename[2048] = {};

                canonical_path( this->meta_line.filename
                              , original_path, sizeof(original_path)
                              , basename, sizeof(basename)
                              , extension, sizeof(extension)
                              , this->verbose);
                snprintf(filename, sizeof(filename), "%s%s%s", this->meta_path, basename, extension);

                if (file_exist(filename)) {
                    strcpy(this->meta_line.filename, filename);
                }
            }

            return 0;
        }

    public:
        const char * current_path() const noexcept
        { return this->meta_line.filename; }

        time_t get_begin_chunk_time() const noexcept
        { return this->meta_line.start_time; }

        time_t get_end_chunk_time() const noexcept
        { return this->meta_line.stop_time; }
    };    
}

struct InMetaSequenceTransport : InputTransport<detail::in_meta_sequence_buf_flat>
{
    InMetaSequenceTransport(CryptoContext * cctx, const char * filename, const char * extension, uint32_t verbose = 0)
    : InputTransport<detail::in_meta_sequence_buf_flat>(
        detail::in_meta_sequence_buf_param(detail::temporary_concat(filename, extension).str, verbose, cctx, cctx))
    {
        this->verbose = verbose;
    }

    explicit InMetaSequenceTransport(CryptoContext * cctx, const char * filename, uint32_t verbose = 0)
    : InputTransport<detail::in_meta_sequence_buf_flat>(
            detail::in_meta_sequence_buf_param(filename, verbose, cctx, cctx))
    {
        this->verbose = verbose;
    }

    bool next() override {
        if (this->status == false) {
            throw Error(ERR_TRANSPORT_NO_MORE_DATA);
        }
        const ssize_t res = this->buffer().next();
        if (res){
            this->status = false;
            if (res < 0) {
                throw Error(ERR_TRANSPORT_READ_FAILED, -res);
            }
            throw Error(ERR_TRANSPORT_NO_MORE_DATA, errno);
        }
        ++this->seqno;
        return true;
    }

    unsigned begin_chunk_time() const noexcept
    { return this->buffer().get_begin_chunk_time(); }

    unsigned end_chunk_time() const noexcept
    { return this->buffer().get_end_chunk_time(); }

    const char * path() const noexcept
    { return this->buffer().current_path(); }
};

struct InputNextTransport_crypto : InputTransport<detail::in_meta_sequence_buf_crypto>
{
    InputNextTransport_crypto() = default;

    template<class T>
    explicit InputNextTransport_crypto(const T & buf_params)
    : InputTransport<detail::in_meta_sequence_buf_crypto>(buf_params)
    {}

    bool next() override {
        if (this->status == false) {
            throw Error(ERR_TRANSPORT_NO_MORE_DATA);
        }
        const ssize_t res = this->buffer().next();
        if (res){
            this->status = false;
            if (res < 0) {
                throw Error(ERR_TRANSPORT_READ_FAILED, -res);
            }
            throw Error(ERR_TRANSPORT_NO_MORE_DATA, errno);
        }
        ++this->seqno;
        return true;
    }

protected:
    typedef InputNextTransport_crypto TransportType;
};

struct CryptoInMetaSequenceTransport : InputNextTransport_crypto
{
    CryptoInMetaSequenceTransport(CryptoContext * cctx, const char * filename, const char * extension, uint32_t verbose = 0)
    : CryptoInMetaSequenceTransport::TransportType(
        detail::in_meta_sequence_buf_param(
            detail::temporary_concat(filename, extension).c_str(), verbose, cctx, cctx))
    {}

    CryptoInMetaSequenceTransport(CryptoContext * cctx, const char * filename, uint32_t verbose = 0)
    : CryptoInMetaSequenceTransport::TransportType(
        detail::in_meta_sequence_buf_param(filename, verbose, cctx, cctx))
    {}

    time_t begin_chunk_time() const noexcept
    { return this->buffer().get_begin_chunk_time(); }

    time_t end_chunk_time() const noexcept
    { return this->buffer().get_end_chunk_time(); }

    const char * path() const noexcept
    { return this->buffer().current_path(); }
};

#endif
