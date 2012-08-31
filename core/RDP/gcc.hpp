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

   Generic Conference Control (T.124)

   T.124 GCC is defined in:

   http://www.itu.int/rec/T-REC-T.124-199802-S/en
   ITU-T T.124 (02/98): Generic Conference Control

*/

#if !defined(__CORE_RDP_GCC_HPP__)
#define __CORE_RDP_GCC_HPP__

#include "stream.hpp"

enum DATA_BLOCK_TYPE {
    //  The data block that follows contains Client Core Data (section 2.2.1.3.2).
    CS_CORE = 0xC001,
    // The data block that follows contains Client Security Data (section 2.2.1.3.3).
    CS_SECURITY = 0xC002,
    // The data block that follows contains Client Network Data (section 2.2.1.3.4).
    CS_NET = 0xC003,
    // The data block that follows contains Client Cluster Data (section 2.2.1.3.5).
    CS_CLUSTER = 0xC004,
    // The data block that follows contains Client Monitor Data (section 2.2.1.3.6).
    CS_MONITOR = 0xC005,
    // The data block that follows contains Server Core Data (section 2.2.1.4.2).
    SC_CORE = 0x0C01,
    // The data block that follows contains Server Security Data (section 2.2.1.4.3).
    SC_SECURITY = 0x0C02,
    // The data block that follows contains Server Network Data (section 2.2.1.4.4).
    SC_NET = 0x0C03
};

#include "gcc_conference_user_data/cs_core.hpp"
#include "gcc_conference_user_data/cs_cluster.hpp"
#include "gcc_conference_user_data/cs_monitor.hpp"
#include "gcc_conference_user_data/cs_net.hpp"
#include "gcc_conference_user_data/cs_sec.hpp"
//#include "gcc_conference_user_data/sc_core.hpp"
#include "gcc_conference_user_data/sc_net.hpp"
#include "gcc_conference_user_data/sc_sec1.hpp"

namespace GCC
{
// ConferenceName ::= SEQUENCE
// {
//    numeric     SimpleNumericString,
//    text        SimpleTextString OPTIONAL,
//    ...,
//    unicodeText TextString OPTIONAL
// }

// ConferenceCreateRequest ::= SEQUENCE
// { -- MCS-Connect-Provider request user data
//    conferenceName         ConferenceName,
//    convenerPassword       Password OPTIONAL,
//    password               Password OPTIONAL,
//    lockedConference       BOOLEAN,
//    listedConference       BOOLEAN,
//    conductibleConference  BOOLEAN,
//    terminationMethod      TerminationMethod,
//    conductorPrivileges    SET OF Privilege OPTIONAL,
//    conductedPrivileges    SET OF Privilege OPTIONAL,
//    nonConductedPrivileges SET OF Privilege OPTIONAL,
//    conferenceDescription  TextString OPTIONAL,
//    callerIdentifier       TextString OPTIONAL,
//    userData               UserData OPTIONAL,
//    ...,
//    conferencePriority     ConferencePriority OPTIONAL,
//    conferenceMode         ConferenceMode OPTIONAL
// }

//    PER encoded (ALIGNED variant of BASIC-PER) GCC Connection Data (ConnectData):
//    00 05 00 14 7c 00 01 81 2a 00 08 00 10 00 01 c0
//    00 44 75 63 61 81 1c

//    0 - CHOICE: From Key select object (0) of type OBJECT IDENTIFIER
//    0 - padding
//    0 - padding
//    0 - padding
//    0 - padding
//    0 - padding
//    0 - padding
//    0 - padding

//    05 -> object length = 5 bytes

//    00 14 7c 00 01 -> object
//    The first byte gives the first two values in the sextuple (m and n), as it is encoded as
//    40m + n. Hence, decoding the remaining data yields the correct results:

//    OID = { 0 0 20 124 0 1 } = {itu-t(0) recommendation(0) t(20) t124(124) version(0) 1}
//    Description = v.1 of ITU-T Recommendation T.124 (Feb 1998): "Generic Conference Control"

//    81 2a -> ConnectData::connectPDU length = 298 bytes
//    Since the most significant bit of the first byte (0x81) is set to 1 and the following bit is
//    set to 0, the length is given by the low six bits of the first byte and the second byte.
//    Hence, the value is 0x12a, which is 298 bytes.

//    PER encoded (ALIGNED variant of BASIC-PER) GCC Conference Create Request PDU:
//    00 08 00 10 00 01 c0 00 44 75 63 61 81 1c

//    0x00:
//    0 - extension bit (ConnectGCCPDU)
//    0 - --\.
//    0 -   | CHOICE: From ConnectGCCPDU select conferenceCreateRequest (0)
//    0 - --/ of type ConferenceCreateRequest
//    0 - extension bit (ConferenceCreateRequest)
//    0 - ConferenceCreateRequest::convenerPassword present
//    0 - ConferenceCreateRequest::password present
//    0 - ConferenceCreateRequest::conductorPrivileges present


//    0x08:
//    0 - ConferenceCreateRequest::conductedPrivileges present
//    0 - ConferenceCreateRequest::nonConductedPrivileges present
//    0 - ConferenceCreateRequest::conferenceDescription present
//    0 - ConferenceCreateRequest::callerIdentifier present
//    1 - ConferenceCreateRequest::userData present
//    0 - extension bit (ConferenceName)
//    0 - ConferenceName::text present
//    0 - padding

//    0x00
//    0 - --\.
//    0 -   |
//    0 -   |
//    0 -   | ConferenceName::numeric length = 0 + 1 = 1 character
//    0 -   | (minimum for SimpleNumericString is 1)
//    0 -   |
//    0 -   |
//    0 - --/

//    0x10:
//    0 - --\.
//    0 -   | ConferenceName::numeric = "1"
//    0 -   |
//    1 - --/
//    0 - ConferenceCreateRequest::lockedConference
//    0 - ConferenceCreateRequest::listedConference
//    0 - ConferenceCreateRequest::conducibleConference
//    0 - extension bit (TerminationMethod)

//    0x00:
//    0 - TerminationMethod::automatic
//    0 - padding
//    0 - padding
//    0 - padding
//    0 - padding
//    0 - padding
//    0 - padding
//    0 - padding

//    0x01:
//    0 - --\.
//    0 -   |
//    0 -   |
//    0 -   | number of UserData sets = 1
//    0 -   |
//    0 -   |
//    0 -   |
//    1 - --/

//    0xc0:
//    1 - UserData::value present
//    1 - CHOICE: From Key select h221NonStandard (1) of type H221NonStandardIdentifier
//    0 - padding
//    0 - padding
//    0 - padding
//    0 - padding
//    0 - padding
//    0 - padding

//    0x00:
//    0 - --\.
//    0 -   |
//    0 -   |
//    0 -   | h221NonStandard length = 0 + 4 = 4 octets
//    0 -   | (minimum for H221NonStandardIdentifier is 4)
//    0 -   |
//    0 -   |
//    0 - --/

//    44 75 63 61 -> h221NonStandard (client-to-server H.221 key) = "Duca"
//    81 1c -> UserData::value length = 284 bytes
//    Since the most significant bit of the first byte (0x81) is set to 1 and the following bit is
//    set to 0, the length is given by the low six bits of the first byte and the second byte.
//    Hence, the value is 0x11c, which is 284 bytes.


    class Create_Request_Send {
        public:
        Create_Request_Send(Stream & stream, size_t payload_size) {
            // ConnectData
            stream.out_per_choice(0); // From Key select object (0) of type OBJECT_IDENTIFIER
            const uint8_t t124_02_98_oid[6] = { 0, 0, 20, 124, 0, 1 };
            stream.out_per_object_identifier(t124_02_98_oid); // ITU-T T.124 (02/98) OBJECT_IDENTIFIER

            //  ConnectData::connectPDU (OCTET_STRING)
            uint16_t offset_len = stream.get_offset();
            stream.out_per_length(256); // connectPDU length (reserve 16 bits)

            //  ConnectGCCPDU
            stream.out_per_choice(0); // From ConnectGCCPDU select conferenceCreateRequest (0) of type ConferenceCreateRequest
            stream.out_per_selection(0x08); // select optional userData from ConferenceCreateRequest

            //  ConferenceCreateRequest::conferenceName
            //	stream.out_per_numeric_string(s, (uint8*)"1", 1, 1); /* ConferenceName::numeric */
            stream.out_uint16_be(16);
            stream.out_per_padding(1); /* padding */

            //  UserData (SET OF SEQUENCE)
            stream.out_per_number_of_sets(1); // one set of UserData
            stream.out_per_choice(0xC0); // UserData::value present + select h221NonStandard (1)

            //  h221NonStandard
            const uint8_t h221_cs_key[4] = {'D', 'u', 'c', 'a'};
            stream.out_per_octet_string(h221_cs_key, 4, 4); // h221NonStandard, client-to-server H.221 key, "Duca"

            stream.out_per_length(payload_size); // user data length
            stream.mark_end();

            stream.set_out_per_length(payload_size + stream.get_offset() - 9, offset_len); // length including header
        }
    };

    class Create_Request_Recv {
        public:
        size_t payload_size;

        SubStream payload;

        Create_Request_Recv(Stream & stream) {
            // Key select object (0) of type OBJECT_IDENTIFIER
            // ITU-T T.124 (02/98) OBJECT_IDENTIFIER
            stream.in_skip_bytes(7);
            uint16_t length_with_header = stream.in_per_length();

            // ConnectGCCPDU
            // From ConnectGCCPDU select conferenceCreateRequest (0) of type ConferenceCreateRequest
            // select optional userData from ConferenceCreateRequest
            // ConferenceCreateRequest::conferenceName
            // UserData (SET OF SEQUENCE), one set of UserData
            // UserData::value present + select h221NonStandard (1)
            // h221NonStandard, client-to-server H.221 key, "Duca"

            stream.in_skip_bytes(12);
            uint16_t length = stream.in_per_length();

            if (length_with_header != length + 14){
                LOG(LOG_WARNING, "GCC Conference Create Request User data Length mismatch with header+data length %u %u", length, length_with_header);
                throw Error(ERR_GCC);
            }

            if (length != stream.size() - stream.get_offset()){
                LOG(LOG_WARNING, "GCC Conference Create Request User data Length mismatch with header %u %u", length, stream.size() - stream.get_offset());
                throw Error(ERR_GCC);
            }
            this->payload.resize(stream, stream.size() - stream.get_offset());
        }
    };

    // GCC Conference Create Response
    // ------------------------------

    // ConferenceCreateResponse Parameters
    // -----------------------------------

    // Generic definitions used in parameter descriptions:

    // simpleTextFirstCharacter UniversalString ::= {0, 0, 0, 0}

    // simpleTextLastCharacter UniversalString ::= {0, 0, 0, 255}

    // SimpleTextString ::=  BMPString (SIZE (0..255)) (FROM (simpleTextFirstCharacter..simpleTextLastCharacter))

    // TextString ::= BMPString (SIZE (0..255)) -- Basic Multilingual Plane of ISO/IEC 10646-1 (Unicode)

    // SimpleNumericString ::= NumericString (SIZE (1..255)) (FROM ("0123456789"))

    // DynamicChannelID ::= INTEGER (1001..65535) -- Those created and deleted by MCS

    // UserID ::= DynamicChannelID

    // H221NonStandardIdentifier ::= OCTET STRING (SIZE (4..255))
    //      -- First four octets shall be country code and
    //      -- Manufacturer code, assigned as specified in
    //      -- Annex A/H.221 for NS-cap and NS-comm

    // Key ::= CHOICE   -- Identifier of a standard or non-standard object
    // {
    //      object              OBJECT IDENTIFIER,
    //      h221NonStandard     H221NonStandardIdentifier
    // }

    // UserData ::= SET OF SEQUENCE
    // {
    //      key     Key,
    //      value   OCTET STRING OPTIONAL
    // }

    // ConferenceCreateResponse ::= SEQUENCE
    // {    -- MCS-Connect-Provider response user data
    //      nodeID              UserID, -- Node ID of the sending node
    //      tag                 INTEGER,
    //      result              ENUMERATED
    //      {
    //          success                         (0),
    //          userRejected                    (1),
    //          resourcesNotAvailable           (2),
    //          rejectedForSymmetryBreaking     (3),
    //          lockedConferenceNotSupported    (4),
    //          ...
    //      },
    //      userData            UserData OPTIONAL,
    //      ...
    //}


    // User Data                 : Optional
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // User Data: Optional user data which may be used for functions outside
    // the scope of this Recommendation such as authentication, billing,
    // etc.

    // Result                    : Mandatory
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // An indication of whether the request was accepted or rejected, and if
    // rejected, the reason why. It contains one of a list of possible
    // results: successful, user rejected, resources not available, rejected
    // for symmetry-breaking, locked conference not supported, Conference
    // Name and Conference Name Modifier already exist, domain parameters
    // unacceptable, domain not hierarchical, lower-layer initiated
    // disconnect, unspecified failure to connect. A negative result in the
    // GCC-Conference-Create confirm does not imply that the physical
    // connection to the node to which the connection was being attempted
    // is disconnected.

    // The ConferenceCreateResponse PDU is shown in Table 8-4. The Node ID
    // parameter, which is the User ID assigned by MCS in response to the
    // MCS-Attach-User request issued by the GCC Provider, shall be supplied
    // by the GCC Provider sourcing this PDU. The Tag parameter is assigned
    // by the source GCC Provider to be locally unique. It is used to
    // identify the returned UserIDIndication PDU. The Result parameter
    // includes GCC-specific failure information sourced directly from
    // the Result parameter in the GCC-Conference-Create response primitive.
    // If the Result parameter is anything except successful, the Result
    // parameter in the MCS-Connect-Provider response is set to
    // user-rejected.

    //            Table 8-4 – ConferenceCreateResponse GCCPDU
    // +------------------+------------------+--------------------------+
    // | Content          |     Source       |         Sink             |
    // +==================+==================+==========================+
    // | Node ID          | Top GCC Provider | Destination GCC Provider |
    // +------------------+------------------+--------------------------+
    // | Tag              | Top GCC Provider | Destination GCC Provider |
    // +------------------+------------------+--------------------------+
    // | Result           | Response         | Confirm                  |
    // +------------------+------------------+--------------------------+
    // | User Data (opt.) | Response         | Confirm                  |
    // +------------------+------------------+--------------------------+

    //PER encoded (ALIGNED variant of BASIC-PER) GCC Connection Data (ConnectData):
    // 00 05 00
    // 14 7c 00 01
    // 2a
    // 14 76 0a 01 01 00 01 c0 00 4d 63 44 6e
    // 81 08

    class Create_Response_Send {
        public:
        Create_Response_Send(Stream & stream, size_t payload_size) {
            // ConnectData
            // 00 05 -> Key::object length = 5 bytes
            // 00 14 7c 00 01 -> Key::object = { 0 0 20 124 0 1 }
            stream.out_uint16_be(5);
            stream.out_copy_bytes("\x00\x14\x7c\x00\x01", 5);

            // 2a -> ConnectData::connectPDU length = 42 bytes
            // This length MUST be ignored by the client.
            stream.out_uint8(0x2a);

            // PER encoded (ALIGNED variant of BASIC-PER) GCC Conference Create Response
            // PDU:
            // 14 76 0a 01 01 00 01 c0 00 00 4d 63 44 6e 81 08

            // 0x14:
            // 0 - extension bit (ConnectGCCPDU)
            // 0 - --\ ...
            // 0 -   | CHOICE: From ConnectGCCPDU select conferenceCreateResponse (1)
            // 1 - --/ of type ConferenceCreateResponse
            // 0 - extension bit (ConferenceCreateResponse)
            // 1 - ConferenceCreateResponse::userData present
            // 0 - padding
            // 0 - padding
            stream.out_uint8(0x10 | 4);

            // ConferenceCreateResponse::nodeID
            //  = 0x760a + 1001 = 30218 + 1001 = 31219
            //  (minimum for UserID is 1001)
            stream.out_uint16_le(0x760a);

            // ConferenceCreateResponse::tag length = 1 byte
            stream.out_uint8(1);

            // ConferenceCreateResponse::tag = 1
            stream.out_uint8(1);

            // 0x00:
            // 0 - extension bit (Result)
            // 0 - --\ ...
            // 0 -   | ConferenceCreateResponse::result = success (0)
            // 0 - --/
            // 0 - padding
            // 0 - padding
            // 0 - padding
            // 0 - padding
            stream.out_uint8(0);

            // number of UserData sets = 1
            stream.out_uint8(1);

            // 0xc0:
            // 1 - UserData::value present
            // 1 - CHOICE: From Key select h221NonStandard (1)
            //               of type H221NonStandardIdentifier
            // 0 - padding
            // 0 - padding
            // 0 - padding
            // 0 - padding
            // 0 - padding
            // 0 - padding
            stream.out_uint8(0xc0);

            // h221NonStandard length = 0 + 4 = 4 octets
            //   (minimum for H221NonStandardIdentifier is 4)
            stream.out_uint8(0);

            // h221NonStandard (server-to-client H.221 key) = "McDn"
            stream.out_copy_bytes("McDn", 4);

            // set user_data_len (TWO_BYTE_UNSIGNED_ENCODING)
            stream.out_uint16_be(0x8000 | payload_size);
            stream.mark_end();
        }
    };

    class Create_Response_Recv {
        public:
        size_t payload_size;

        SubStream payload;

        Create_Response_Recv(Stream & stream) {
            stream.in_skip_bytes(21); /* header (T.124 ConferenceCreateResponse) */
            size_t length = stream.in_per_length();
            if (length != stream.size() - stream.get_offset()){
                LOG(LOG_WARNING, "GCC Conference Create Response User data Length mismatch with header %u %u",
                    length, stream.size() - stream.get_offset());
                throw Error(ERR_GCC);
            }
            
            this->payload.resize(stream, stream.size() - stream.get_offset());
        }
    };


    // 2.2.1.3.1 User Data Header (TS_UD_HEADER)
    // =========================================

    // type (2 bytes): A 16-bit, unsigned integer. The type of the data
    //                 block that this header precedes.

    // +-------------------+-------------------------------------------------------+
    // | CS_CORE 0xC001    | The data block that follows contains Client Core      |
    // |                   | Data (section 2.2.1.3.2).                             |
    // +-------------------+-------------------------------------------------------+
    // | CS_SECURITY 0xC002| The data block that follows contains Client           |
    // |                   | Security Data (section 2.2.1.3.3).                    |
    // +-------------------+-------------------------------------------------------+
    // | CS_NET 0xC003     | The data block that follows contains Client Network   |
    // |                   | Data (section 2.2.1.3.4).                             |
    // +-------------------+-------------------------------------------------------+
    // | CS_CLUSTER 0xC004 | The data block that follows contains Client Cluster   |
    // |                   | Data (section 2.2.1.3.5).                             |
    // +-------------------+-------------------------------------------------------+
    // | CS_MONITOR 0xC005 | The data block that follows contains Client           |
    // |                   | Monitor Data (section 2.2.1.3.6).                     |
    // +-------------------+-------------------------------------------------------+
    // | SC_CORE 0x0C01    | The data block that follows contains Server Core      |
    // |                   | Data (section 2.2.1.4.2)                              |
    // +-------------------+-------------------------------------------------------+
    // | SC_SECURITY 0x0C02| The data block that follows contains Server           |
    // |                   | Security Data (section 2.2.1.4.3).                    |
    // +-------------------+-------------------------------------------------------+
    // | SC_NET 0x0C03     | The data block that follows contains Server Network   |
    // |                   | Data (section 2.2.1.4.4)                              |
    // +-------------------+-------------------------------------------------------+

    // length (2 bytes): A 16-bit, unsigned integer. The size in bytes of the data
    //   block, including this header.

    namespace UserData
    {
        struct RecvFactory
        {
            uint16_t tag;
            uint16_t length;
            SubStream payload;

            RecvFactory(Stream & stream) : payload(stream, stream.get_offset())
            {
                if (!stream.check_rem(4)){
                    LOG(LOG_WARNING, "Incomplete GCC::UserData data block header");                      
                    throw Error(ERR_GCC);
                }
                this->tag = stream.in_uint16_le();
                this->length = stream.in_uint16_le();
                LOG(LOG_INFO, "GCC::UserData tag=%0.4x length=%u", tag, length);
                if (!stream.check_rem(length - 4)){
                    LOG(LOG_WARNING, "Incomplete GCC::UserData data block"
                                     " tag=%u length=%u available_length=%u",
                                     tag, length, stream.size() - 4);                      
                    throw Error(ERR_GCC);
                }
                stream.in_skip_bytes(length - 4);
                this->payload.resize(this->payload, length);
            }
        };


        // 2.2.1.4.2 Server Core Data (TS_UD_SC_CORE)
        // ==========================================

        // The TS_UD_SC_CORE data block contains core server connection-related
        // information.

        // header (4 bytes): GCC user data block header, as specified in User Data
        //  Header (section 2.2.1.3.1). The User Data Header type field MUST be set to
        //  SC_CORE (0x0C01).

        // version (4 bytes): A 32-bit, unsigned integer. The server version number for
        //  the RDP. The major version number is stored in the high two bytes, while the
        //  minor version number is stored in the low two bytes.

        // 0x00080001 RDP 4.0 servers
        // 0x00080004 RDP 5.0, 5.1, 5.2, 6.0, 6.1, and 7.0 servers

        // If the server advertises a version number greater than or equal to 0x00080004,
        // it MUST support a maximum length of 512 bytes for the UserName field in the
        // Info Packet (section 2.2.1.11.1.1).

        // clientRequestedProtocols (4 bytes): A 32-bit, unsigned integer that contains
        //  the flags sent by the client in the requestedProtocols field of the RDP
        //  Negotiation Request (section 2.2.1.1.1). In the event that an RDP
        //  Negotiation Request was not received from the client, this field MUST be
        //  initialized to PROTOCOL_RDP (0).

        // Exemple:
        //01 0c 0c 00 -> TS_UD_HEADER::type = SC_CORE (0x0c01), length = 12 bytes
        //04 00 08 00 -> TS_UD_SC_CORE::version = 0x0080004
        //00 00 00 00 -> TS_UD_SC_CORE::clientRequestedProtocols = PROTOCOL_RDP

        struct SCCore {
            uint16_t userDataType;
            uint16_t length;
            uint32_t version;
            bool option_clientRequestedProtocols;
            uint32_t clientRequestedProtocols;

            SCCore(Stream & stream)
            : userDataType(SC_CORE)
            , length(8)
            , version(0x00080001)
            , option_clientRequestedProtocols(false)
            , clientRequestedProtocols(0)
            {
                this->userDataType = stream.in_uint16_le();
                this->length = stream.in_uint16_le();
                this->version = stream.in_uint32_le();
                if (this->length < 12) { 
                    return;
                }
                this->option_clientRequestedProtocols = true;
                this->clientRequestedProtocols = stream.in_uint32_le();
            }

            SCCore(uint32_t version = 0x00080001, 
                   bool option_clientRequestedProtocols = false, 
                   uint32_t clientRequestedProtocols = 0)
            : userDataType(SC_CORE)
            , length(8 + 4 * option_clientRequestedProtocols)
            , version(version)
            , option_clientRequestedProtocols(option_clientRequestedProtocols)
            , clientRequestedProtocols(clientRequestedProtocols)
            {
            }

            void emit(Stream & stream)
            {
                stream.out_uint16_le(this->userDataType);
                stream.out_uint16_le(this->length);
                stream.out_uint32_le(this->version);
                if (this->option_clientRequestedProtocols){
                    stream.out_uint32_le(this->clientRequestedProtocols);
                }
                stream.mark_end();
            }

            void log(const char * msg)
            {
                // --------------------- Base Fields ---------------------------------------
                LOG(LOG_INFO, "%s GCC User Data SC_CORE (%u bytes)", msg, this->length);
                LOG(LOG_INFO, "sc_core::version [%04x] %s", this->version,
                      (this->version==0x00080001) ? "RDP 4 client"
                     :(this->version==0x00080004) ? "RDP 5.0, 5.1, 5.2, and 6.0 clients)"
                                                  : "Unknown client");
                if (this->length < 12) { return; }
                this->option_clientRequestedProtocols = true;
                LOG(LOG_INFO, "sc_core::clientRequestedProtocols  = %u", this->clientRequestedProtocols);
            }
        };
    };
};

#endif
