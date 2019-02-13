import os
import sys

from cffi import FFI

from . import evt_exception


class LibEVT:
    ffi = FFI()
    lib = None
    abi = None


def check_lib_init():
    if LibEVT.lib is None:
        evt_exception.evt_exception_raiser(
            evt_exception.EVTErrCode.EVT_NOT_INIT)
    return LibEVT


def init_evt_lib():
    assert LibEVT.abi is None

    LibEVT.ffi.cdef("""
            typedef struct {
                size_t  sz;
                char    buf[0];
            } evt_data_t;

            #define EVT_OK                       0
            #define EVT_INTERNAL_ERROR          -1
            #define EVT_INVALID_ARGUMENT        -2
            #define EVT_INVALID_PRIVATE_KEY     -3
            #define EVT_INVALID_PUBLIC_KEY      -4
            #define EVT_INVALID_SIGNATURE       -5
            #define EVT_INVALID_HASH            -6
            #define EVT_INVALID_ACTION          -7
            #define EVT_INVALID_BINARY          -8
            #define EVT_INVALID_JSON            -9
            #define EVT_INVALID_ADDRESS         -10
            #define EVT_SIZE_NOT_EQUALS         -11
            #define EVT_DATA_NOT_EQUALS         -12
            #define EVT_INVALID_LINK            -13

            int evt_free(void*);
            int evt_equals(evt_data_t* rhs, evt_data_t* lhs);
            int evt_version();
            int evt_last_error();


            typedef evt_data_t evt_bin_t;
            typedef evt_data_t evt_chain_id_t;
            typedef evt_data_t evt_block_id_t;
            typedef evt_data_t evt_public_key_t;
            typedef evt_data_t evt_private_key_t;
            typedef evt_data_t evt_signature_t;
            typedef evt_data_t evt_checksum_t;
            typedef evt_data_t evt_address_t;
            typedef void       evt_link_t;


            void* evt_abi();
            void evt_free_abi(void* abi);
            int evt_abi_json_to_bin(void* evt_abi, const char* action, const char* json, evt_bin_t** bin /* out */);
            int evt_abi_bin_to_json(void* evt_abi, const char* action, evt_bin_t* bin, char** json /* out */);
            int evt_trx_json_to_digest(void* evt_abi, const char* json, evt_chain_id_t* chain_id, evt_checksum_t** digest /* out */);
            int evt_chain_id_from_string(const char* str, evt_chain_id_t** chain_id /* out */);


            int evt_generate_new_pair(evt_public_key_t** pub_key /* out */, evt_private_key_t** priv_key /* out */);
            int evt_get_public_key(evt_private_key_t* priv_key, evt_public_key_t** pub_key /* out */);
            int evt_sign_hash(evt_private_key_t* priv_key, evt_checksum_t* hash, evt_signature_t** sign /* out */);
            int evt_recover(evt_signature_t* sign, evt_checksum_t* hash, evt_public_key_t** pub_key /* out */);
            int evt_hash(const char* buf, size_t sz, evt_checksum_t** hash /* out */);

            int evt_public_key_string(evt_public_key_t* pub_key, char** str /* out */);
            int evt_private_key_string(evt_private_key_t* priv_key, char** str /* out */);
            int evt_signature_string(evt_signature_t* sign, char** str /* out */);
            int evt_checksum_string(evt_checksum_t* hash, char** str /* out */);

            int evt_public_key_from_string(const char* str, evt_public_key_t** pub_key /* out */);
            int evt_private_key_from_string(const char* str, evt_private_key_t** priv_key /* out */);
            int evt_signature_from_string(const char* str, evt_signature_t** sign /* out */);
            int evt_checksum_from_string(const char* str, evt_checksum_t** hash /* out */);

            int evt_block_id_from_string(const char* str, evt_block_id_t** block_id /* out */);
            int evt_ref_block_num(evt_block_id_t* block_id, uint16_t* ref_block_num);
            int evt_ref_block_prefix(evt_block_id_t* block_id, uint32_t* ref_block_prefix);
            
            int evt_address_from_string(const char* str, evt_address_t** addr /* out */);
            int evt_address_to_string(evt_address_t* addr, char** str /* out */);
            int evt_address_public_key(evt_public_key_t *pub_key, evt_address_t** addr/* out */);
            int evt_address_reserved(evt_address_t** addr/* out */);
            int evt_address_generated(const char* prefix, const char* key, uint32_t nonce, evt_address_t** addr/* out */);
            int evt_address_get_public_key(evt_address_t* addr, evt_public_key_t **pub_key/* out */);
            int evt_address_get_prefix(evt_address_t* addr, char** str/* out */);
            int evt_address_get_key(evt_address_t* addr, char** str/* out */);
            int evt_address_get_nonce(evt_address_t* addr, uint32_t* nonce/* out */);
            int evt_address_get_type(evt_address_t* addr, char** str/* out */);

            evt_link_t* evt_link_new();
            void evt_link_free(evt_link_t*);
            int evt_link_tostring(evt_link_t*, char**);
            int evt_link_parse_from_evtli(const char*, evt_link_t*);
            int evt_link_get_header(evt_link_t*, uint16_t*);
            int evt_link_set_header(evt_link_t*, uint16_t);
            int evt_link_get_segment_int(evt_link_t*, uint8_t, uint32_t*);
            int evt_link_get_segment_str(evt_link_t*, uint8_t, char**);
            int evt_link_add_segment_int(evt_link_t*, uint8_t, uint32_t);
            int evt_link_add_segment_str(evt_link_t*, uint8_t, const char*);
            int evt_link_get_signatures(evt_link_t*, evt_signature_t***, uint32_t*);
            int evt_link_sign(evt_link_t*, evt_private_key_t*);
            """)

    if sys.platform == 'linux':
        ext = '.so'
    elif sys.platform == 'darwin':
        ext = '.dylib'
    else:
        raise Exception('Not supported platform: {}'.format(sys.platform))

    if 'LIBEVT_PATH' in os.environ:
        LibEVT.lib = LibEVT.ffi.dlopen(
            os.environ['LIBEVT_PATH'] + '/libevt' + ext)
    else:
        LibEVT.lib = LibEVT.ffi.dlopen('libevt' + ext)

    LibEVT.abi = LibEVT.lib.evt_abi()


def init_lib():
    LibEVT.ffi.init_once(init_evt_lib, 'init_evt')
    return LibEVT.lib.evt_version()
