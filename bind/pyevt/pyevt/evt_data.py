import evt_exception
import libevt


class EvtData:
    def __init__(self, data):
        self.data = data
        self.evt = libevt.check_lib_init()

    def __del__(self):
        ret = self.evt.lib.evt_free(self.data)
        evt_exception.evt_exception_raiser(ret)
