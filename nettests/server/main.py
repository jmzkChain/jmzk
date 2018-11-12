import zmq
from twisted.internet import reactor

from handler import Handler
from server import Server

ctx = zmq.Context()
socket = ctx.socket(zmq.REP)
socket.bind('tcp://*:6666')

handler = Handler()

server = Server(socket).onReadable(handler).registerOn(reactor)

reactor.run()
