from . import evt_exception, libevt
from .evt_data import EvtData
from enum import Enum


class SegmentType(Enum):
    timestamp = 42
    max_pay = 43
    domain = 91
    token = 92
    symbol = 93
    max_pay_str = 94
    address = 95
    link_id = 156


class EvtLink(EvtData):
    def __init__(self, data):
        super().__init__(data)

    def __str__(self):
        return self.to_string()

    def get_header(self):
        header_c = self.evt.ffi.new('uint16_t*')
        ret = self.evt.lib.evt_link_get_header(self.data, header_c)
        evt_exception.evt_exception_raiser(ret)
        return int(header_c[0])

    def get_segment_str(self, type_str):
        intv_c = self.evt.ffi.new('uint32_t*')
        strv_c = self.evt.ffi.new('char**')
        ret = self.evt.lib.evt_link_get_segment(self.data, SegmentType[type_str].value, intv_c, strv_c)
        evt_exception.evt_exception_raiser(ret)
        strv = self.evt.ffi.string(strv_c[0]).decode('utf-8')
        return strv

    def get_segment_int(self, type_str):
        intv_c = self.evt.ffi.new('uint32_t*')
        strv_c = self.evt.ffi.new('char**')
        ret = self.evt.lib.evt_link_get_segment(self.data, SegmentType[type_str].value, intv_c, strv_c)
        evt_exception.evt_exception_raiser(ret)
        return int(intv_c[0])

    @staticmethod
    def parse_from_evtli(link_str):
        evt = libevt.check_lib_init()
        link_c = evt.ffi.new('evt_link_t**')
        str_c = bytes(link_str, encoding='utf-8')
        ret = evt.lib.evt_link_parse_from_evtli(str_c, link_c)
        evt_exception.evt_exception_raiser(ret)
        return EvtLink(link_c[0])


