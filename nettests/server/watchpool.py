import functools
import json

from twisted.internet import reactor
from twisted.internet.defer import Deferred, succeed
from twisted.internet.protocol import Protocol
from twisted.web.client import Agent, readBody
from twisted.web.iweb import IBodyProducer


def singleton(cls):
    cls.__new_original__ = cls.__new__

    @functools.wraps(cls.__new__)
    def singleton_new(cls, *args, **kwargs):
        it = cls.__dict__.get('__it__')
        if it is not None:
            return it

        cls.__it__ = it = cls.__new_original__(cls, *args, **kwargs)
        it.__init_original__(*args, **kwargs)
        return it

    cls.__new__ = singleton_new
    cls.__init_original__ = cls.__init__
    cls.__init__ = object.__init__

    return cls


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


def compare_block_num(body):
    j = json.loads(str(body, encoding='utf-8'))
    block_num = j['block_num']
    link_id = j['transaction']['actions'][0]['data']['link']['segments'][-1]['value']
    if block_num <= WatchPool().irr_block_num:
        WatchPool().del_watch(link_id)
        print('del', link_id, WatchPool().size())


def get_transaction(body):
    j = json.loads(str(body, encoding='utf-8'))
    block_num = j['block_num']
    trx_id = j['trx_id']
    agent = Agent(reactor)
    d2 = agent.request(
        bytes('POST', encoding='utf-8'),
        bytes(WatchPool().url + '/v1/chain/get_transaction', encoding='utf-8'),
        bodyProducer=StringProducer('''
        {
            "block_num": %d,
            "id": "%s"
        }
        ''' % (block_num, trx_id))
    )

    def cb_read_body(response):
        if response.code != 200:
            return
        d = readBody(response)
        d.addCallback(compare_block_num)
    d2.addCallback(cb_read_body)


def get_trx_id_for_link_id(url, link_id):
    agent = Agent(reactor)
    d1 = agent.request(
        bytes('POST', encoding='utf-8'),
        bytes(url + '/v1/evt_link/get_trx_id_for_link_id', encoding='utf-8'),
        bodyProducer=StringProducer('{"link_id": "%s"}' % (link_id))
    )

    def cb_read_body(response):
        if response.code != 200:
            return
        d = readBody(response)
        d.addCallback(get_transaction)
    d1.addCallback(cb_read_body)


def set_irr_block_num(body):
    j = json.loads(str(body, encoding='utf-8'))
    irr_block_num = j['last_irreversible_block_num']
    WatchPool().set_irr_block_num(irr_block_num)


@singleton
class WatchPool:
    def __new__(cls):
        return object.__new__(cls)

    def __init__(self):
        self.watches = set([])
        self.status = True
        self.irr_block_num = 0

    def set_url(self, url):
        self.url = url

    def set_irr_block_num(self, block_num):
        self.irr_block_num = block_num

    def add_watch(self, link_id):
        self.watches.add(link_id)

    def del_watch(self, link_id):
        self.watches.remove(link_id)

    def size(self):
        return len(self.watches)

    def get_irr_block_num(self):
        agent = Agent(reactor)
        d = agent.request(
            bytes('GET', encoding='utf-8'),
            bytes(self.url + '/v1/chain/get_info', encoding='utf-8')
        )

        def cb_read_body(response):
            if response.code != 200:
                return
            d = readBody(response)
            d.addCallback(set_irr_block_num)
        d.addCallback(cb_read_body)

        reactor.callLater(2, self.get_irr_block_num)

    def run(self):
        for each in self.watches:
            get_trx_id_for_link_id(self.url, each)
        reactor.callLater(1, self.run)
