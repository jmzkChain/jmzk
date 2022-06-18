from . import jmzk_exception, libjmzk
from .jmzk_data import jmzkData


def version():
    return libjmzk.init_lib()


def json_to_bin(action, json):
    jmzk = libjmzk.check_lib_init()
    action_c = bytes(action, encoding='utf-8')
    json_c = bytes(json, encoding='utf-8')
    bin_c = jmzk.ffi.new('jmzk_bin_t**')
    ret = jmzk.lib.jmzk_abi_json_to_bin(
        jmzk.abi, action_c, json_c, bin_c)
    jmzk_exception.jmzk_exception_raiser(ret)
    return jmzkData(bin_c[0])


def bin_to_json(action, bin):
    jmzk = libjmzk.check_lib_init()
    action_c = bytes(action, encoding='utf-8')
    bin_c = bin.data
    json_c = jmzk.ffi.new('char** ')
    ret = jmzk.lib.jmzk_abi_bin_to_json(
        jmzk.abi, action_c, bin_c, json_c)
    jmzk_exception.jmzk_exception_raiser(ret)
    json = jmzk.ffi.string(json_c[0]).decode('utf-8')
    ret = jmzk.lib.jmzk_free(json_c[0])
    jmzk_exception.jmzk_exception_raiser(ret)
    return json


def trx_json_to_digest(json, chain_id):
    jmzk = libjmzk.check_lib_init()
    json_c = bytes(json, encoding='utf-8')
    chain_id_c = chain_id.data
    digest_c = jmzk.ffi.new('jmzk_checksum_t**')
    ret = jmzk.lib.jmzk_trx_json_to_digest(
        jmzk.abi, json_c, chain_id_c, digest_c)
    jmzk_exception.jmzk_exception_raiser(ret)
    return jmzkData(digest_c[0])


class ChainId(jmzkData):
    def __init__(self, data):
        super().__init__(data)

    @staticmethod
    def from_string(str):
        jmzk = libjmzk.check_lib_init()
        str_c = bytes(str, encoding='utf-8')
        chain_id_c = jmzk.ffi.new('jmzk_chain_id_t**')
        ret = jmzk.lib.jmzk_chain_id_from_string(str_c, chain_id_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return ChainId(chain_id_c[0])


class BlockId(jmzkData):
    def __init__(self, data):
        super().__init__(data)

    @staticmethod
    def from_string(str):
        jmzk = libjmzk.check_lib_init()
        str_c = bytes(str, encoding='utf-8')
        block_id_c = jmzk.ffi.new('jmzk_block_id_t**')
        ret = jmzk.lib.jmzk_block_id_from_string(str_c, block_id_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return BlockId(block_id_c[0])

    def ref_block_num(self):
        uint16_c = self.jmzk.ffi.new('uint16_t*')
        ret = self.jmzk.lib.jmzk_ref_block_num(self.data, uint16_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return int(uint16_c[0])

    def ref_block_prefix(self):
        uint32_c = self.jmzk.ffi.new('uint32_t*')
        ret = self.jmzk.lib.jmzk_ref_block_prefix(self.data, uint32_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return int(uint32_c[0])
