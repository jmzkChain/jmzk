import evt_exception
import libevt
from evt_data import EvtData


class ChainId(EvtData):
    def __init__(self, data):
        super().__init__(data)

    @staticmethod
    def from_string(str):
        evt = libevt.check_lib_init()
        str_c = bytes(str, encoding='utf-8')
        chain_id_c = evt.ffi.new('evt_chain_id_t**')
        ret = evt.lib.evt_chain_id_from_string(str_c, chain_id_c)
        evt_exception.evt_exception_raiser(ret)
        return ChainId(chain_id_c[0])


class PublicKey(EvtData):
    def __init__(self, data):
        super().__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = self.evt.ffi.new('char**')
        ret = self.evt.lib.evt_public_key_string(self.data, str_c)
        evt_exception.evt_exception_raiser(ret)
        str = self.evt.ffi.string(str_c[0]).decode('utf-8')
        ret = self.evt.lib.evt_free(str_c[0])
        evt_exception.evt_exception_raiser(ret)
        return str

    @staticmethod
    def from_string(str):
        evt = libevt.check_lib_init()
        public_key_c = evt.ffi.new('evt_public_key_t**')
        str_c = bytes(str, encoding='utf-8')
        ret = evt.lib.evt_public_key_from_string(str_c, public_key_c)
        evt_exception.evt_exception_raiser(ret)
        return PublicKey(public_key_c[0])

    @staticmethod
    def recover(sign, hash):
        evt = libevt.check_lib_init()
        public_key_c = evt.ffi.new('evt_public_key_t**')
        ret = evt.lib.evt_recover(sign.data, hash.data, public_key_c)
        evt_exception.evt_exception_raiser(ret)
        return PublicKey(public_key_c[0])


class PrivateKey(EvtData):
    def __init__(self, data):
        super().__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = self.evt.ffi.new('char**')
        ret = self.evt.lib.evt_private_key_string(self.data, str_c)
        evt_exception.evt_exception_raiser(ret)
        str = self.evt.ffi.string(str_c[0]).decode('utf-8')
        ret = self.evt.lib.evt_free(str_c[0])
        evt_exception.evt_exception_raiser(ret)
        return str

    def get_public_key(self):
        public_key_c = self.evt.ffi.new('evt_public_key_t**')
        ret = self.evt.lib.evt_get_public_key(self.data, public_key_c)
        evt_exception.evt_exception_raiser(ret)
        return PublicKey(public_key_c[0])

    def sign_hash(self, hash):
        signature_c = self.evt.ffi.new('evt_signature_t**')
        ret = self.evt.lib.evt_sign_hash(
            self.data, hash.data, signature_c)
        evt_exception.evt_exception_raiser(ret)
        return Signature(signature_c[0])

    @staticmethod
    def from_string(str):
        evt = libevt.check_lib_init()
        private_key_c = evt.ffi.new('evt_private_key_t**')
        str_c = bytes(str, encoding='utf-8')
        ret = evt.lib.evt_private_key_from_string(
            str_c, private_key_c)
        evt_exception.evt_exception_raiser(ret)
        return PrivateKey(private_key_c[0])


class Signature(EvtData):
    def __init__(self, data):
        super().__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = self.evt.ffi.new('char**')
        ret = self.evt.lib.evt_signature_string(str_c, self.data)
        evt_exception.evt_exception_raiser(ret)
        str = self.evt.ffi.string(str_c[0]).decode('utf-8')
        ret = self.evt.lib.evt_free(str_c[0])
        evt_exception.evt_exception_raiser(ret)
        return str


class Checksum(EvtData):
    def __init__(self, data):
        super().__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = self.evt.ffi.new('char**')
        ret = self.evt.lib.evt_checksum_string(self.data, str_c)
        evt_exception.evt_exception_raiser(ret)
        str = self.evt.ffi.string(str_c[0]).decode('utf-8')
        ret = self.evt.lib.evt_free(str_c[0])
        evt_exception.evt_exception_raiser(ret)
        return str

    @staticmethod
    def from_string(str):
        evt = libevt.check_lib_init()
        str_c = bytes(str, encoding='utf-8')
        evt_hash = evt.ffi.new('evt_checksum_t**')
        ret = evt.lib.evt_hash(str_c, len(str_c), evt_hash)
        evt_exception.evt_exception_raiser(ret)
        return Checksum(evt_hash[0])


def generate_new_pair():
    evt = libevt.check_lib_init()
    public_key_c = evt.ffi.new('evt_public_key_t**')
    private_key_c = evt.ffi.new('evt_private_key_t**')
    ret = evt.lib.evt_generate_new_pair(public_key_c, private_key_c)
    evt_exception.evt_exception_raiser(ret)
    return PublicKey(public_key_c[0]), PrivateKey(private_key_c[0])
