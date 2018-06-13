class evt_error_code:
    EVT_OK = 0
    EVT_INTERNAL_ERROR = -1
    EVT_INVALID_ARGUMENT = -2
    EVT_INVALID_PRIVATE_KEY = -3
    EVT_INVALID_PUBLIC_KEY = -4
    EVT_INVALID_SIGNATURE = -5
    EVT_INVALID_HASH = -6
    EVT_INVALID_ACTION = -7
    EVT_INVALID_BINARY = -8
    EVT_SIZE_NOT_EQUALS = -11
    EVT_DATA_NOT_EQUALS = -12
    EVT_NOT_INIT = -15


class EvtInternalErrorException(Exception):
    def __init__(self):
        err = 'EVT_INTERNAL_ERROR'
        Exception.__init__(self, err)


class EvtInvaildArgumentException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_ARGUMENT'
        Exception.__init__(self, err)


class EvtInvaildPrivateKeyException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_PRIVATE_KEY'
        Exception.__init__(self, err)


class EvtInvaildPublicKeyException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_PUBLIC_KEY'
        Exception.__init__(self, err)


class EvtInvalidSignatureException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_SIGNATURE'
        Exception.__init__(self, err)


class EvtInvalidHashException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_HASH'
        Exception.__init__(self, err)


class EvtInvalidActionException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_ACTION'
        Exception.__init__(self, err)


class EvtInvalidBinaryException(Exception):
    def __init__(self):
        err = 'EVT_INVALID_BINARY'
        Exception.__init__(self, err)


class EvtSizeNotEqualsException(Exception):
    def __init__(self):
        err = 'EVT_SIZE_NOT_EQUALS'
        Exception.__init__(self, err)


class EvtDataNotEqualsException(Exception):
    def __init__(self):
        err = 'EVT_DATA_NOT_EQUALS'
        Exception.__init__(self, err)


class EvtNotInitException(Exception):
    def __init__(self):
        err = 'EVT_NOT_INIT'
        Exception.__init__(self, err)


def evt_exception_raiser(error_code):
    if(error_code == evt_error_code.EVT_OK):
        return
    if(error_code == evt_error_code.EVT_INTERNAL_ERROR):
        raise EvtInternalErrorException()
    if(error_code == evt_error_code.EVT_INVALID_ARGUMENT):
        raise EvtInvaildArgumentException()
    if(error_code == evt_error_code.EVT_INVALID_PRIVATE_KEY):
        raise EvtInvaildPrivateKeyException()
    if(error_code == evt_error_code.EVT_INVALID_PUBLIC_KEY):
        raise EvtInvaildPublicKeyExceptionn()
    if(error_code == evt_error_code.EVT_INVALID_SIGNATURE):
        raise EvtInvalidSignatureException()
    if(error_code == evt_error_code.EVT_INVALID_HASH):
        raise EvtInvalidHashException()
    if(error_code == evt_error_code.EVT_INVALID_ACTION):
        raise EvtInvalidActionException()
    if(error_code == evt_error_code.EVT_INVALID_BINARY):
        raise EvtInvalidBinaryException()
    if(error_code == evt_error_code.EVT_SIZE_NOT_EQUALS):
        raise EvtSizeNotEqualsException()
    if(error_code == evt_error_code.EVT_DATA_NOT_EQUALS):
        raise EvtDataNotEqualsException()
    if(error_code == evt_error_code.EVT_NOT_INIT):
        raise EvtNotInitException()
