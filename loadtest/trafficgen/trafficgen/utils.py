import struct

import lz4.frame


class Reader:
    def __init__(self, filename):
        self.file = lz4.frame.open(filename, 'rb')

    def read(self, size):
        return self.file.read(size)

    def read_trx(self):
        header = self.file.read(4)
        size = struct.unpack('i', header)
        print(size)
        content = self.file.read(size[0])
        return str(content, encoding='utf-8')

    def close(self):
        self.file.close()


class Writer:
    def __init__(self, filename):
        self.file = lz4.frame.open(filename, 'wb')

    def write(self, data):
        self.file.write(data)

    def write_trx(self, trx):
        content = bytes(trx, encoding='utf-8')
        size = len(content)
        header = struct.pack('i', size)
        self.file.write(header)
        self.file.write(content)

    def close(self):
        self.file.close()
