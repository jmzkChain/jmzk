from . import libevt


class EVTErrCode:
    EVT_OK = 0
    EVT_INTERNAL_ERROR = -1
    EVT_INVALID_ARGUMENT = -2
    EVT_INVALID_PRIVATE_KEY = -3
    EVT_INVALID_PUBLIC_KEY = -4
    EVT_INVALID_SIGNATURE = -5
    EVT_INVALID_HASH = -6
    EVT_INVALID_ACTION = -7
    EVT_INVALID_BINARY = -8
    EVT_INVALID_JSON = -9
    EVT_INVALID_ADDRESS = -10
    EVT_SIZE_NOT_EQUALS = -11
    EVT_DATA_NOT_EQUALS = -12
    EVT_INVALID_LINK = -13
    EVT_NOT_INIT = -15


class EVTException(Exception):
    def __init__(self, err):
        if err == 'EVT_INTERNAL_ERROR':
            evt = libevt.check_lib_init()
            code = evt.evt_last_error()

            errmsg = '{}: {}'.format(err, code)
            super().__init__(self, errmsg)
        else:
            super().__init__(self, err)


class EVTInternalErrorException(Exception):
    def __init__(self):
        err = 'EVT_INTERNAL_ERROR'
        super().__init__(self, err)


class EVTInvalidArgumentException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_ARGUMENT'
        super().__init__(self, err)


class EVTInvalidPrivateKeyException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_PRIVATE_KEY'
        super().__init__(self, err)


class EVTInvalidPublicKeyException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_PUBLIC_KEY'
        super().__init__(self, err)


class EVTInvalidSignatureException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_SIGNATURE'
        super().__init__(self, err)


class EVTInvalidHashException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_HASH'
        super().__init__(self, err)


class EVTInvalidActionException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_ACTION'
        super().__init__(self, err)


class EVTInvalidBinaryException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_BINARY'
        super().__init__(self, err)


class EVTInvalidJsonException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_JSON'
        super().__init__(self, err)


class EVTInvalidAddressException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_ADDRESS'
        super().__init__(self, err)


class EVTSizeNotEqualsException(Exception):
    def __init__(self):
        err = 'EVT_SIZE_NOT_EQUALS'
        super().__init__(self, err)


class EVTDataNotEqualsException(Exception):
    def __init__(self):
        err = 'EVT_DATA_NOT_EQUALS'
        super().__init__(self, err)


class EVTInvalidLinkException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_LINK'
        super().__init__(self, err)


class EVTNotInitException(Exception):
    def __init__(self):
        err = 'EVT_NOT_INIT'
        super().__init__(self, err)


ex_map = {
    EVTErrCode.EVT_INTERNAL_ERROR: EVTInternalErrorException,
    EVTErrCode.EVT_INVALID_ARGUMENT: EVTInvalidArgumentException,
    EVTErrCode.EVT_INVALID_PRIVATE_KEY: EVTInvalidPrivateKeyException,
    EVTErrCode.EVT_INVALID_PUBLIC_KEY: EVTInvalidPublicKeyException,
    EVTErrCode.EVT_INVALID_SIGNATURE: EVTInvalidSignatureException,
    EVTErrCode.EVT_INVALID_HASH: EVTInvalidHashException,
    EVTErrCode.EVT_INVALID_ACTION: EVTInvalidActionException,
    EVTErrCode.EVT_INVALID_BINARY: EVTInvalidBinaryException,
    EVTErrCode.EVT_INVALID_JSON: EVTInvalidJsonException,
    EVTErrCode.EVT_INVALID_ADDRESS: EVTInvalidAddressException,
    EVTErrCode.EVT_INVALID_LINK: EVTInvalidLinkException,
    EVTErrCode.EVT_SIZE_NOT_EQUALS: EVTSizeNotEqualsException,
    EVTErrCode.EVT_DATA_NOT_EQUALS: EVTDataNotEqualsException,
    EVTErrCode.EVT_NOT_INIT: EVTNotInitException
}


def evt_exception_raiser(error_code):
    if error_code == EVTErrCode.EVT_OK:
        return
    if error_code in ex_map:
        raise ex_map[error_code]
    raise Exception('Unknown error code')
