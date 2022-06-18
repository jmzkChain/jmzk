from . import libjmzk


class jmzkErrCode:
    jmzk_OK = 0
    jmzk_INTERNAL_ERROR = -1
    jmzk_INVALID_ARGUMENT = -2
    jmzk_INVALID_PRIVATE_KEY = -3
    jmzk_INVALID_PUBLIC_KEY = -4
    jmzk_INVALID_SIGNATURE = -5
    jmzk_INVALID_HASH = -6
    jmzk_INVALID_ACTION = -7
    jmzk_INVALID_BINARY = -8
    jmzk_INVALID_JSON = -9
    jmzk_INVALID_ADDRESS = -10
    jmzk_SIZE_NOT_EQUALS = -11
    jmzk_DATA_NOT_EQUALS = -12
    jmzk_INVALID_LINK = -13
    jmzk_NOT_INIT = -15


class jmzkException(Exception):
    def __init__(self, err):
        if err == 'jmzk_INTERNAL_ERROR':
            jmzk = libjmzk.check_lib_init()
            code = jmzk.jmzk_last_error()

            errmsg = '{}: {}'.format(err, code)
            super().__init__(self, errmsg)
        else:
            super().__init__(self, err)


class jmzkInternalErrorException(Exception):
    def __init__(self):
        err = 'jmzk_INTERNAL_ERROR'
        super().__init__(self, err)


class jmzkInvalidArgumentException(Exception):
    def __init__(self):
        err = 'jmzk_INVALID_ARGUMENT'
        super().__init__(self, err)


class jmzkInvalidPrivateKeyException(Exception):
    def __init__(self):
        err = 'jmzk_INVALID_PRIVATE_KEY'
        super().__init__(self, err)


class jmzkInvalidPublicKeyException(Exception):
    def __init__(self):
        err = 'jmzk_INVALID_PUBLIC_KEY'
        super().__init__(self, err)


class jmzkInvalidSignatureException(Exception):
    def __init__(self):
        err = 'jmzk_INVALID_SIGNATURE'
        super().__init__(self, err)


class jmzkInvalidHashException(Exception):
    def __init__(self):
        err = 'jmzk_INVALID_HASH'
        super().__init__(self, err)


class jmzkInvalidActionException(Exception):
    def __init__(self):
        err = 'jmzk_INVALID_ACTION'
        super().__init__(self, err)


class jmzkInvalidBinaryException(Exception):
    def __init__(self):
        err = 'jmzk_INVALID_BINARY'
        super().__init__(self, err)


class jmzkInvalidJsonException(Exception):
    def __init__(self):
        err = 'jmzk_INVALID_JSON'
        super().__init__(self, err)


class jmzkInvalidAddressException(Exception):
    def __init__(self):
        err = 'jmzk_INVALID_ADDRESS'
        super().__init__(self, err)


class jmzkSizeNotEqualsException(Exception):
    def __init__(self):
        err = 'jmzk_SIZE_NOT_EQUALS'
        super().__init__(self, err)


class jmzkDataNotEqualsException(Exception):
    def __init__(self):
        err = 'jmzk_DATA_NOT_EQUALS'
        super().__init__(self, err)


class jmzkInvalidLinkException(Exception):
    def __init__(self):
        err = 'jmzk_INVALID_LINK'
        super().__init__(self, err)


class jmzkNotInitException(Exception):
    def __init__(self):
        err = 'jmzk_NOT_INIT'
        super().__init__(self, err)


ex_map = {
    jmzkErrCode.jmzk_INTERNAL_ERROR: jmzkInternalErrorException,
    jmzkErrCode.jmzk_INVALID_ARGUMENT: jmzkInvalidArgumentException,
    jmzkErrCode.jmzk_INVALID_PRIVATE_KEY: jmzkInvalidPrivateKeyException,
    jmzkErrCode.jmzk_INVALID_PUBLIC_KEY: jmzkInvalidPublicKeyException,
    jmzkErrCode.jmzk_INVALID_SIGNATURE: jmzkInvalidSignatureException,
    jmzkErrCode.jmzk_INVALID_HASH: jmzkInvalidHashException,
    jmzkErrCode.jmzk_INVALID_ACTION: jmzkInvalidActionException,
    jmzkErrCode.jmzk_INVALID_BINARY: jmzkInvalidBinaryException,
    jmzkErrCode.jmzk_INVALID_JSON: jmzkInvalidJsonException,
    jmzkErrCode.jmzk_INVALID_ADDRESS: jmzkInvalidAddressException,
    jmzkErrCode.jmzk_INVALID_LINK: jmzkInvalidLinkException,
    jmzkErrCode.jmzk_SIZE_NOT_EQUALS: jmzkSizeNotEqualsException,
    jmzkErrCode.jmzk_DATA_NOT_EQUALS: jmzkDataNotEqualsException,
    jmzkErrCode.jmzk_NOT_INIT: jmzkNotInitException
}


def jmzk_exception_raiser(error_code):
    if error_code == jmzkErrCode.jmzk_OK:
        return
    if error_code in ex_map:
        raise ex_map[error_code]
    raise Exception('Unknown error code')
