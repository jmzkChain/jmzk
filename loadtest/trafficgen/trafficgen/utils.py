import struct

import cffi

ffi = cffi.FFI()
ffi.cdef('''
    int LZ4_compress(const char* src, char* dst, int isize);
    int LZ4_uncompress_unknownOutputSize(const char* src, char* dst, int isize, int max_osize);
    int LZ4_compressBound(int isize);
''')
lz4 = ffi.dlopen('lz4')


def compress(src):
    dest_len = lz4.LZ4_compressBound(len(src))
    dest = ffi.new('char[]', dest_len)
    l = lz4.LZ4_compress(src, dest, len(src))
    if l == 0:
        return None
    else:
        return ffi.buffer(dest, l)[:]


def uncompress(src, osize):
    dest = ffi.new('char[]', osize)
    l = lz4.LZ4_uncompress_unknownOutputSize(src, dest, len(src), osize)
    if l > 0:
        return ffi.buffer(dest, l)[:]
    else:
        return None


def bin2short(data):
    return struct.unpack('H', data)[0]


def short2bin(data):
    return struct.pack('H', data)


class Reader:
    def __init__(self, filename):
        self.file = open(filename, 'rb')

    def read(self, size):
        return self.file.read(size)

    def read_trx(self):
        bsize = bin2short(self.file.read(2))
        csize = bin2short(self.file.read(2))
        trx_c = self.file.read(csize)
        trx_b = uncompress(trx_c, bsize)
        return str(trx_b, encoding='utf-8')

    def close(self):
        self.file.close()


class Writer:
    def __init__(self, filename):
        self.file = open(filename, 'wb')

    def write(self, data):
        self.file.write(data)

    def write_trx(self, trx):
        trx_b = bytes(trx, encoding='utf-8')
        trx_c = compress(trx_b)
        bsize = len(trx_b)
        csize = len(trx_c)
        self.file.write(short2bin(bsize))
        self.file.write(short2bin(csize))
        self.file.write(trx_c)

    def close(self):
        self.file.close()
