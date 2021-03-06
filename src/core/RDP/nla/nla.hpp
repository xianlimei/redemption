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
  Author(s): Christophe Grosjean, Raphael Zhou, Meng Tan
*/


#pragma once

#include "core/RDP/nla/sspi.hpp"
#include "core/RDP/nla/credssp.hpp"
#include "core/RDP/nla/ntlm/ntlm.hpp"
#include "core/RDP/tpdu_buffer.hpp"
#include "utils/hexdump.hpp"
#include "utils/translation.hpp"
#include "system/ssl_sha256.hpp"

#ifndef __EMSCRIPTEN__
#include "core/RDP/nla/kerberos/kerberos.hpp"
#endif

#include "transport/transport.hpp"

#define NLA_PKG_NAME NTLMSP_NAME

/* CredSSP Client-To-Server Binding Hash */
static const uint8_t client_server_hash_magic[] =
    "CredSSP Client-To-Server Binding Hash";
/* CredSSP Server-To-Client Binding Hash */
static const uint8_t server_client_hash_magic[] =
    "CredSSP Server-To-Client Binding Hash";
static const size_t CLIENT_NONCE_LENTH = 32;

class rdpCredsspBase : noncopyable
{
protected:
    bool server;
    int send_seq_num;
    int recv_seq_num;

    TSCredentials ts_credentials;
    TSRequest ts_request;
    Array & negoToken;
    Array & pubKeyAuth;
    Array & authInfo;
    Array & clientNonce;
    uint8_t SavedClientNonce[CLIENT_NONCE_LENTH];
    Array PublicKey;
    Array ClientServerHash;
    Array ServerClientHash;

    Array ServicePrincipalName;
    SEC_WINNT_AUTH_IDENTITY identity;
    std::unique_ptr<SecurityFunctionTable> table;
    SecPkgContext_Sizes ContextSizes;
    bool RestrictedAdminMode;
    SecInterface sec_interface;

    const char * target_host;
    Random & rand;
    TimeObj & timeobj;
    std::string& extra_message;
    Translation::language_t lang;
    const bool verbose;

private:
    Transport & trans;
    char const* class_name_log;
    std::function<Ntlm_SecurityFunctionTable::PasswordCallback(SEC_WINNT_AUTH_IDENTITY&)> set_password_cb;

public:
    rdpCredsspBase(
        uint8_t const* user,
        uint8_t const* domain,
        uint8_t const* pass,
        uint8_t const* hostname,
        const char * target_host,
        const bool krb,
        const bool restricted_admin_mode,
        Random & rand,
        TimeObj & timeobj,
        std::string& extra_message,
        Translation::language_t lang,
        Transport & trans,
        char const* class_name_log,
        std::function<Ntlm_SecurityFunctionTable::PasswordCallback(SEC_WINNT_AUTH_IDENTITY&)>&& set_password_cb,
        const bool verbose = false
    )
        : server(false)
        , send_seq_num(0)
        , recv_seq_num(0)
        , ts_request(6) // Credssp Version 6 Supported
        , negoToken(ts_request.negoTokens)
        , pubKeyAuth(ts_request.pubKeyAuth)
        , authInfo(ts_request.authInfo)
        , clientNonce(ts_request.clientNonce)
        , SavedClientNonce()
        , table(new SecurityFunctionTable)
        , RestrictedAdminMode(restricted_admin_mode)
        , sec_interface(krb ? Kerberos_Interface : NTLM_Interface)
        , target_host(target_host)
        , rand(rand)
        , timeobj(timeobj)
        , extra_message(extra_message)
        , lang(lang)
        , verbose(verbose)
        , trans(trans)
        , class_name_log(class_name_log)
        , set_password_cb(std::move(set_password_cb))
    {
        if (this->verbose) {
            LOG(LOG_INFO, "%s:: Initialization", this->class_name_log);
        }
        this->set_credentials(user, domain, pass, hostname);
    }

protected:
    int credssp_ntlm_init(bool is_server) {
        if (this->verbose) {
            LOG(LOG_INFO, "%s::ntlm_init", this->class_name_log);
        }

        this->server = is_server;

        // ============================================
        /* Get Public Key From TLS Layer and hostname */
        // ============================================

        auto const key = this->trans.get_public_key();
        this->PublicKey.init(key.size());
        this->PublicKey.copy(key.data(), key.size());

        return 1;
    }

    void credssp_send() {
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredsspServer::send");
        }
        StaticOutStream<65536> ts_request_emit;
        this->ts_request.emit(ts_request_emit);
        this->trans.send(ts_request_emit.get_data(), ts_request_emit.get_offset());
    }

protected:
    void set_credentials(uint8_t const* user, uint8_t const* domain,
                         uint8_t const* pass, uint8_t const* hostname) {
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredsspClient::set_credentials");
        }
        this->identity.SetUserFromUtf8(user);
        this->identity.SetDomainFromUtf8(domain);
        this->identity.SetPasswordFromUtf8(pass);
        this->SetHostnameFromUtf8(hostname);
        // hexdump_c(user, strlen((char*)user));
        // hexdump_c(domain, strlen((char*)domain));
        // hexdump_c(pass, strlen((char*)pass));
        // hexdump_c(hostname, strlen((char*)hostname));
        this->identity.SetKrbAuthIdentity(user, pass);
    }

private:
    void SetHostnameFromUtf8(const uint8_t * pszTargetName) {
        size_t length = (pszTargetName && *pszTargetName) ? strlen(char_ptr_cast(pszTargetName)) : 0;
        this->ServicePrincipalName.init(length + 1);
        this->ServicePrincipalName.copy(pszTargetName, length);
        this->ServicePrincipalName.get_data()[length] = 0;
    }

protected:
    void InitSecurityInterface(SecInterface secInter) {
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredsspClient::InitSecurityInterface");
        }

        this->table.reset();

        switch (secInter) {
            case NTLM_Interface:
                LOG(LOG_INFO, "Credssp: NTLM Authentication");
                {
                    this->table = std::make_unique<Ntlm_SecurityFunctionTable>(this->rand, this->timeobj, this->set_password_cb);
                }
                break;
            case Kerberos_Interface:
                LOG(LOG_INFO, "Credssp: KERBEROS Authentication");
                #ifndef __EMSCRIPTEN__
                this->table = std::make_unique<Kerberos_SecurityFunctionTable>();
                #else
                assert(false && "Unsupported Kerberos");
                #endif
                break;
            default:
                this->table = std::make_unique<SecurityFunctionTable>();
        }
    }

protected:
    static void ap_integer_increment_le(uint8_t* number, size_t size) {
        size_t index = 0;

        for (index = 0; index < size; index++) {
            if (number[index] < 0xFF) {
                number[index]++;
                break;
            }

            number[index] = 0;
        }
    }

    static void ap_integer_decrement_le(uint8_t* number, size_t size) {
        size_t index = 0;
        for (index = 0; index < size; index++) {
            if (number[index] > 0) {
                number[index]--;
                break;
            }
            number[index] = 0xFF;
        }
    }

    void credssp_generate_client_nonce() {
        LOG(LOG_DEBUG, "credssp generate client nonce");
        this->rand.random(this->SavedClientNonce, CLIENT_NONCE_LENTH);
        this->credssp_set_client_nonce();
    }

    void credssp_get_client_nonce() {
        LOG(LOG_DEBUG, "credssp get client nonce");
        if (this->clientNonce.size() == CLIENT_NONCE_LENTH) {
            memcpy(this->SavedClientNonce, this->clientNonce.get_data(), CLIENT_NONCE_LENTH);
        }
    }
    void credssp_set_client_nonce() {
        LOG(LOG_DEBUG, "credssp set client nonce");
        if (this->clientNonce.size() == 0) {
            this->clientNonce.init(CLIENT_NONCE_LENTH);
            memcpy(this->clientNonce.get_data(), this->SavedClientNonce, CLIENT_NONCE_LENTH);
        }
    }

    void credssp_generate_public_key_hash(bool client_to_server) {
        LOG(LOG_DEBUG, "generate credssp public key hash (%s)",
            client_to_server ? "client->server" : "server->client");
        Array & SavedHash = client_to_server
            ? this->ClientServerHash
            : this->ServerClientHash;
        const uint8_t * magic_hash = client_to_server
            ? client_server_hash_magic
            : server_client_hash_magic;
        size_t magic_hash_len = client_to_server
            ? sizeof(client_server_hash_magic)
            : sizeof(server_client_hash_magic);
        SslSha256 sha256;
        uint8_t hash[SslSha256::DIGEST_LENGTH];
        sha256.update(magic_hash, magic_hash_len);
        sha256.update(this->SavedClientNonce, CLIENT_NONCE_LENTH);
        sha256.update(this->PublicKey.get_data(), this->PublicKey.size());
        sha256.final(hash);
        SavedHash.init(sizeof(hash));
        memcpy(SavedHash.get_data(), hash, sizeof(hash));
    }

    SEC_STATUS credssp_encrypt_public_key_echo() {
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredsspClient::encrypt_public_key_echo");
        }
        uint32_t version = this->ts_request.use_version;

        int public_key_length = this->PublicKey.size();
        uint8_t * public_key = this->PublicKey.get_data();
        if (version >= 5) {
            const bool client_to_server = !this->server;
            if (client_to_server) {
                this->credssp_generate_client_nonce();
            } else {
                this->credssp_get_client_nonce();
            }
            this->credssp_generate_public_key_hash(client_to_server);
            public_key_length = client_to_server
                ? this->ClientServerHash.size()
                : this->ServerClientHash.size();
            public_key = client_to_server
                ? this->ClientServerHash.get_data()
                : this->ServerClientHash.get_data();
        }

        SecBuffer Buffers[2]; /* Signature, TLS Public Key */

        Buffers[0].init(this->ContextSizes.cbMaxSignature);

        Buffers[1].init(public_key_length);
        Buffers[1].copy(public_key, public_key_length);

        if (this->server && version < 5) {
            // if we are server and protocol is 2,3,4
            // then echos the public key +1
            this->ap_integer_increment_le(Buffers[1].get_data(), Buffers[1].size());
        }

        SEC_STATUS const status = this->table->EncryptMessage(
            Buffers[1], Buffers[0], this->send_seq_num++);

        if (status != SEC_E_OK) {
            LOG(LOG_ERR, "EncryptMessage status: 0x%08X", status);
            return status;
        }

        // this->pubKeyAuth.init(this->ContextSizes.cbMaxSignature + public_key_length);
        this->pubKeyAuth.init(Buffers[0].size() + Buffers[1].size());
        this->pubKeyAuth.copy(Buffers[0].get_data(), Buffers[0].size());
        this->pubKeyAuth.copy(Buffers[1].get_data(), Buffers[1].size(),
                              this->ContextSizes.cbMaxSignature);
        return status;
    }

    SEC_STATUS credssp_decrypt_public_key_echo() {
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredsspClient::decrypt_public_key_echo");
        }

        if (this->pubKeyAuth.size() < this->ContextSizes.cbMaxSignature) {
            LOG(LOG_ERR, "unexpected pubKeyAuth buffer size:%zu",
                this->pubKeyAuth.size());
            if (this->pubKeyAuth.size() == 0) {
                this->extra_message = " ";
                this->extra_message.append(TR(trkeys::err_login_password, this->lang));
                LOG(LOG_INFO, "Provided login/password is probably incorrect.");
            }
            return SEC_E_INVALID_TOKEN;
        }
        int const length = this->pubKeyAuth.size();

        SecBuffer Buffers[2]; /* Signature, Encrypted TLS Public Key */

        Buffers[0].init(this->ContextSizes.cbMaxSignature);
        Buffers[0].copy(this->pubKeyAuth.get_data(), this->ContextSizes.cbMaxSignature);

        Buffers[1].init(length - this->ContextSizes.cbMaxSignature);
        Buffers[1].copy(this->pubKeyAuth.get_data() + this->ContextSizes.cbMaxSignature,
                        Buffers[1].size());

        SEC_STATUS const status = this->table->DecryptMessage(
            Buffers[1], Buffers[0], this->recv_seq_num++);

        if (status != SEC_E_OK) {
            LOG(LOG_ERR, "DecryptMessage failure: 0x%08X", status);
            return status;
        }

        const uint32_t version = this->ts_request.use_version;

        unsigned int public_key_length = this->PublicKey.size();
        uint8_t* public_key1 = this->PublicKey.get_data();
        if (version >= 5) {
            bool client_to_server = this->server;
            this->credssp_get_client_nonce();
            this->credssp_generate_public_key_hash(client_to_server);
            public_key_length = client_to_server
                ? this->ClientServerHash.size()
                : this->ServerClientHash.size();
            public_key1 = client_to_server
                ? this->ClientServerHash.get_data()
                : this->ServerClientHash.get_data();
        }

        uint8_t* public_key2 = Buffers[1].get_data();

        if (Buffers[1].size() != public_key_length) {
            LOG(LOG_ERR, "Decrypted Pub Key length or hash length does not match ! (%zu != %zu)", Buffers[1].size(), size_t(public_key_length));
            return SEC_E_MESSAGE_ALTERED; /* DO NOT SEND CREDENTIALS! */
        }

        if (!this->server && version < 5) {
            // if we are client and protocol is 2,3,4
            // then get the public key minus one
            ap_integer_decrement_le(public_key2, public_key_length);
        }
        if (memcmp(public_key1, public_key2, public_key_length) != 0) {
            LOG(LOG_ERR, "Could not verify server's public key echo");

            LOG(LOG_ERR, "Expected (length = %u):", public_key_length);
            hexdump_c(public_key1, public_key_length);

            LOG(LOG_ERR, "Actual (length = %u):", public_key_length);
            hexdump_c(public_key2, public_key_length);

            return SEC_E_MESSAGE_ALTERED; /* DO NOT SEND CREDENTIALS! */
        }

        return SEC_E_OK;
    }

    void credssp_buffer_free() {
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredsspServer::buffer_free");
        }
        this->negoToken.init(0);
        this->pubKeyAuth.init(0);
        this->authInfo.init(0);
        this->clientNonce.init(0);
        this->ts_request.error_code = 0;
    }
};

class rdpCredsspClient : public rdpCredsspBase
{
    OutTransport trans;

public:
    rdpCredsspClient(OutTransport transport,
               uint8_t * user,
               uint8_t * domain,
               uint8_t * pass,
               uint8_t * hostname,
               const char * target_host,
               const bool krb,
               const bool restricted_admin_mode,
               Random & rand,
               TimeObj & timeobj,
               std::string& extra_message,
               Translation::language_t lang,
               const bool verbose = false)
        : rdpCredsspBase(
            user, domain, pass, hostname, target_host, krb,
            restricted_admin_mode, rand, timeobj, extra_message, lang,
            transport.get_transport(), "rdpCredsspClient", {}, verbose)
        , trans(transport)
    {
    }

public:
    void credssp_encode_ts_credentials() {
        if (this->RestrictedAdminMode) {
            LOG(LOG_INFO, "Restricted Admin Mode");
            this->ts_credentials.set_credentials(nullptr, 0,
                                                 nullptr, 0,
                                                 nullptr, 0);
        }
        else {
            this->ts_credentials.set_credentials(this->identity.Domain.get_data(),
                                                 this->identity.Domain.size(),
                                                 this->identity.User.get_data(),
                                                 this->identity.User.size(),
                                                 this->identity.Password.get_data(),
                                                 this->identity.Password.size());
        }
    }

    SEC_STATUS credssp_encrypt_ts_credentials() {
        SEC_STATUS status;
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredsspClient::encrypt_ts_credentials");
        }
        this->credssp_encode_ts_credentials();

        StaticOutStream<65536> ts_credentials_send;
        this->ts_credentials.emit(ts_credentials_send);

        SecBuffer Buffers[2]; /* Signature, TSCredentials */

        this->authInfo.init(this->ContextSizes.cbMaxSignature + ts_credentials_send.get_offset());

        Buffers[0].init(this->ContextSizes.cbMaxSignature);
        memset(Buffers[0].get_data(), 0, Buffers[0].size());

        Buffers[1].init(ts_credentials_send.get_offset());
        Buffers[1].copy(ts_credentials_send.get_data(),
                               ts_credentials_send.get_offset());

        status = this->table->EncryptMessage(Buffers[1], Buffers[0], this->send_seq_num++);

        if (status != SEC_E_OK) {
            return status;
        }

        // this->authInfo.init(this->ContextSizes.cbMaxSignature + ts_credentials_send.size());
        this->authInfo.init(Buffers[0].size() + Buffers[1].size());

        this->authInfo.copy(Buffers[0].get_data(),
                            Buffers[0].size());
        this->authInfo.copy(Buffers[1].get_data(),
                            Buffers[1].size(),
                            this->ContextSizes.cbMaxSignature);
        return SEC_E_OK;
    }

private:
    enum class Res : bool { Err, Ok };

    Res sm_credssp_client_authenticate_start()
    {
        SEC_STATUS status;
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredsspClient::client_authenticate");
        }

        if (this->credssp_ntlm_init(false) == 0) {
            return Res::Err;
        }
        SecPkgInfo packageInfo;
        bool interface_changed = false;
        do {
            interface_changed = false;
            this->InitSecurityInterface(this->sec_interface);

            if (this->table == nullptr) {
                LOG(LOG_ERR, "Could not Initiate %u Security Interface!", this->sec_interface);
                return Res::Err;
            }
            packageInfo = this->table->QuerySecurityPackageInfo();
            status = this->table->AcquireCredentialsHandle(this->target_host,
                                                           SECPKG_CRED_OUTBOUND,
                                                           &this->ServicePrincipalName,
                                                           &this->identity);
            if (status == SEC_E_NO_CREDENTIALS) {
                if (this->sec_interface != NTLM_Interface) {
                    this->sec_interface = NTLM_Interface;
                    interface_changed = true;
                    LOG(LOG_INFO, "Credssp: No Kerberos Credentials, fallback to NTLM");
                }
            }
        } while (interface_changed);

        if (status != SEC_E_OK) {
            LOG(LOG_ERR, "AcquireCredentialsHandle status: 0x%08X", status);
            return Res::Err;
        }

        this->client_auth_data.cbMaxToken = packageInfo.cbMaxToken;
        this->client_auth_data.input_buffer.init(0);
        this->client_auth_data.have_input_buffer = false;
        this->ContextSizes = {};

        return Res::Ok;
    }

    struct ClientAuthenticateData
    {
        enum : uint8_t { Start, Loop, Final } state = Start;
        unsigned long cbMaxToken;
        bool have_input_buffer;
        SecBuffer input_buffer;
    };
    ClientAuthenticateData client_auth_data;

    Res sm_credssp_client_authenticate_send()
    {
        /*
         * from tspkg.dll: 0x00000132
         * ISC_REQ_MUTUAL_AUTH
         * ISC_REQ_CONFIDENTIALITY
         * ISC_REQ_USE_SESSION_KEY
         * ISC_REQ_ALLOCATE_MEMORY
         */
        SecBuffer output_buffer;
        unsigned long const fContextReq
          = ISC_REQ_MUTUAL_AUTH | ISC_REQ_CONFIDENTIALITY | ISC_REQ_USE_SESSION_KEY;

        output_buffer.init(this->client_auth_data.cbMaxToken);

        SEC_STATUS status = this->table->InitializeSecurityContext(
            char_ptr_cast(this->ServicePrincipalName.get_data()),
            fContextReq,
            this->client_auth_data.have_input_buffer
                ? &this->client_auth_data.input_buffer
                : nullptr,
            this->verbose,
            output_buffer);
        if ((status != SEC_I_COMPLETE_AND_CONTINUE) &&
            (status != SEC_I_COMPLETE_NEEDED) &&
            (status != SEC_E_OK) &&
            (status != SEC_I_CONTINUE_NEEDED)) {
            LOG(LOG_ERR, "Initialize Security Context Error !");
            return Res::Err;
        }

        if (this->client_auth_data.have_input_buffer
         && this->client_auth_data.input_buffer.size() > 0) {
            this->client_auth_data.input_buffer.init(0);
        }

        SEC_STATUS encrypted = SEC_E_INVALID_TOKEN;
        if ((status == SEC_I_COMPLETE_AND_CONTINUE) ||
            (status == SEC_I_COMPLETE_NEEDED) ||
            (status == SEC_E_OK)) {
            this->table->CompleteAuthToken(output_buffer);

            // have_pub_key_auth = true;
            this->ContextSizes = this->table->QueryContextSizes();
            encrypted = this->credssp_encrypt_public_key_echo();
            if (status == SEC_I_COMPLETE_NEEDED) {
                status = SEC_E_OK;
            }
            else if (status == SEC_I_COMPLETE_AND_CONTINUE) {
                status = SEC_I_CONTINUE_NEEDED;
            }
        }

        /* send authentication token to server */

        if (output_buffer.size() > 0) {
            // copy or set reference ? BStream
            this->negoToken.copy(output_buffer);

            // #ifdef WITH_DEBUG_CREDSSP
            //             LOG(LOG_ERR, "Sending Authentication Token");
            //             hexdump_c(this->negoToken.pvBuffer, this->negoToken.cbBuffer);
            // #endif
            if (this->verbose) {
                LOG(LOG_INFO, "rdpCredssp - Client Authentication : Sending Authentication Token");
            }

            this->credssp_send();
            this->credssp_buffer_free();
        }
        else if (encrypted == SEC_E_OK) {
            this->negoToken.init(0);
            this->credssp_send();
            this->credssp_buffer_free();
        }

        if (status != SEC_I_CONTINUE_NEEDED) {
            if (this->verbose) {
                LOG(LOG_INFO, "rdpCredssp Token loop: CONTINUE_NEEDED");
            }

            this->client_auth_data.state = ClientAuthenticateData::Final;
            return Res::Ok;
        }
        /* receive server response and place in input buffer */

        return Res::Ok;
    }

    Res sm_credssp_client_authenticate_recv(InStream & in_stream)
    {
        this->ts_request.recv(in_stream);

        // #ifdef WITH_DEBUG_CREDSSP
        //         LOG(LOG_ERR, "Receiving Authentication Token (%d)", (int) this->negoToken.cbBuffer);
        //         hexdump_c(this->negoToken.pvBuffer, this->negoToken.cbBuffer);
        // #endif
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredssp - Client Authentication : Receiving Authentication Token");
        }
        this->client_auth_data.input_buffer.copy(this->negoToken);

        this->client_auth_data.have_input_buffer = true;

        return Res::Ok;
    }

    Res sm_credssp_client_authenticate_stop(InStream & in_stream)
    {
        /* Encrypted Public Key +1 */
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredssp - Client Authentication : Receiving Encrypted PubKey + 1");
        }

        this->ts_request.recv(in_stream);

        /* Verify Server Public Key Echo */

        SEC_STATUS status = this->credssp_decrypt_public_key_echo();
        this->credssp_buffer_free();

        if (status != SEC_E_OK) {
            LOG(LOG_ERR, "Could not verify public key echo!");
            this->credssp_buffer_free();
            return Res::Err;
        }

        /* Send encrypted credentials */

        status = this->credssp_encrypt_ts_credentials();

        if (status != SEC_E_OK) {
            LOG(LOG_ERR, "credssp_encrypt_ts_credentials status: 0x%08X", status);
            return Res::Err;
        }
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredssp - Client Authentication : Sending Credentials");
        }
        this->credssp_send();

        /* Free resources */
        this->credssp_buffer_free();


        return Res::Ok;
    }

public:
    bool credssp_client_authenticate_init()
    {
        this->client_auth_data.state = ClientAuthenticateData::Start;
        if (Res::Err == this->sm_credssp_client_authenticate_start()) {
            return false;
        }

        this->client_auth_data.state = ClientAuthenticateData::Loop;
        return Res::Err != this->sm_credssp_client_authenticate_send();
    }

    enum class State { Err, Cont, Finish, };

    State credssp_client_authenticate_next(InStream & in_stream)
    {
        switch (this->client_auth_data.state)
        {
            case ClientAuthenticateData::Start:
                return State::Err;
            case ClientAuthenticateData::Loop:
                if (Res::Err == this->sm_credssp_client_authenticate_recv(in_stream)
                 || Res::Err == this->sm_credssp_client_authenticate_send()) {
                    return State::Err;
                }
                return State::Cont;
            case ClientAuthenticateData::Final:
                if (Res::Err == this->sm_credssp_client_authenticate_stop(in_stream)) {
                    return State::Err;
                }
                this->client_auth_data.state = ClientAuthenticateData::Start;
                return State::Finish;
        }

        return State::Err;
    }
};

class rdpCredsspServer : public rdpCredsspBase
{
public:
    rdpCredsspServer(Transport & transport,
               const bool krb,
               const bool restricted_admin_mode,
               Random & rand,
               TimeObj & timeobj,
               std::string& extra_message,
               Translation::language_t lang,
               std::function<Ntlm_SecurityFunctionTable::PasswordCallback(SEC_WINNT_AUTH_IDENTITY&)> set_password_cb,
               const bool verbose = false)
        : rdpCredsspBase(
            nullptr, nullptr, nullptr, nullptr, "", krb,
            restricted_admin_mode, rand, timeobj, extra_message, lang,
            transport, "rdpCredsspServer", std::move(set_password_cb), verbose)
    {
    }

    using rdpCredsspBase::set_credentials;

    SEC_STATUS credssp_decrypt_ts_credentials() {
        int length;
        SEC_STATUS status;
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredsspServer::decrypt_ts_credentials");
        }

        SecBuffer Buffers[2]; /* Signature, TSCredentials */

        if (this->authInfo.size() < 1) {
            LOG(LOG_ERR, "credssp_decrypt_ts_credentials missing authInfo buffer");
            return SEC_E_INVALID_TOKEN;
        }

        length = this->authInfo.size();

        Buffers[0].init(this->ContextSizes.cbMaxSignature);
        Buffers[0].copy(this->authInfo.get_data(), Buffers[0].size());

        Buffers[1].init(length - this->ContextSizes.cbMaxSignature);
        Buffers[1].copy(this->authInfo.get_data() + this->ContextSizes.cbMaxSignature, Buffers[1].size());

        status = this->table->DecryptMessage(Buffers[1], Buffers[0], this->recv_seq_num++);

        if (status != SEC_E_OK) {
            return status;
        }

        InStream decrypted_creds(Buffers[1].get_data(), Buffers[1].size());
        this->ts_credentials.recv(decrypted_creds);

        // hexdump(this->ts_credentials.passCreds.userName,
        //         this->ts_credentials.passCreds.userName_length);
        // hexdump(this->ts_credentials.passCreds.domainName,
        //         this->ts_credentials.passCreds.domainName_length);
        // hexdump(this->ts_credentials.passCreds.password,
        //         this->ts_credentials.passCreds.password_length);

        return SEC_E_OK;
    }

private:
    struct ServerAuthenticateData
    {
        enum : uint8_t { Start, Loop, Final } state = Start;
        unsigned long cbMaxToken;
    };
    ServerAuthenticateData server_auth_data;
    enum class Res : bool { Err, Ok };

    Res sm_credssp_server_authenticate_start()
    {
        if (this->verbose) {
            LOG(LOG_INFO, "rdpCredsspServer::server_authenticate");
        }
        // TODO
        // sspi_GlobalInit();

        if (this->credssp_ntlm_init(true) == 0) {
            return Res::Err;
        }

        this->InitSecurityInterface(NTLM_Interface);

        SecPkgInfo const packageInfo = this->table->QuerySecurityPackageInfo();

        SEC_STATUS status = this->table->AcquireCredentialsHandle(
            nullptr, SECPKG_CRED_INBOUND, nullptr, nullptr);

        if (status != SEC_E_OK) {
            LOG(LOG_ERR, "AcquireCredentialsHandle status: 0x%08X", status);
            return Res::Err;
        }

        this->server_auth_data.cbMaxToken = packageInfo.cbMaxToken;
        this->ContextSizes = {};

        /*
        * from tspkg.dll: 0x00000112
        * ASC_REQ_MUTUAL_AUTH
        * ASC_REQ_CONFIDENTIALITY
        * ASC_REQ_ALLOCATE_MEMORY
        */

        return Res::Ok;
    }

private:
    SEC_STATUS state_accept_security_context = SEC_I_INCOMPLETE_CREDENTIALS;

public:
    Res sm_credssp_server_authenticate_recv(InStream & in_stream)
    {
        if (this->state_accept_security_context != SEC_I_LOCAL_LOGON) {
            /* receive authentication token */
            this->ts_request.recv(in_stream);
        }

        SecBuffer input_buffer;
        SecBuffer output_buffer;

        input_buffer.init(0);
        output_buffer.init(0);

        input_buffer.copy(this->negoToken);

        if (this->negoToken.size() < 1) {
            LOG(LOG_ERR, "CredSSP: invalid negoToken!");
            return Res::Err;
        }

        output_buffer.init(this->server_auth_data.cbMaxToken);

        unsigned long const fContextReq = 0
            | ASC_REQ_MUTUAL_AUTH
            | ASC_REQ_CONFIDENTIALITY
            | ASC_REQ_CONNECTION
            | ASC_REQ_USE_SESSION_KEY
            | ASC_REQ_REPLAY_DETECT
            | ASC_REQ_SEQUENCE_DETECT
            | ASC_REQ_EXTENDED_ERROR;
        SEC_STATUS status = this->table->AcceptSecurityContext(
            input_buffer, fContextReq, output_buffer);
        this->state_accept_security_context = status;
        if (status == SEC_I_LOCAL_LOGON) {
            return Res::Ok;
        }

        this->negoToken.copy(output_buffer);

        if ((status == SEC_I_COMPLETE_AND_CONTINUE) || (status == SEC_I_COMPLETE_NEEDED)) {
            this->table->CompleteAuthToken(output_buffer);

            if (status == SEC_I_COMPLETE_NEEDED) {
                status = SEC_E_OK;
            }
            else if (status == SEC_I_COMPLETE_AND_CONTINUE) {
                status = SEC_I_CONTINUE_NEEDED;
            }
        }

        if (status == SEC_E_OK) {
            this->ContextSizes = this->table->QueryContextSizes();

            if (this->credssp_decrypt_public_key_echo() != SEC_E_OK) {
                LOG(LOG_ERR, "Error: could not verify client's public key echo");
                return Res::Err;
            }

            this->negoToken.init(0);

            this->credssp_encrypt_public_key_echo();
        }

        if ((status != SEC_E_OK) && (status != SEC_I_CONTINUE_NEEDED)) {
            LOG(LOG_ERR, "AcceptSecurityContext status: 0x%08X", status);
            return Res::Err;
        }

        this->credssp_send();
        this->credssp_buffer_free();

        if (status != SEC_I_CONTINUE_NEEDED) {
            if (status != SEC_E_OK) {
                LOG(LOG_ERR, "AcceptSecurityContext status: 0x%08X", status);
                return Res::Err;
            }
            this->server_auth_data.state = ServerAuthenticateData::Final;
        }

        return Res::Ok;
    }

    Res sm_credssp_server_authenticate_final(InStream & in_stream)
    {
        /* Receive encrypted credentials */
        this->ts_request.recv(in_stream);

        SEC_STATUS status = this->credssp_decrypt_ts_credentials();

        if (status != SEC_E_OK) {
            LOG(LOG_ERR, "Could not decrypt TSCredentials status: 0x%08X", status);
            return Res::Err;
        }

        status = this->table->ImpersonateSecurityContext();

        if (status != SEC_E_OK) {
            LOG(LOG_ERR, "ImpersonateSecurityContext status: 0x%08X", status);
            return Res::Err;
        }

        status = this->table->RevertSecurityContext();

        if (status != SEC_E_OK) {
            LOG(LOG_ERR, "RevertSecurityContext status: 0x%08X", status);
            return Res::Err;
        }

        return Res::Ok;
    }

public:
    bool credssp_server_authenticate_init()
    {
        this->server_auth_data.state = ServerAuthenticateData::Start;
        if (Res::Err == this->sm_credssp_server_authenticate_start()) {
            return false;
        }
        this->server_auth_data.state = ServerAuthenticateData::Loop;
        return true;
    }

    enum class State { Err, Cont, Finish, };

    State credssp_server_authenticate_next(InStream & in_stream)
    {
        switch (this->server_auth_data.state)
        {
            case ServerAuthenticateData::Start:
                return State::Err;
            case ServerAuthenticateData::Loop:
                if (Res::Err == this->sm_credssp_server_authenticate_recv(in_stream)) {
                    return State::Err;
                }
                return State::Cont;
            case ServerAuthenticateData::Final:
                if (Res::Err == this->sm_credssp_server_authenticate_final(in_stream)) {
                    return State::Err;
                }
                this->server_auth_data.state = ServerAuthenticateData::Start;
                return State::Finish;
        }

        return State::Err;
    }
};
