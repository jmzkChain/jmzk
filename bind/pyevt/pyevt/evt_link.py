from . import evt_exception, libevt
from .evt_data import EvtData
from .ecc import Signature
from enum import Enum


class SegmentType(Enum):
    timestamp = 42
    max_pay = 43
    symbol_id = 44
    domain = 91
    token = 92
    max_pay_str = 94
    address = 95
    link_id = 156


class EvtLink():
    def __init__(self):
        self.evt = libevt.check_lib_init()
        self.linkp = self.evt.lib.evt_link_new()

    def __del__(self):
        self.free()

    def free(self):
        self.evt.lib.evt_link_free(self.linkp)

    def __str__(self):
        return self.to_string()

    def to_string(self):
        strv_c = self.evt.ffi.new('char**')
        ret = self.evt.lib.evt_link_tostring(self.linkp, strv_c)
        evt_exception.evt_exception_raiser(ret)
        return self.evt.ffi.string(strv_c[0]).decode('utf-8')

    @staticmethod
    def parse_from_evtli(link_str):
        evt_link = EvtLink()
        str_c = bytes(link_str, encoding='utf-8')
        ret = evt_link.evt.lib.evt_link_parse_from_evtli(str_c, evt_link.linkp)
        evt_exception.evt_exception_raiser(ret)
        return evt_link

    def get_header(self):
        header_c = self.evt.ffi.new('uint16_t*')
        ret = self.evt.lib.evt_link_get_header(self.linkp, header_c)
        evt_exception.evt_exception_raiser(ret)
        return int(header_c[0])

    def set_header(self, header):
        ret = self.evt.lib.evt_link_set_header(self.linkp, header)
        evt_exception.evt_exception_raiser(ret)

    def get_segment_str(self, type_str):
        intv_c = self.evt.ffi.new('uint32_t*')
        strv_c = self.evt.ffi.new('char**')
        ret = self.evt.lib.evt_link_get_segment(self.linkp, SegmentType[type_str].value, intv_c, strv_c)
        evt_exception.evt_exception_raiser(ret)
        strv = self.evt.ffi.string(strv_c[0]).decode('utf-8')
        return strv

    def get_segment_int(self, type_str):
        intv_c = self.evt.ffi.new('uint32_t*')
        strv_c = self.evt.ffi.new('char**')
        ret = self.evt.lib.evt_link_get_segment(self.linkp, SegmentType[type_str].value, intv_c, strv_c)
        evt_exception.evt_exception_raiser(ret)
        return int(intv_c[0])

    def add_segment_str(self, type_str, strv):
        strv_c = bytes(strv, encoding='utf-8')
        ret = self.evt.lib.evt_link_add_segment_str(self.linkp, SegmentType[type_str].value, strv_c)
        evt_exception.evt_exception_raiser(ret)
    
    def add_segment_int(self, type_str, intv):
        ret = self.evt.lib.evt_link_add_segment_int(self.linkp, SegmentType[type_str].value, intv)
        evt_exception.evt_exception_raiser(ret)

    def get_signatures(self):
        signs_c = self.evt.ffi.new('evt_signature_t***')
        len_c = self.evt.ffi.new('uint32_t*')
        ret = self.evt.lib.evt_link_get_signatures(self.linkp, signs_c, len_c)
        evt_exception.evt_exception_raiser(ret)
        l = [Signature(signs_c[0][i]) for i in range(int(len_c[0]))]
        return l

    def sign(self, priv_key):
        ret = self.evt.lib.evt_link_sign(self.linkp, priv_key.data)
        evt_exception.evt_exception_raiser(ret)