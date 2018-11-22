import functools
import json

from twisted.internet import reactor

from utils import post_cb


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


def compare_block_num(body, node_info, link_info):
    j = json.loads(str(body, encoding='utf-8'))
    block_num = j['block_num']

    if block_num <= node_info.irr_block_num:
        link_info.accept_nodes.add(node_info)

    if len(link_info.accept_nodes) == WatchPool().nodes_num:
        print('remove %s' % (link_info.link_id))
        WatchPool().watches.remove(link_info)


def get_transaction(body, node_info, link_info):
    j = json.loads(str(body, encoding='utf-8'))
    block_num = j['block_num']
    trx_id = j['trx_id']

    post_cb(
        url=node_info.url + '/v1/chain/get_transaction',
        callback=compare_block_num,
        args=(node_info, link_info),
        method='POST',
        body='''
        {
            "block_num": %d,
            "id": "%s"
        }
        ''' % (block_num, trx_id)
    )


def get_trx_id_for_link_id(node_info, link_info):
    post_cb(
        url=node_info.url + '/v1/evt_link/get_trx_id_for_link_id',
        callback=get_transaction,
        args=(node_info, link_info),
        method='POST',
        body='{"link_id": "%s"}' % (link_info.link_id)
    )


def set_irr_block_num(body, node):
    j = json.loads(str(body, encoding='utf-8'))
    node.irr_block_num = j['last_irreversible_block_num']


class NodeInfo:
    def __init__(self, url):
        self.url = url
        self.irr_block_num = 0


class LinkInfo:
    def __init__(self, link_id, timestamp):
        self.link_id = link_id
        self.timestamp = timestamp
        self.accept_nodes = set([])
        self.status = True


@singleton
class WatchPool:
    def __new__(cls):
        return object.__new__(cls)

    def __init__(self, nodes=['http://127.0.0.1:8888']):
        self.watches = set([])
        self.status = True
        self.irr_block_num = 0
        self.nodes = [NodeInfo(each) for each in nodes]
        self.nodes_num = len(self.nodes)

    def set_url(self, url):
        self.url = url

    def add_watch(self, link_id, timestamp):
        self.watches.add(LinkInfo(link_id, timestamp))

    def get_node_info_by_url(self, url):
        for node in self.nodes:
            if node.url == url:
                return node
        raise Exception('No Such Node with URL: ' + url)

    def size(self):
        return len(self.watches)

    def get_irr_block_num(self):
        for node in self.nodes:
            post_cb(
                url=node.url + '/v1/chain/get_info',
                callback=set_irr_block_num,
                args=(node,)
            )
        reactor.callLater(2, self.get_irr_block_num)

    def watch(self):
        for each in self.watches:
            for node in self.nodes:
                get_trx_id_for_link_id(node, each)
        reactor.callLater(1, self.watch)

    def run(self):
        reactor.callWhenRunning(self.watch)
        reactor.callWhenRunning(self.get_irr_block_num)
