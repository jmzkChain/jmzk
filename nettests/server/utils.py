from twisted.internet import reactor
from twisted.internet.defer import Deferred, succeed
from twisted.internet.protocol import Protocol
from twisted.web.client import Agent, readBody
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


def post_cb(url, callback, args=None, method='GET', body=None):
    agent = Agent(reactor)
    d = agent.request(
        bytes(method, encoding='utf-8'),
        bytes(url, encoding='utf-8'),
        bodyProducer=StringProducer(body) if body else None
    )

    def cb_read_body(response):
        if response.code != 200:
            return
        d = readBody(response)
        d.addCallback(callback, *args)
    d.addCallback(cb_read_body)
