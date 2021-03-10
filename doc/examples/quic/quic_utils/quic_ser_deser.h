#ifndef __quic_ser_deser_h__
#define __quic_ser_deser_h__
    extern int scid_h;
    extern int dcid_h;
    #include <inttypes.h>


    typedef __int128_t int128_t;
    typedef __uint128_t uint128_t;


//https://stackoverflow.com/questions/25114597/how-to-print-int128-in-g
std::ostream&
operator<<( std::ostream& dest, __int128_t value )
{
    std::ostream::sentry s( dest );
    if ( s ) {
        __uint128_t tmp = value < 0 ? -value : value;
        char buffer[ 128 ];
        char* d = std::end( buffer );
        do
        {
            -- d;
            *d = "0123456789"[ tmp % 10 ];
            tmp /= 10;
        } while ( tmp != 0 );
        if ( value < 0 ) {
            -- d;
            *d = '-';
        }
        int len = std::end( buffer ) - d;
        if ( dest.rdbuf()->sputn( d, len ) != len ) {
            dest.setstate( std::ios_base::badbit );
        }
    }
    return dest;
}


    typedef struct tls_name_struct {
        const char *name;
        int128_t value;
        //long long value;
    } *tls_name_struct_ptr;
    struct tls_name_map : hash_space::hash_map<std::string,int128_t> {};
    //struct tls_name_map : hash_space::hash_map<std::string,long long> {};

    std::string quic_params[17] = {
        "quic_transport_parameters",
        "initial_max_stream_data_bidi_local",
        "initial_max_data",
        "initial_max_stream_id_bidi",
        "max_idle_timeout",
        "preferred_address",
        "max_packet_size",
        "stateless_reset_token",
        "ack_delay_exponent",
        "initial_max_stream_id_uni",
        "disable_migration",
        "active_connection_id_limit",
        "initial_max_stream_data_bidi_remote",
        "initial_max_stream_data_uni",
        "max_ack_delay",
        "initial_source_connection_id",
        "loss_bits"
    };

    struct tls_name_struct tls_field_length_bytes[33] = {
        {"fragment",2},
        {"content",2},
        {"tls.client_hello",3},
        {"tls.server_hello",3},
        {"tls.encrypted_extensions",3},
        {"unknown_message_bytes",3},
        {"session_id",1},
        {"cipher_suites",2},
        {"compression_methods",1},
        {"extensions",2},
        {"quic_transport_parameters",2},
        //{"transport_parameters",2}, // Parameter length field removal
        {"initial_max_stream_data_bidi_local",1},
        {"original_destination_connection_id",1},
        {"initial_max_data",1},
        {"initial_max_stream_id_bidi",1},
        {"max_idle_timeout",1},
        {"preferred_address",1},
        {"max_packet_size",1},
        {"stateless_reset_token",1},
        {"ack_delay_exponent",1},
        {"initial_max_stream_id_uni",1},
        {"disable_active_migration",1},
        {"active_connection_id_limit",1},
        {"initial_max_stream_data_bidi_remote",1},
        {"initial_max_stream_data_uni",1},
        {"max_ack_delay",1},
        {"initial_source_connection_id",1},
        {"retry_source_connection_id",1},
        {"loss_bits",2}, //for picoquic TODO test
        {"grease_quic_bit",1}, //for picoquic
        //{"enable_time_stamp",1}, //for picoquic TODO test
        {"min_ack_delay",1},
        {"unknown_transport_parameter",1},
        {0,0}
    };
    tls_name_map tls_field_length_bytes_map;
    struct tls_name_struct tls_field_bytes[31] = {
        {"version",2},
        {"client_version",2}, //0x0303 = 2 bytes
        {"server_version",2},
        {"etype",2},
        {"mtype",1},
        {"gmt_unix_time",4},
        {"cipher_suites",2},
        {"the_cipher_suite",2},
        {"compression_methods",1},
        {"the_compression_method",1},
        {"session_id",1},
        {"content",1},
        {"initial_version",4},
        {"stream_pos_32",-1},
        {"unknown",0},
        {"stream_id_16",-1},
        {"seconds_16",-1},
        {"stream_pos_16",-1},
        {"exponent_8",-1},
        {"data_8",1},
        {"dcid",8},
        {"scid",8},
        {"pcid",8},
        {"ip_addr",4},
        {"ip_port",2},
        {"ip6_addr1",8},
        {"ip6_addr2",8},
        {"ip6_port",2},
        {"pref_token",16},
        {"pcid_len",1},
        {0,0}
    };
    tls_name_map tls_field_bytes_map;
    //TODO check old version
    struct tls_name_struct tls_tags[31] = {
        {"tls.handshake_record",22},
        {"tls.application_data_record",23},
        {"tls.change_cipher_spec",20},
        {"tls.client_hello",1},
        {"tls.server_hello",2},
        {"tls.encrypted_extensions",0x08},
        {"tls.unknown_message",-1},
        {"tls.unknown_extension",-1},
        {"quic_transport_parameters",0xffa5},
        {"initial_max_stream_data_bidi_local",0x05},
        {"initial_max_data",0x04},
        {"initial_max_stream_id_bidi",0x08},
        {"original_destination_connection_id",0x00},
        {"max_idle_timeout",0x01},
        {"preferred_address",0x0d},
        {"max_packet_size",0x03},
        {"stateless_reset_token",0x02},
        {"ack_delay_exponent",0x0a},
        {"initial_max_stream_id_uni",0x09},
        {"disable_active_migration",0x0c},
        {"active_connection_id_limit",0x0e},
        {"initial_max_stream_data_bidi_remote",0x06},
        {"initial_max_stream_data_uni",0x07},
        {"max_ack_delay",0x0b},
        {"initial_source_connection_id",0x0f},
        {"retry_source_connection_id",0x1010}, //0x10 = 0100 0000 = 2 bytes with varint
        {"loss_bits",0x1057}, //for picoquic
        {"grease_quic_bit",0x2ab2}, //for picoquic
        //{"enable_time_stamp",0x7158}, //for picoquic
        {"min_ack_delay",-4611686014149009894}, //0xFF02DE1A ||  0xc0000000FF02DE1A (13835058059560541722)
        {"unknown_transport_parameter",-2},
       {0,0}
    };
    tls_name_map tls_tags_map;
    struct tls_name_struct tls_tag_bytes[24] = {
        {"tls.unknown_extension",2},
        {"quic_transport_parameters",2},
        {"initial_max_stream_data_bidi_local",1},
        {"initial_max_data",1},
        {"initial_max_stream_id_bidi",1},
        {"max_idle_timeout",1},
        {"preferred_address",1},
        {"max_packet_size",1},
        {"stateless_reset_token",1},
        {"ack_delay_exponent",1},
        {"initial_max_stream_id_uni",1},
        {"original_destination_connection_id",1},
        {"disable_migration",1},
        {"active_connection_id_limit",1},
        {"initial_max_stream_data_bidi_remote",1},
        {"initial_max_stream_data_uni",1},
        {"max_ack_delay",1},
        {"initial_source_connection_id",1},
        {"retry_source_connection_id",2},
        {"disable_active_migration",1},
        {"loss_bits",2}, //for picoquic
        {"grease_quic_bit",2}, //for picoquic
        {"min_ack_delay",8},
        //{"enable_time_stamp",2}, //for picoquic
        //{"unknown_transport_parameter",2},
        {0,0}
    };
    tls_name_map tls_tag_bytes_map;

    void tls_make_name_map(tls_name_struct *vals, tls_name_map &map) {
        while (vals->name) {
            map[vals->name] = vals->value;
            vals++;
        }
    }

#endif
