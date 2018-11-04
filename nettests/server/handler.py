import json

from twisted.internet import reactor
from twisted.internet.defer import Deferred, succeed
from twisted.internet.protocol import Protocol
from twisted.web.client import Agent
from twisted.web.iweb import IBodyProducer


class StringProducer(object):
    def __init__(self, body):
        self.body = bytes(body, encoding='utf-8')
        self.length = len(body)

    def startProducing(self, consumer):
        consumer.write(self.body)
        return succeed(None)

    def pauseProducing(self):
        pass

    def stopProducing(self):
        pass


class ResourcePrinter(Protocol):
    def __init__(self, finished):
        self.finished = finished

    def dataReceived(self, data):
        print(str(data, encoding='utf-8'))

    def connectionLost(self, reason):
        self.finished.callback(None)


def printResource(response):
    finished = Deferred()
    response.deliverBody(ResourcePrinter(finished))
    return finished


def printError(failure):
    print(failure)


class Handler:
    def __call__(self, socket):
        data = socket.recv_string()
        cmd = json.loads(data)
        func = cmd['func']
        if func == 'kill':
            reactor.stop()
        elif func == 'send':
            agent = Agent(reactor)
            body = StringProducer(cmd['data'])
            d = agent.request(
                bytes('POST', encoding='utf-8'),
                bytes(cmd['url'], encoding='utf-8'),
                bodyProducer=body)
            d.addCallback(printResource)
        else:
            raise Exception('No such function')
