from io import StringIO

from . import jmzk_exception, libjmzk


class jmzkData:
    def __init__(self, data):
        self.data = data
        self.jmzk = libjmzk.check_lib_init()

    def __del__(self):
        ret = self.jmzk.lib.jmzk_free(self.data)
        jmzk_exception.jmzk_exception_raiser(ret)

    def to_hex_string(self):
        hstr = StringIO()
        for i in range(self.data.sz):
            hstr.write(self.data.buf[i].hex())
        return hstr.getvalue()
