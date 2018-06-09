from cffi import FFI

import evt_exception
import libevt
from evt_data import EvtData


class ChainId(EvtData):
    def __init__(self, data):
        super(ChainId, self).__init__(data)

    def chain_id_from_string(str):
        libevt.check_lib_init()
        str_c = bytes(str, encoding='utf-8')
        chain_id_c = libevt.libevt.ffi.new('evt_chain_id_t**')
        ret = libevt.libevt.lib.evt_chain_id_from_string(str_c, chain_id_c)
        evt_exception.evt_exception_raiser(ret)
        return ChainId(chain_id_c[0])


class PublicKey(EvtData):
    def __init__(self, data):
        super(PublicKey, self).__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = libevt.libevt.ffi.new('char**')
        ret = libevt.libevt.lib.evt_public_key_string(self.data, str_c)
        evt_exception.evt_exception_raiser(ret)
        str = libevt.libevt.ffi.string(str_c[0]).decode('utf-8')
        ret = libevt.libevt.lib.evt_free(str_c[0])
        evt_exception.evt_exception_raiser(ret)
        return str

    def from_string(str):
        libevt.check_lib_init()
        public_key_c = libevt.libevt.ffi.new('evt_public_key_t**')
        str_c = bytes(data, encoding='utf-8')
        ret = libevt.libevt.lib.evt_public_key_from_string(str_c, public_key_c)
        evt_exception.evt_exception_raiser(ret)
        return PublicKey(public_key_c[0])

    def recover(sign, hash):
        libevt.check_lib_init()
        public_key_c = libevt.libevt.ffi.new('evt_public_key_t**')
        ret = libevt.libevt.lib.evt_recover(sign.data, hash.data, public_key_c)
        evt_exception.evt_exception_raiser(ret)
        return PublicKey(public_key_c[0])


class PrivateKey(EvtData):
    def __init__(self, data):
        super(PrivateKey, self).__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = libevt.libevt.ffi.new('char**')
        ret = libevt.libevt.lib.evt_private_key_string(self.data, str_c)
        evt_exception.evt_exception_raiser(ret)
        str = libevt.libevt.ffi.string(str_c[0]).decode('utf-8')
        ret = libevt.libevt.lib.evt_free(str_c[0])
        evt_exception.evt_exception_raiser(ret)
        return str

    def get_public_key(self):
        public_key_c = libevt.libevt.ffi.new('evt_public_key_t**')
        ret = libevt.libevt.lib.evt_get_public_key(self.data, public_key_c)
        evt_exception.evt_exception_raiser(ret)
        return PublicKey(public_key_c[0])

    def sign_hash(self, hash):
        libevt.check_lib_init()
        signature_c = libevt.libevt.ffi.new('evt_signature_t**')
        ret = libevt.libevt.lib.evt_sign_hash(
            self.data, hash.data, signature_c)
        evt_exception.evt_exception_raiser(ret)
        return Signature(signature_c[0])

    def from_string(str):
        libevt.check_lib_init()
        private_key_c = libevt.libevt.ffi.new('evt_private_key_t**')
        str_c = bytes(str, encoding='utf-8')
        ret = libevt.libevt.lib.evt_private_key_from_string(
            str_c, private_key_c)
        evt_exception.evt_exception_raiser(ret)
        return PrivateKey(private_key_c[0])


class Signature(EvtData):
    def __init__(self, data):
        super(Signature, self).__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = libevt.libevt.ffi.new('char**')
        ret = libevt.libevt.lib.evt_signature_string(str_c, self.data)
        evt_exception.evt_exception_raiser(ret)
        str = libevt.libevt.ffi.string(str_c[0]).decode('utf-8')
        ret = libevt.libevt.lib.evt_free(str_c[0])
        evt_exception.evt_exception_raiser(ret)
        return str


class Checksum(EvtData):
    def __init__(self, data):
        super(Checksum, self).__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = libevt.libevt.ffi.new('char**')
        ret = libevt.libevt.lib.evt_checksum_string(self.data, str_c)
        evt_exception.evt_exception_raiser(ret)
        str = libevt.libevt.ffi.string(str_c[0]).decode('utf-8')
        ret = libevt.libevt.lib.evt_free(str_c[0])
        evt_exception.evt_exception_raiser(ret)
        return str

    def from_string(str):
        libevt.check_lib_init()
        str_c = bytes(str, encoding='utf-8')
        evt_hash = libevt.libevt.ffi.new('evt_checksum_t**')
        ret = libevt.libevt.lib.evt_hash(str_c, len(str_c), evt_hash)
        evt_exception.evt_exception_raiser(ret)
        return Checksum(evt_hash[0])


def generate_new_pair():
    libevt.check_lib_init()
    public_key_c = libevt.libevt.ffi.new('evt_public_key_t**')
    private_key_c = libevt.libevt.ffi.new('evt_private_key_t**')
    ret = libevt.libevt.lib.evt_generate_new_pair(public_key_c, private_key_c)
    evt_exception.evt_exception_raiser(ret)
    return PublicKey(public_key_c[0]), PrivateKey(private_key_c[0])



