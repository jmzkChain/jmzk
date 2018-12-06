import functools
import json
from datetime import datetime

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

    if not 'block_num' in j:
        if j['error']['code'] == 3190003 and link_info.status[node_info.url]>0:
            link_info.status[node_info.url] = -1
        return

    block_num = j['block_num']

    if link_info.status[node_info.url] == -1:
        print(node_info.url, ' ', link_info.link_id, ' ', int(j['block_id'],16))
    link_info.status[node_info.url] = int(j['block_id'],16)

    if block_num <= node_info.irr_block_num:
        link_info.accept_nodes.add(node_info)

    if len(link_info.accept_nodes) == WatchPool().nodes_num and link_info not in WatchPool().accepts and link_info in WatchPool().watches:
        print('remove %s' % (link_info.link_id))
        WatchPool().accepts.add(link_info)

def get_trx_id_for_link_id(node_info, link_info):
    post_cb(
        url=node_info.url + '/v1/evt_link/get_trx_id_for_link_id',
        callback=compare_block_num,
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
        self.status = {}


@singleton
class WatchPool:
    def __new__(cls):
        return object.__new__(cls)

    def __init__(self):
        self.watches = set([])
        self.accepts = set([])
        self.status = True
        self.irr_block_num = 0
        self.alive = True

    def set_socket(self, socket):
        self.socket = socket

    def set_nodes(self, nodes):
        self.nodes = [NodeInfo(each) for each in nodes]
        self.nodes_num = len(self.nodes)

    def add_watch(self, link_id, timestamp):
        link = LinkInfo(link_id, timestamp)
        for node in self.nodes:
            link.status[node.url] = 0
        self.watches.add(link)

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
        if self.alive:
            reactor.callLater(2, self.get_irr_block_num)

    def check_timeout(self):
        print('check_timeout')
        print(len(self.watches), ' ', len(self.accepts))
        if len(self.watches) == len(self.accepts):
            self.socket.send_string('Success')
            self.stop()
        now = int(datetime.now().timestamp())
        for link_info in self.watches:
            if now > link_info.timestamp + 200:
                self.socket.send_string('wait too long, maybe block can not be checked')
                self.stop()
                return
            if now > link_info.timestamp + 20:
                not_packed = 1
                rolled = 1
                for node in self.nodes:
                    if link_info.status[node.url]>0:
                        not_packed=0
                        rolled=0
                    elif link_info.status[node.url]!=-1:
                        rolled=0

                if rolled:
                    for node in self.nodes:
                        print(link_info.status[node.url])
                    self.socket.send_string('rolled')
                    self.stop()
                    return
                elif not_packed:
                    self.socket.send_string('not packed')
                    self.stop()
                    return

                tmp = link_info.status[self.nodes[0].url]
                for node in self.nodes:
                    if link_info.status[node.url] <= 0:
                        return
                    if link_info.status[node.url]>0 and tmp>0 and link_info.status[node.url]!=tmp:
                        self.socket.send_string('forked!!')
                        self.stop()
                        return
                    if link_info.status[node.url]>0:
                        tmp = link_info.status[node.url]

        if self.alive:
            reactor.callLater(10, self.check_timeout)

    def watch(self):
        for each in self.watches:
            for node in self.nodes:
                get_trx_id_for_link_id(node, each)
        if self.alive:
            reactor.callLater(1, self.watch)

    def run(self):
        self.alive = True
        print('callWhenRunning')
        reactor.callWhenRunning(self.watch)
        reactor.callWhenRunning(self.get_irr_block_num)
        reactor.callWhenRunning(self.check_timeout)

    def stop(self):
        self.alive = False
        self.watches.clear()
        self.accepts.clear()
        print('watch size : ', len(self.watches))
        print('The WatchPool stops now')