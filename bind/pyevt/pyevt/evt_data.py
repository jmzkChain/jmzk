from cffi import FFI

import evt_exception
import libevt


class EvtData:
    def __init__(self, data):
        self.data = data
        libevt.check_lib_init()

    def __del__(self):
        ret = libevt.libevt.lib.evt_free(self.data)
        evt_exception.evt_exception_raiser(ret)
