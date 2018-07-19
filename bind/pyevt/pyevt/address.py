from . import evt_exception, libevt
from .evt_data import EvtData


class Address(EvtData):
    def __init__(self, data):
        super().__init__(data)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        str_c = self.evt.ffi.new('char**')
        ret = self.evt.lib.evt_address_to_string(self.data, str_c)
        evt_exception.evt_exception_raiser(ret)
        str = self.evt.ffi.string(str_c[0]).decode('utf-8')
        ret = self.evt.lib.evt_free(str_c[0])
        evt_exception.evt_exception_raiser(ret)
        return str

    @staticmethod
    def from_string(str):
        evt = libevt.check_lib_init()
        addr_c = evt.ffi.new('evt_address_t**')
        str_c = bytes(str, encoding='utf-8')
        ret = evt.lib.evt_address_from_string(str_c, addr_c)
        evt_exception.evt_exception_raiser(ret)
        return Address(addr_c[0])

    @staticmethod
    def public_key(pub_key):
        evt = libevt.check_lib_init()
        addr_c = evt.ffi.new('evt_address_t**')
        ret = evt.lib.evt_address_public_key(pub_key.data, addr_c)
        evt_exception.evt_exception_raiser(ret)
        return Address(addr_c[0])

    @staticmethod
    def reserved():
        evt = libevt.check_lib_init()
        addr_c = evt.ffi.new('evt_address_t**')
        ret = evt.lib.evt_address_reserved(addr_c)
        evt_exception.evt_exception_raiser(ret)
        return Address(addr_c[0])

    @staticmethod
    def generated(prefix, key, nonce):
        evt = libevt.check_lib_init()
        addr_c = evt.ffi.new('evt_address_t**')
        prefix_c = bytes(prefix, encoding='utf-8')
        key_c = bytes(key, encoding='utf-8')
        ret = evt.lib.evt_address_generated(prefix_c, key_c, nonce, addr_c)
        evt_exception.evt_exception_raiser(ret)
        return Address(addr_c[0])