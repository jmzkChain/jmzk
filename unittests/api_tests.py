#!/usr/bin/python
# -*- coding: UTF-8 -*-

import os
import datetime
import json
import random
import string
import sys
import time
import subprocess

from pyevt import abi, ecc, libevt
from pyevtsdk import action, api, base, transaction, unit_test
import click
import unittest

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
                               token1_name, token2_name], owner=[base.Address().set_public_key(user.pub_key)])

    destroytoken = AG.new_action('destroytoken', domain=domain_name, name=token2_name)

    group_json = json.loads(group_json_raw)
    group_json['name'] = group_name
    group_json['group']['name'] = group_name
    group_json['group']['key'] = str(user.pub_key)
    newgroup = AG.new_action_from_json('newgroup', json.dumps(group_json))

    symbol = base.Symbol(
            sym_name=sym_name, sym_id=sym_id, precision=sym_prec)
    asset = base.new_asset(symbol)
    newfungible = AG.new_action(
        'newfungible', name=sym_name, sym_name=sym_name, sym=symbol, creator=user.pub_key, total_supply=asset(100000))

    trx = TG.new_trx()
    trx.add_action(newdomain)
    trx.add_sign(user.priv_key)
    trx.add_action(newgroup)
    trx.add_action(newfungible)
    api.push_transaction(trx.dumps())

    trx = TG.new_trx()
    trx.add_action(issuetoken)
    trx.add_sign(user.priv_key)
    api.push_transaction(trx.dumps())

    trx = TG.new_trx()
    trx.add_action(destroytoken)
    trx.add_sign(user.priv_key)
    api.push_transaction(trx.dumps())
    time.sleep(2)

class Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        libevt.init_lib()

    def test_get_domains(self):
        req = {
            'keys': [user.pub_key.to_string()]
        }
        resp = api.get_domains(json.dumps(req)).text
        self.assertTrue(domain_name in resp)

    def test_get_tokens(self):
        req = {
            'keys': [user.pub_key.to_string()]
        }
        resp = api.get_tokens(json.dumps(req)).text
        self.assertTrue(token1_name in resp)
        self.assertFalse(token2_name in resp)

    def test_get_groups(self):
        req = {
            'keys': [user.pub_key.to_string()]
        }

        resp = api.get_groups(json.dumps(req)).text
        self.assertTrue(group_name in resp)

    def test_get_fungibles(self):
        req = {
            'keys': [user.pub_key.to_string()]
        }

        resp = api.get_fungibles(json.dumps(req)).text
        self.assertTrue(str(sym_id) in resp)

    def test_get_actions(self):
        req = {
            "domain": ".fungible",
            "key": ".issue",
            "names": [
            "issuetoken"
            ],
            "skip": 0,
            "take": 10
        }
        req["domain"] = domain_name

        resp = api.get_actions(json.dumps(req)).text
        self.assertTrue(token1_name in resp)


    def test_get_fungible_actions(self):
        symbol = base.Symbol(
            sym_name=sym_name, sym_id=sym_id, precision=sym_prec)
        asset = base.new_asset(symbol)
        pub2, priv2 = ecc.generate_new_pair()
        issuefungible = AG.new_action(
            'issuefungible', address=base.Address().set_public_key(pub2), number=asset(100), memo='goodluck')
        trx = TG.new_trx()
        trx.add_action(issuefungible)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())
        time.sleep(0.5)

        req = {
             "sym_id": 338422621,
             "address": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
             "skip": 0,
             "take": 10
        }
        req["sym_id"] = sym_id

        resp = api.get_fungible_actions(json.dumps(req)).text
        self.assertTrue("issuefungible" in resp)
        self.assertTrue(str(sym_id) in resp)

    def test_get_transaction(self):
        name = fake_name()
        newdomain = AG.new_action('newdomain', name=name, creator=user.pub_key)

        trx = TG.new_trx()
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)

        resp = api.push_transaction(data=trx.dumps()).text
        res_dict = json.loads(resp)
        time.sleep(0.5)

        req = {
             "id": "f0c789933e2b381e88281e8d8e750b561a4d447725fb0eb621f07f219fe2f738"
        }
        req["id"] = res_dict["transaction_id"]

        resp = api.get_transaction(json.dumps(req)).text
        self.assertTrue("newdomain" in resp)
        self.assertTrue(name in resp)

    def test_get_transactions(self):
        req = {
             "keys": [
                 "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
                 "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                 "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"
              ],
              "skip": 0,
              "take": 10
        }
        req["keys"] = [user.pub_key.to_string()]
        
        resp = api.get_transactions(json.dumps(req)).text
        self.assertTrue(domain_name in resp)
        self.assertTrue(token1_name in resp)
        self.assertTrue(group_name in resp)
        self.assertTrue(str(sym_id) in resp)
        

@click.command()
@click.option('--url', '-u', default='http://127.0.0.1:8888')
@click.option('--evtd-path', '-m', default='/home/laighno/Documents/evt/programs/evtd/evtd')
@click.option('--public-key', '-p', type=str, prompt='Public Key')
@click.option('--private-key', '-k', type=str, prompt='Private Key')
def main(url, evtd_path, public_key, private_key):
    p = subprocess.Popen(["nohup", evtd_path, "-e", "--http-validate-host=false", "--charge-free-mode", "--plugin=evt::mongo_db_plugin", "--plugin=evt::history_plugin", "--plugin=evt::history_api_plugin", "--plugin=evt::chain_api_plugin", "--plugin=evt::evt_api_plugin", "--producer-name=evt", "--delete-all-blocks", "-d", "/tmp/evt", "-m", 'mongodb://127.0.0.1:27017','&'], stdout=None, shell=False)
    time.sleep(3)

    global domain_name 
    domain_name = fake_name()
    global token1_name, token2_name
    token1_name = fake_name('token')
    token2_name = fake_name('token')
    global group_name
    group_name = fake_name('group')
    global sym_name, sym_id, sym_prec
    sym_name, sym_id, sym_prec = fake_symbol()

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
    runner.run(suite)

    p.kill()


if __name__ == '__main__':
    main()
