from io import StringIO

from . import evt_exception, libevt


class EvtData:
    def __init__(self, data):
        self.data = data
        self.evt = libevt.check_lib_init()

    def __del__(self):
        ret = self.evt.lib.evt_free(self.data)
        evt_exception.evt_exception_raiser(ret)

    def to_hex_string(self):
        hstr = StringIO()
        for i in range(self.data.sz):
            hstr.write(self.data.buf[i].hex())
        return hstr.getvalue()
