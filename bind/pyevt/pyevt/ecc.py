from . import jmzk_exception, libjmzk
from .jmzk_data import jmzkData


class PublicKey(jmzkData):
    def __init__(self, data):
        super().__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = self.jmzk.ffi.new('char**')
        ret = self.jmzk.lib.jmzk_public_key_string(self.data, str_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        str = self.jmzk.ffi.string(str_c[0]).decode('utf-8')
        ret = self.jmzk.lib.jmzk_free(str_c[0])
        jmzk_exception.jmzk_exception_raiser(ret)
        return str

    @staticmethod
    def from_string(str):
        jmzk = libjmzk.check_lib_init()
        public_key_c = jmzk.ffi.new('jmzk_public_key_t**')
        str_c = bytes(str, encoding='utf-8')
        ret = jmzk.lib.jmzk_public_key_from_string(str_c, public_key_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return PublicKey(public_key_c[0])

    @staticmethod
    def recover(sign, hash):
        jmzk = libjmzk.check_lib_init()
        public_key_c = jmzk.ffi.new('jmzk_public_key_t**')
        ret = jmzk.lib.jmzk_recover(sign.data, hash.data, public_key_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return PublicKey(public_key_c[0])


class PrivateKey(jmzkData):
    def __init__(self, data):
        super().__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = self.jmzk.ffi.new('char**')
        ret = self.jmzk.lib.jmzk_private_key_string(self.data, str_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        str = self.jmzk.ffi.string(str_c[0]).decode('utf-8')
        ret = self.jmzk.lib.jmzk_free(str_c[0])
        jmzk_exception.jmzk_exception_raiser(ret)
        return str

    def get_public_key(self):
        public_key_c = self.jmzk.ffi.new('jmzk_public_key_t**')
        ret = self.jmzk.lib.jmzk_get_public_key(self.data, public_key_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return PublicKey(public_key_c[0])

    def sign_hash(self, hash):
        signature_c = self.jmzk.ffi.new('jmzk_signature_t**')
        ret = self.jmzk.lib.jmzk_sign_hash(
            self.data, hash.data, signature_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return Signature(signature_c[0])

    @staticmethod
    def from_string(str):
        jmzk = libjmzk.check_lib_init()
        private_key_c = jmzk.ffi.new('jmzk_private_key_t**')
        str_c = bytes(str, encoding='utf-8')
        ret = jmzk.lib.jmzk_private_key_from_string(
            str_c, private_key_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        return PrivateKey(private_key_c[0])


class Signature(jmzkData):
    def __init__(self, data):
        super().__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = self.jmzk.ffi.new('char**')
        ret = self.jmzk.lib.jmzk_signature_string(self.data, str_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        str = self.jmzk.ffi.string(str_c[0]).decode('utf-8')
        ret = self.jmzk.lib.jmzk_free(str_c[0])
        jmzk_exception.jmzk_exception_raiser(ret)
        return str


class Checksum(jmzkData):
    def __init__(self, data):
        super().__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = self.jmzk.ffi.new('char**')
        ret = self.jmzk.lib.jmzk_checksum_string(self.data, str_c)
        jmzk_exception.jmzk_exception_raiser(ret)
        str = self.jmzk.ffi.string(str_c[0]).decode('utf-8')
        ret = self.jmzk.lib.jmzk_free(str_c[0])
        jmzk_exception.jmzk_exception_raiser(ret)
        return str

    @staticmethod
    def from_string(str):
        jmzk = libjmzk.check_lib_init()
        str_c = bytes(str, encoding='utf-8')
        jmzk_hash = jmzk.ffi.new('jmzk_checksum_t**')
        ret = jmzk.lib.jmzk_hash(str_c, len(str_c), jmzk_hash)
        jmzk_exception.jmzk_exception_raiser(ret)
        return Checksum(jmzk_hash[0])


def generate_new_pair():
    jmzk = libjmzk.check_lib_init()
    public_key_c = jmzk.ffi.new('jmzk_public_key_t**')
    private_key_c = jmzk.ffi.new('jmzk_private_key_t**')
    ret = jmzk.lib.jmzk_generate_new_pair(public_key_c, private_key_c)
    jmzk_exception.jmzk_exception_raiser(ret)
    return PublicKey(public_key_c[0]), PrivateKey(private_key_c[0])
