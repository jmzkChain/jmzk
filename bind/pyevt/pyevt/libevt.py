import os
import sys

from cffi import FFI

from . import jmzk_exception


class Libjmzk:
    ffi = FFI()
    lib = None
    abi = None


def check_lib_init():
    if Libjmzk.lib is None:
        jmzk_exception.jmzk_exception_raiser(
            jmzk_exception.jmzkErrCode.jmzk_NOT_INIT)
    return Libjmzk


def init_jmzk_lib():
    assert Libjmzk.abi is None

    Libjmzk.ffi.cdef("""
            typedef struct {
                size_t  sz;
                char    buf[0];
            } jmzk_data_t;

            #define jmzk_OK                       0
            #define jmzk_INTERNAL_ERROR          -1
            #define jmzk_INVALID_ARGUMENT        -2
            #define jmzk_INVALID_PRIVATE_KEY     -3
            #define jmzk_INVALID_PUBLIC_KEY      -4
            #define jmzk_INVALID_SIGNATURE       -5
            #define jmzk_INVALID_HASH            -6
            #define jmzk_INVALID_ACTION          -7
            #define jmzk_INVALID_BINARY          -8
            #define jmzk_INVALID_JSON            -9
            #define jmzk_INVALID_ADDRESS         -10
            #define jmzk_SIZE_NOT_EQUALS         -11
            #define jmzk_DATA_NOT_EQUALS         -12
            #define jmzk_INVALID_LINK            -13

            int jmzk_free(void*);
            int jmzk_equals(jmzk_data_t* rhs, jmzk_data_t* lhs);
            int jmzk_version();
            int jmzk_last_error();


            typedef jmzk_data_t jmzk_bin_t;
            typedef jmzk_data_t jmzk_chain_id_t;
            typedef jmzk_data_t jmzk_block_id_t;
            typedef jmzk_data_t jmzk_public_key_t;
            typedef jmzk_data_t jmzk_private_key_t;
            typedef jmzk_data_t jmzk_signature_t;
            typedef jmzk_data_t jmzk_checksum_t;
            typedef jmzk_data_t jmzk_address_t;
            typedef void       jmzk_link_t;


            void* jmzk_abi();
            void jmzk_free_abi(void* abi);
            int jmzk_abi_json_to_bin(void* jmzk_abi, const char* action, const char* json, jmzk_bin_t** bin /* out */);
            int jmzk_abi_bin_to_json(void* jmzk_abi, const char* action, jmzk_bin_t* bin, char** json /* out */);
            int jmzk_trx_json_to_digest(void* jmzk_abi, const char* json, jmzk_chain_id_t* chain_id, jmzk_checksum_t** digest /* out */);
            int jmzk_chain_id_from_string(const char* str, jmzk_chain_id_t** chain_id /* out */);


            int jmzk_generate_new_pair(jmzk_public_key_t** pub_key /* out */, jmzk_private_key_t** priv_key /* out */);
            int jmzk_get_public_key(jmzk_private_key_t* priv_key, jmzk_public_key_t** pub_key /* out */);
            int jmzk_sign_hash(jmzk_private_key_t* priv_key, jmzk_checksum_t* hash, jmzk_signature_t** sign /* out */);
            int jmzk_recover(jmzk_signature_t* sign, jmzk_checksum_t* hash, jmzk_public_key_t** pub_key /* out */);
            int jmzk_hash(const char* buf, size_t sz, jmzk_checksum_t** hash /* out */);

            int jmzk_public_key_string(jmzk_public_key_t* pub_key, char** str /* out */);
            int jmzk_private_key_string(jmzk_private_key_t* priv_key, char** str /* out */);
            int jmzk_signature_string(jmzk_signature_t* sign, char** str /* out */);
            int jmzk_checksum_string(jmzk_checksum_t* hash, char** str /* out */);

            int jmzk_public_key_from_string(const char* str, jmzk_public_key_t** pub_key /* out */);
            int jmzk_private_key_from_string(const char* str, jmzk_private_key_t** priv_key /* out */);
            int jmzk_signature_from_string(const char* str, jmzk_signature_t** sign /* out */);
            int jmzk_checksum_from_string(const char* str, jmzk_checksum_t** hash /* out */);

            int jmzk_block_id_from_string(const char* str, jmzk_block_id_t** block_id /* out */);
            int jmzk_ref_block_num(jmzk_block_id_t* block_id, uint16_t* ref_block_num);
            int jmzk_ref_block_prefix(jmzk_block_id_t* block_id, uint32_t* ref_block_prefix);
            
            int jmzk_address_from_string(const char* str, jmzk_address_t** addr /* out */);
            int jmzk_address_to_string(jmzk_address_t* addr, char** str /* out */);
            int jmzk_address_public_key(jmzk_public_key_t *pub_key, jmzk_address_t** addr/* out */);
            int jmzk_address_reserved(jmzk_address_t** addr/* out */);
            int jmzk_address_generated(const char* prefix, const char* key, uint32_t nonce, jmzk_address_t** addr/* out */);
            int jmzk_address_get_public_key(jmzk_address_t* addr, jmzk_public_key_t **pub_key/* out */);
            int jmzk_address_get_prefix(jmzk_address_t* addr, char** str/* out */);
            int jmzk_address_get_key(jmzk_address_t* addr, char** str/* out */);
            int jmzk_address_get_nonce(jmzk_address_t* addr, uint32_t* nonce/* out */);
            int jmzk_address_get_type(jmzk_address_t* addr, char** str/* out */);

            jmzk_link_t* jmzk_link_new();
            void jmzk_link_free(jmzk_link_t*);
            int jmzk_link_tostring(jmzk_link_t*, char**);
            int jmzk_link_parse_from_jmzkli(const char*, jmzk_link_t*);
            int jmzk_link_get_header(jmzk_link_t*, uint16_t*);
            int jmzk_link_set_header(jmzk_link_t*, uint16_t);
            int jmzk_link_get_segment_int(jmzk_link_t*, uint8_t, uint32_t*);
            int jmzk_link_get_segment_str(jmzk_link_t*, uint8_t, char**);
            int jmzk_link_add_segment_int(jmzk_link_t*, uint8_t, uint32_t);
            int jmzk_link_add_segment_str(jmzk_link_t*, uint8_t, const char*);
            int jmzk_link_get_signatures(jmzk_link_t*, jmzk_signature_t***, uint32_t*);
            int jmzk_link_sign(jmzk_link_t*, jmzk_private_key_t*);
            """)

    if sys.platform == 'linux':
        ext = '.so'
    elif sys.platform == 'darwin':
        ext = '.dylib'
    else:
        raise Exception('Not supported platform: {}'.format(sys.platform))

    if 'LIBjmzk_PATH' in os.environ:
        Libjmzk.lib = Libjmzk.ffi.dlopen(
            os.environ['LIBjmzk_PATH'] + '/libjmzk' + ext)
    else:
        Libjmzk.lib = Libjmzk.ffi.dlopen('libjmzk' + ext)

    Libjmzk.abi = Libjmzk.lib.jmzk_abi()


def init_lib():
    Libjmzk.ffi.init_once(init_jmzk_lib, 'init_jmzk')
    return Libjmzk.lib.jmzk_version()
