from . import jmzk_exception, libjmzk
from .ecc import PublicKey
from .jmzk_data import jmzkData


class Address(jmzkData):
    def __init__(self, data):
        super().__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = self.jmzk.ffi.new('char**')
        ret = self.jmzk.lib.jmzk_address_to_string(self.data, str_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        str = self.jmzk.ffi.string(str_c[0]).decode('utf-8')
        ret = self.jmzk.lib.jmzk_free(str_c[0])
        jmzk_exception.jmzk_exception_raiser(ret)
        return str

    @staticmethod
    def from_string(str):
        jmzk = libjmzk.check_lib_init()
        addr_c = jmzk.ffi.new('jmzk_address_t**')
        str_c = bytes(str, encoding='utf-8')
        ret = jmzk.lib.jmzk_address_from_string(str_c, addr_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return Address(addr_c[0])

    @staticmethod
    def public_key(pub_key):
        jmzk = libjmzk.check_lib_init()
        addr_c = jmzk.ffi.new('jmzk_address_t**')
        ret = jmzk.lib.jmzk_address_public_key(pub_key.data, addr_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return Address(addr_c[0])

    @staticmethod
    def reserved():
        jmzk = libjmzk.check_lib_init()
        addr_c = jmzk.ffi.new('jmzk_address_t**')
        ret = jmzk.lib.jmzk_address_reserved(addr_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return Address(addr_c[0])

    @staticmethod
    def generated(prefix, key, nonce):
        jmzk = libjmzk.check_lib_init()
        addr_c = jmzk.ffi.new('jmzk_address_t**')
        prefix_c = bytes(prefix, encoding='utf-8')
        key_c = bytes(key, encoding='utf-8')
        ret = jmzk.lib.jmzk_address_generated(prefix_c, key_c, nonce, addr_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return Address(addr_c[0])

    def get_public_key(self):
        pub_key_c = self.jmzk.ffi.new('jmzk_public_key_t**')
        ret = self.jmzk.lib.jmzk_address_get_public_key(self.data, pub_key_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return PublicKey(pub_key_c[0])

    def get_prefix(self):
        str_c = self.jmzk.ffi.new('char**')
        ret = self.jmzk.lib.jmzk_address_get_prefix(self.data, str_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        str = self.jmzk.ffi.string(str_c[0]).decode('utf-8')
        ret = self.jmzk.lib.jmzk_free(str_c[0])
        jmzk_exception.jmzk_exception_raiser(ret)
        return str

    def get_key(self):
        str_c = self.jmzk.ffi.new('char**')
        ret = self.jmzk.lib.jmzk_address_get_key(self.data, str_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        str = self.jmzk.ffi.string(str_c[0]).decode('utf-8')
        ret = self.jmzk.lib.jmzk_free(str_c[0])
        jmzk_exception.jmzk_exception_raiser(ret)
        return str

    def get_nonce(self):
        nonce_c = self.jmzk.ffi.new('uint32_t*')
        ret = self.jmzk.lib.jmzk_address_get_nonce(self.data, nonce_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return int(nonce_c[0])

    def get_type(self):
        str_c = self.jmzk.ffi.new('char**')
        ret = self.jmzk.lib.jmzk_address_get_type(self.data, str_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        str = self.jmzk.ffi.string(str_c[0]).decode('utf-8')
        ret = self.jmzk.lib.jmzk_free(str_c[0])
        jmzk_exception.jmzk_exception_raiser(ret)
        return str
