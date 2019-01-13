#!/usr/bin/env python

import datetime
import json
import os
import random
import string
import subprocess
import sys
import time
import iso8601
import unittest
from concurrent.futures import ThreadPoolExecutor
from io import StringIO

import click
import grequests
from pyevt import abi, ecc, evt_link, libevt
from pyevtsdk import action, api, base, transaction, unit_test


def fake_name(prefix='test'):
    return prefix + ''.join(random.choice(string.hexdigits) for _ in range(8)) + str(int(datetime.datetime.now().timestamp()))[:-5]


def fake_symbol():
    prec = random.randint(3, 10)
    sym_id = random.randint(1000, 1000000000)
    sym_name = ''.join(random.choice(
        string.ascii_letters[26:]) for _ in range(6))
    return sym_name, sym_id, prec


group_json_raw = '''
{
   "name":"5jxX",
   "group":{
      "name":"5jxXg",
      "key":"EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
      "root":{
         "threshold":6,
         "weight":0,
         "nodes":[
            {
               "type":"branch",
               "threshold":1,
               "weight":3,
               "nodes":[
                  {
                     "key":"EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                     "weight":1
                  },
                  {
                     "key":"EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                     "weight":1
                  }
               ]
            },
            {
               "key":"EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
               "weight":3
            },
            {
               "threshold":1,
               "weight":3,
               "nodes":[
                  {
                     "key":"EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                     "weight":1
                  },
                  {
                     "key":"EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                     "weight":2
                  }
               ]
            }
         ]
      }
   }
}
'''

def pre_action():
    newdomain = AG.new_action(
        'newdomain', name=domain_name, creator=user.pub_key)

    issuetoken = AG.new_action('issuetoken', domain=domain_name, names=[
                               token1_name, token2_name, token3_name], owner=[base.Address().set_public_key(user.pub_key)])

    destroytoken = AG.new_action(
        'destroytoken', domain=domain_name, name=token2_name)

    pass_link = evt_link.EvtLink()
    pass_link.set_header(evt_link.HeaderType.version1.value |
                         evt_link.HeaderType.everiPass.value | evt_link.HeaderType.destroy.value)
    pass_link.set_domain(domain_name)
    pass_link.set_token(token3_name)
    pass_link.set_timestamp(int(time.time()))
    pass_link.sign(user.priv_key)

    everipass = AG.new_action(
        'everipass', link=pass_link.to_string())

    group_json = json.loads(group_json_raw)
    group_json['name'] = group_name
    group_json['group']['name'] = group_name
    group_json['group']['key'] = str(user.pub_key)
    newgroup = AG.new_action_from_json('newgroup', json.dumps(group_json))

    symbol = base.Symbol(
        sym_name=sym_name, sym_id=sym_id, precision=sym_prec)
    asset = base.new_asset(symbol)
    newfungible = AG.new_action(
        'newfungible', name=sym_name, sym_name=sym_name, sym=symbol, creator=user.pub_key, total_supply=asset(10000000))

    issuefungible1 = AG.new_action(
        'issuefungible', address=base.Address().set_public_key(user.pub_key), number=asset(100), memo='goodluck')

    issuefungible2 = AG.new_action(
        'issuefungible', address=base.Address().set_public_key(user.pub_key), number=base.EvtAsset(1000), memo='goodluck')

    e2p = AG.new_action(
        'evt2pevt', _from=base.Address().set_public_key(user.pub_key), to=base.Address().set_public_key(user.pub_key), number=base.EvtAsset(100), memo='goodluck')

    pay_link = evt_link.EvtLink()
    pay_link.set_timestamp(int(time.time()))
    pay_link.set_max_pay(999999999)
    pay_link.set_header(evt_link.HeaderType.version1.value |
                        evt_link.HeaderType.everiPay.value)
    pay_link.set_symbol_id(sym_id)
    pay_link.set_link_id_rand()
    pay_link.sign(user.priv_key)

    everipay = AG.new_action('everipay', payee=pub2, number=asset(
        1), link=pay_link.to_string())

    trx = TG.new_trx()
    trx.add_action(issuefungible2)
    trx.add_sign(priv_evt)
    trx.add_sign(user.priv_key)
    trx.set_payer(user.pub_key.to_string())
    resp = api.push_transaction(trx.dumps())
    print(resp)

    trx = TG.new_trx()
    trx.add_action(newdomain)
    trx.add_sign(user.priv_key)
    trx.add_action(newgroup)
    trx.add_action(newfungible)
    trx.set_payer(user.pub_key.to_string())
    resp = api.push_transaction(trx.dumps())
    print(resp)

    trx = TG.new_trx()
    trx.add_action(issuetoken)
    trx.add_action(issuefungible1)
    trx.add_sign(user.priv_key)
    trx.set_payer(user.pub_key.to_string())
    resp = api.push_transaction(trx.dumps())
    print(resp)

    trx = TG.new_trx()
    trx.add_action(issuefungible2)
    trx.add_sign(priv_evt)
    trx.set_payer(user.pub_key.to_string())
    resp = api.push_transaction(trx.dumps())
    print(resp)
    

    trx = TG.new_trx()
    trx.add_action(e2p)
    trx.add_sign(user.priv_key)
    trx.set_payer(user.pub_key.to_string())
    resp = api.push_transaction(trx.dumps())
    print(resp)
   

    trx = TG.new_trx()
    trx.add_action(destroytoken)
    trx.add_action(everipass)
    trx.add_action(everipay)
    trx.add_sign(user.priv_key)
    trx.set_payer(user.pub_key.to_string())
    resp = api.push_transaction(trx.dumps())
    print(resp)


    time.sleep(2)


class Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        libevt.init_lib()

    def _test_evt_link_response(self, resp):
        j = json.loads(resp)
        self.assertTrue('trx_id' in j, msg=j)
        self.assertTrue('block_num' in j, msg=j)

        req = {
            'block_num': j['block_num'],
            'id': j['trx_id']
        }
        resp2 = api.get_transaction(json.dumps(req))
        self.assertEqual(resp2.status_code, 200, msg=resp2.content)

    def test_evt_link_for_trx_id(self):
        symbol = base.Symbol(
            sym_name=sym_name, sym_id=sym_id, precision=sym_prec)
        asset = base.new_asset(symbol)

        pay_link = evt_link.EvtLink()
        pay_link.set_timestamp(int(time.time()))
        pay_link.set_max_pay(999999999)
        pay_link.set_header(evt_link.HeaderType.version1.value |
                            evt_link.HeaderType.everiPay.value)
        pay_link.set_symbol_id(sym_id)
        pay_link.set_link_id_rand()
        pay_link.sign(user.priv_key)

        everipay = AG.new_action('everipay', payee=pub2, number=asset(
            1), link=pay_link.to_string())
        trx = TG.new_trx()
        trx.add_action(everipay)
        trx.add_sign(user.priv_key)
        trx.set_payer(user.pub_key.to_string())
        api.push_transaction(trx.dumps())

        req = {
            'link_id': 'ddc101c51318d51733d682e80b8ea2bc'
        }
        req['link_id'] = pay_link.get_link_id().hex()
        for i in range(1000):
            resp = api.get_trx_id_for_link_id(json.dumps(req)).text
            self._test_evt_link_response(resp)

    def test_evt_link_for_trx_id2(self):
        symbol = base.Symbol(
            sym_name=sym_name, sym_id=sym_id, precision=sym_prec)
        asset = base.new_asset(symbol)

        pay_link = evt_link.EvtLink()
        pay_link.set_timestamp(int(time.time()))
        pay_link.set_max_pay(999999999)
        pay_link.set_header(evt_link.HeaderType.version1.value |
                            evt_link.HeaderType.everiPay.value)
        pay_link.set_symbol_id(sym_id)
        pay_link.set_link_id_rand()
        pay_link.sign(user.priv_key)

        everipay = AG.new_action('everipay', payee=pub2, number=asset(
            1), link=pay_link.to_string())
        trx = TG.new_trx()
        trx.add_action(everipay)
        trx.add_sign(user.priv_key)
        trx.set_payer(user.pub_key.to_string())

        req = {
            'link_id': 'd1680fea21a3c3d8ef555afd8fd8c903'
        }
        req['link_id'] = pay_link.get_link_id().hex()

        def get_response(req):
            return api.get_trx_id_for_link_id(json.dumps(req)).text

        executor = ThreadPoolExecutor(max_workers=2)
        f = executor.submit(get_response, req)

        time.sleep(1)
        api.push_transaction(trx.dumps())

        resp = f.result()
        self._test_evt_link_response(resp)

    def test_evt_link_for_trx_id3(self):
        symbol = base.Symbol(
            sym_name=sym_name, sym_id=sym_id, precision=sym_prec)
        asset = base.new_asset(symbol)
        pay_link = evt_link.EvtLink()
        pay_link.set_timestamp(int(time.time()))
        pay_link.set_max_pay(999999999)
        pay_link.set_header(evt_link.HeaderType.version1.value |
                            evt_link.HeaderType.everiPay.value)
        pay_link.set_symbol_id(sym_id)
        pay_link.set_link_id_rand()
        pay_link.sign(user.priv_key)

        req = {
            'link_id': 'd1680fea21a3c3d8ef555afd8fd8c903'
        }

        url = 'http://127.0.0.1:8888/v1/evt_link/get_trx_id_for_link_id'

        tasks = []
        for i in range(200):
            pay_link.set_link_id_rand()
            req['link_id'] = pay_link.get_link_id().hex()
            tasks.append(grequests.post(url, data=json.dumps(req)))

        for resp in grequests.imap(tasks, size=2000):
            self.assertEqual(resp.status_code, 500, msg=resp.content)

    def test_evt_link_for_trx_id4(self):
        symbol = base.Symbol(
            sym_name=sym_name, sym_id=sym_id, precision=sym_prec)
        asset = base.new_asset(symbol)
        pay_link = evt_link.EvtLink()
        pay_link.set_timestamp(int(time.time()))
        pay_link.set_max_pay(999999999)
        pay_link.set_header(evt_link.HeaderType.version1.value |
                            evt_link.HeaderType.everiPay.value)
        pay_link.set_symbol_id(sym_id)
        pay_link.set_link_id_rand()
        pay_link.sign(user.priv_key)

        req = {
            'link_id': 'd1680fea21a3c3d8ef555afd8fd8c903'
        }

        url = 'http://127.0.0.1:8888/v1/evt_link/get_trx_id_for_link_id'

        tasks = []
        for i in range(10240):
            pay_link.set_link_id_rand()
            req['link_id'] = pay_link.get_link_id().hex()
            tasks.append(grequests.post(url, data=json.dumps(req)))

        i = 0
        for resp in grequests.imap(tasks, size=50):
            self.assertEqual(resp.status_code, 500, msg=resp.content)
            i += 1
            if i % 100 == 0:
                print('Received {} responses'.format(i))

    def test_get_domains(self):
        req = {
            'keys': [user.pub_key.to_string()]
        }
        resp = api.get_domains(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(res_dict[0], domain_name, msg=resp)

    def test_get_tokens(self):

        req = {
            'skip': 0,
            'take': 10
        }
        req['domain'] = domain_name
        resp = api.get_tokens(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 3, msg=resp)
        self.assertEqual(res_dict[0]['name'], token3_name, msg=resp)
        self.assertEqual(res_dict[0]['domain'], domain_name, msg=resp)
        self.assertTrue('owner' in res_dict[0].keys())

    def test_get_history_tokens(self):  

        req = {
            'keys': [user.pub_key.to_string()]
        }
        req['domain'] = domain_name
        req['name'] = token1_name
        resp = api.get_history_tokens(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(res_dict['cookie'][0], token1_name, msg=resp)
        self.assertFalse(token2_name in resp, msg=resp)
        self.assertFalse(token3_name in resp, msg=resp)

        req['domain'] = domain_name
        resp = api.get_history_tokens(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(res_dict['cookie'][0], token1_name, msg=resp)


        req['name'] = token1_name
        resp = api.get_history_tokens(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(res_dict['cookie'][0], token1_name, msg=resp)

    def test_get_groups(self):
        req = {
            'keys': [user.pub_key.to_string()]
        }

        resp = api.get_groups(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(res_dict[0], group_name, msg=resp)

    def test_get_fungibles(self):
        req = {
            'keys': [user.pub_key.to_string()]
        }

        resp = api.get_fungibles(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(res_dict[0], sym_id, msg=resp)

    def test_get_assets(self):
        req = {
            'address': user.pub_key.to_string()
        }

        resp = api.get_assets(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(res_dict[0], '896.63370 S#1', msg=resp)
        self.assertEqual(res_dict[1], '94.57100 S#2', msg=resp)
        self.assertEqual(res_dict[2], '97.00000 S#3', msg=resp)

        req = {
            'address': pub2.to_string()
        }

        resp = api.get_assets(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertTrue(type(res_dict) is list)
        self.assertEqual(res_dict[0], '3.00000 S#3', msg=resp)

    def test_get_actions(self):
        req = {
            'domain': '.fungible',
            'skip': 0,
            'take': 10
        }
        req['domain'] = domain_name

        resp = api.get_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 4, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertTrue('T' in res_dict[0]['timestamp'], msg=res_dict[0]['timestamp'])
        self.assertTrue(type(iso8601.parse_date(res_dict[0]['timestamp'])) is datetime.datetime, msg=res_dict[0]['timestamp'])
        self.assertTrue(token1_name in resp, msg=resp)

        req = {
            'domain': '.fungible',
            'dire': 'asc',
            'skip': 0,
            'take': 10
        }
        req['domain'] = domain_name

        resp = api.get_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 4, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertTrue(token1_name in resp, msg=resp)
        self.assertTrue('timestamp' in resp, msg=resp)

        req = {
            'domain': '.fungible',
            'key': '.issue',
            'skip': 0,
            'take': 10
        }
        req['domain'] = domain_name

        resp = api.get_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 1, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertTrue(token1_name in resp, msg=resp)
        self.assertTrue('timestamp' in resp, msg=resp)

        req = {
            'domain': '.fungible',
            'key': '.issue',
            'dire': 'asc',
            'skip': 0,
            'take': 10
        }
        req['domain'] = domain_name

        resp = api.get_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 1, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertTrue(token1_name in resp, msg=resp)
        self.assertTrue('timestamp' in resp, msg=resp)

        req = {
            'domain': '.fungible',
            'names': [
                'issuetoken'
            ],
            'skip': 0,
            'take': 10
        }
        req['domain'] = domain_name

        resp = api.get_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 1, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertTrue(token1_name in resp, msg=resp)
        self.assertTrue('timestamp' in resp, msg=resp)

        req = {
            'domain': '.fungible',
            'names': [
                'issuetoken'
            ],
            'dire': 'asc',
            'skip': 0,
            'take': 10
        }
        req['domain'] = domain_name

        resp = api.get_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 1, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertTrue(token1_name in resp, msg=resp)
        self.assertTrue('timestamp' in resp, msg=resp)

        req = {
            'domain': '.fungible',
            'names': [
                'issuetoken'
            ],
            'key': '.issue',
            'skip': 0,
            'take': 10
        }
        req['domain'] = domain_name

        resp = api.get_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 1, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertTrue(token1_name in resp, msg=resp)
        self.assertTrue('timestamp' in resp, msg=resp)

        req = {
            'domain': '.fungible',
            'key': 3,
            'names': [
                'everipay'
            ],
            'dire': 'asc',
            'skip': 0,
            'take': 10
        }
        req['key'] = sym_id

        resp = api.get_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 3, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertTrue('everipay' in resp, msg=resp)
        self.assertTrue('timestamp' in resp, msg=resp)

    def test_get_fungible_actions(self):
        req = {
            'sym_id': 338422621,
            'address': 'EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV',
            'dire': 'asc',
            'skip': 0,
            'take': 10
        }
        req['sym_id'] = sym_id

        resp = api.get_fungible_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 4, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertEqual(res_dict[0]['name'], 'issuefungible', msg=resp)
        self.assertTrue(str(sym_id) in resp, msg=resp)
        self.assertTrue('timestamp' in resp, msg=resp)

        req = {
            'sym_id': 338422621,
            'dire': 'asc',
            'skip': 0,
            'take': 10
        }
        req['sym_id'] = sym_id

        resp = api.get_fungible_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 4, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertEqual(res_dict[0]['name'], 'issuefungible', msg=resp)
        self.assertTrue(str(sym_id) in resp, msg=resp)

        req = {
            'sym_id': 338422621,
            'address': 'EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV',
            'skip': 0,
            'take': 10
        }
        req['sym_id'] = sym_id

        resp = api.get_fungible_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 4, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertEqual(res_dict[0]['name'], 'everipay', msg=resp)
        self.assertTrue(str(sym_id) in resp, msg=resp)

        req = {
            'sym_id': 338422621,
            'address': 'EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV',
            'dire': 'asc',
            'skip': 0,
            'take': 10
        }
        req['sym_id'] = sym_id

        resp = api.get_fungible_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 4, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertTrue('issuefungible' in resp, msg=resp)
        self.assertTrue(str(sym_id) in resp, msg=resp)

        req['sym_id'] = 1
        req['address'] = user.pub_key.to_string()
        resp = api.get_fungible_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 2, msg=resp)
        self.assertTrue('trx_id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('name' in res_dict[0].keys(), msg=resp)
        self.assertTrue('domain' in res_dict[0].keys(), msg=resp)
        self.assertTrue('key' in res_dict[0].keys(), msg=resp)
        self.assertTrue('data' in res_dict[0].keys(), msg=resp)
        self.assertTrue('timestamp' in res_dict[0].keys(), msg=resp)
        self.assertTrue('evt2pevt' in resp, msg=resp)

    def test_get_history_transaction(self):

        prodvote = AG.new_action(
        'prodvote', producer='evt', key="global-charge-factor", value=10000)

        trx = TG.new_trx()
        trx.add_action(prodvote)
        trx.add_sign(priv_evt)
        trx.add_sign(user.priv_key)
        trx.set_payer(user.pub_key.to_string())
        resp = api.push_transaction(trx.dumps())
        time.sleep(0.5)

        name = fake_name()
        newdomain = AG.new_action('newdomain', name=name, creator=user.pub_key)

        trx = TG.new_trx()
        trx.add_action(newdomain)
        trx.set_payer(user.pub_key.to_string())
        trx.add_sign(user.priv_key)

        resp = api.push_transaction(data=trx.dumps()).text
        res_dict = json.loads(resp)
        time.sleep(0.5)

        req = {
            'id': 'f0c789933e2b381e88281e8d8e750b561a4d447725fb0eb621f07f219fe2f738'
        }
        req['id'] = res_dict['transaction_id']

        resp = api.get_history_transaction(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(res_dict['transaction']['actions'][0]['name'], 'newdomain', msg=resp)
        self.assertTrue(name in resp, msg=resp)

        resp = api.get_transaction_actions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertEqual(len(res_dict), 2, msg=res_dict)
        self.assertEqual(res_dict[1]['name'], 'paycharge', msg=res_dict)

    def test_get_history_transactions(self):
        req = {
            'keys': [
                'EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK'
            ],
            'skip': 0,
            'take': 10
        }

        resp = api.get_history_transactions(json.dumps(req)).text
        res_dict = json.loads(resp)
        self.assertTrue([] == res_dict, msg=resp)

        req['keys'] = [user.pub_key.to_string()]
        resp = api.get_history_transactions(json.dumps(req)).text
        res_dict = json.loads(resp)

        self.assertEqual(len(res_dict), 9, msg=resp)
        self.assertTrue('id' in res_dict[0].keys(), msg=resp)
        self.assertTrue('signatures' in res_dict[0].keys(), msg=resp)
        self.assertTrue('compression' in res_dict[0].keys(), msg=resp)
        self.assertTrue('packed_trx' in res_dict[0].keys(), msg=resp)
        self.assertTrue('transaction' in res_dict[0].keys(), msg=resp)
        self.assertEqual('.fungible', res_dict[0]['transaction']['actions'][0]['domain'], msg=resp)
        self.assertTrue(group_name in resp, msg=resp)
        self.assertTrue(str(sym_id) in resp, msg=resp)

    def test_get_fungible_ids(self):
        req = {
            'skip': 0,
            'take': 10
        }

        resp = api.get_fungible_ids(json.dumps(req)).text
        self.assertTrue(str(sym_id) in resp, msg=resp)


@click.command()
@click.option('--url', '-u', default='http://127.0.0.1:8888')
@click.option('--start-evtd', '-s', default=True)
@click.option('--evtd-path', '-m', default='/home/laighno/Documents/evt/programs/evtd/evtd')
@click.option('--public-key', '-p', type=str, default='EVT8CAme1QR2664bLQsVfrERBkXL3xEKsALMSfogGavaXFkaVVqR1')
@click.option('--private-key', '-k', type=str, default='5JFZQ7bRRuBJyh42uV5ELwswr2Bt3rfwmyEsyXAoUue18NUreAF')
def main(url, start_evtd, evtd_path, public_key, private_key):
    global evtdout
    evtdout = None

    p = None
    if start_evtd == True:
        evtdout = open('/tmp/evt_api_tests_evtd.log', 'w')

        p = subprocess.Popen([evtd_path, '-e', '--http-validate-host=false',  '--plugin=evt::postgres_plugin',
                              '--plugin=evt::history_plugin', '--plugin=evt::history_api_plugin', '--plugin=evt::chain_api_plugin', '--plugin=evt::evt_api_plugin',
                              '--plugin=evt::evt_link_plugin', '--producer-name=evt', '--delete-all-blocks', '--replay-blockchain', '-d', './tmp', '--postgres-uri=postgresql://postgres@localhost:5432/evt'],
                             stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=evtdout, shell=False)
        
        # wait for evtd to initialize
        time.sleep(3)

    try:
        global domain_name
        domain_name = 'cookie'

        global token1_name, token2_name, token3_name
        token1_name = fake_name('token')
        token2_name = fake_name('token')
        token3_name = 'tpass'

        global group_name
        group_name = fake_name('group')

        global sym_name, sym_id, sym_prec
        sym_name, sym_id, sym_prec = fake_symbol()
        sym_id = 3
        sym_prec = 5

        global pub2, priv2
        pub2, priv2 = ecc.generate_new_pair()

        global evt_pub, priv_evt
        evt_pub = ecc.PublicKey.from_string(
            'EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV')
        priv_evt = ecc.PrivateKey.from_string(
            '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3')

        global TG
        TG = transaction.TrxGenerator(url=url, payer=public_key)

        global user
        user = base.User.from_string(public_key, private_key)

        global api, EvtAsset, AG
        api = api.Api(url)
        EvtAsset = base.EvtAsset
        AG = action.ActionGenerator()

        pre_action()
        suite = unittest.TestLoader().loadTestsFromTestCase(Test)
        runner = unittest.TextTestRunner()
        result = runner.run(suite)
    finally:
        if p is not None:
            p.kill()

        if evtdout is not None:
            evtdout.close()


if __name__ == '__main__':
    main()
