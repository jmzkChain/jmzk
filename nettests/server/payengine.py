import json
import logging
import random
from datetime import datetime

import click

from pyevt import evt_link
from pyevtsdk import api, base
from pyevtsdk.action import ActionGenerator
from pyevtsdk.transaction import TrxGenerator

from twisted.internet import reactor

from watchpool import WatchPool

logger = logging.getLogger()
logger.setLevel(logging.INFO)

AG = ActionGenerator()

class PayEngine:
    def __init__(self, freq, users, sym='5,S#1', nodes=None):
        self.freq = freq
        self.users = self.load_users(users)
        self.linkids = set([])
        self.nodes = nodes
        self.sym = sym
        self.symbol_id = int(sym.split('#')[1])
        self.balance = {}
        DBG_Symbol = base.Symbol('DBG', 666, 5)
        self.asset = base.new_asset(DBG_Symbol)

    def load_users(self, filename):
        with open(filename, 'r') as f:
            tmp = json.load(f)
        return [base.User.from_string(each['pub_key'], each['priv_key']) for each in tmp]

    def set_url(self, url):
        self.Api = api.Api(url)
        self.TG = TrxGenerator(url, payer=None)

    def get_linkids(self):
        return self.linkids

    def fetch_balances(self):
        for user in self.users:
            resp = self.Api.get_assets('''{
            "address": "%s",
            "sym": "%s"
            }''' % (user.pub_key.to_string(), self.sym)).text
            print(resp)
            self.balance[user] = float(resp[2: resp.find(' ')])

    def build_evt_link(self, user):
        link = evt_link.EvtLink()
        link.set_header(5)
        link.set_timestamp(int(datetime.now().timestamp()))
        link.set_max_pay(1000000)
        link.set_symbol_id(self.symbol_id)
        link.set_link_id_rand()
        link.sign(user.priv_key)
        return link

    def run(self):
        [user1, user2] = random.sample(self.users, 2)
        while self.balance[user1] == 0 and self.balance[user2] == 0:
            [user1, user2] = reandom.sample(self.users, 2)

        if self.balance[user1] < self.balance[user2]:
            user1, user2 = user2, user1

        user1_link = self.build_evt_link(user1)
        WatchPool().add_watch(user1_link.get_link_id().hex())

        act_pay = AG.new_action('everipay', payee=user2.pub_key, number=self.asset(1), link=user1_link.to_string())
        trx = self.TG.new_trx()
        trx.add_action(act_pay)
        trx.add_sign(user2.priv_key)
        trx.set_payer(user2.pub_key.to_string())
        resp = self.Api.push_transaction(trx.dumps())
        print(user1_link.get_link_id().hex(), resp, WatchPool().size())

        reactor.callLater(self.freq, self.run)

def prepare_for_debug(filename):
    TG = TrxGenerator('http://127.0.0.1:8888', payer=None)
    Api = api.Api('http://127.0.0.1:8888')
    boss = base.User()
    # issue fungible
    DBG_Symbol = base.Symbol('DBG', 666, 5)
    DBG_Asset = base.new_asset(DBG_Symbol)
    act_newf = AG.new_action('newfungible', name='DBG', sym_name='DBG', sym=DBG_Symbol, creator=boss.pub_key, total_supply=DBG_Asset(1000000))
    trx = TG.new_trx()
    trx.add_action(act_newf)
    trx.add_sign(boss.priv_key)
    trx.set_payer(boss.pub_key.to_string())
    resp = Api.push_transaction(trx.dumps())
    print(resp)

    with open(filename, 'r') as f:
        tmp = json.load(f)
    users = [base.User.from_string(each['pub_key'], each['priv_key']) for each in tmp]

    trx = TG.new_trx()
    for user in users:
        act_issue = AG.new_action('issuefungible', address=base.Address().set_public_key(user.pub_key), number=DBG_Asset(1000), memo='')
        trx.add_action(act_issue)
    trx.add_sign(boss.priv_key)
    trx.set_payer(boss.pub_key.to_string())
    resp = Api.push_transaction(trx.dumps())
    print(resp)

@click.command()
@click.option('--freq', '-f', default=1)
@click.option('--users', '-u', default='payers.json')
@click.option('--symbol', '-s', default='5,S#1')
@click.option('--debug', is_flag=True)
def main(freq, users, symbol, debug):
    if debug:
        prepare_for_debug(users)
        symbol = '5,S#666'

    pe = PayEngine(freq, users, sym=symbol, nodes=None)
    pe.set_url('http://127.0.0.1:8888')
    pe.fetch_balances()
    pe_runid = reactor.callWhenRunning(pe.run)
    WatchPool().set_url('http://127.0.0.1:8888')
    ir_runid = reactor.callWhenRunning(WatchPool().get_irr_block_num)
    wp_runid = reactor.callWhenRunning(WatchPool().run)
    
    
    reactor.run()

if __name__ == '__main__':
    main()
