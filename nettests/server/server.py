import zmq


class Server(object):

    def __init__(self, socket):
        super(Server, self).__init__()
        self.socket = socket
        self.fd = socket.fd
        self.callback = None
        self.reactor = None

    def fileno(self):
        return self.fd

    def logPrefix(self):
        return '.'.join([self.__class__.__name__, str(self.fileno())])

    def __str__(self):
        return self.logPrefix()

    def connectionLost(self, reason):
        print('{} unregistering from {}'.format(self, self.reactor))
        self.reactor.removeReader(self)
        print('connection lost: {}'.format(reason))

    def onReadable(self, callback):
        self.callback = callback
        return self

    def doRead(self):
        while self.socket.events & zmq.POLLIN:
            self.callback(self.socket)

    def registerOn(self, reactor):
        if not self.callback:
            raise Exception('callback is not set')
        print('{} registered in {}'.format(self, reactor))
        self.reactor = reactor
        self.reactor.addReader(self)
