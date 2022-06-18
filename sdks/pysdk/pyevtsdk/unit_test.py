import datetime
import json
import random
import string
import sys
import time
import unittest

import click

from pyjmzk import abi, ecc, libjmzk

from . import action, api, base, transaction


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
      "key":"jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
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
                     "key":"jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                     "weight":1
                  },
                  {
                     "key":"jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                     "weight":1
                  }
               ]
            },
            {
               "key":"jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
               "weight":3
            },
            {
               "threshold":1,
               "weight":3,
               "nodes":[
                  {
                     "key":"jmzk6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                     "weight":1
                  },
                  {
                     "key":"jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                     "weight":2
                  }
               ]
            }
         ]
      }
   }
}
'''


class Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        libjmzk.init_lib()

    def test_action_newdomain(self):
        name = fake_name()

        newdomain = AG.new_action('newdomain', name=name, creator=user.pub_key)

        trx = TG.new_trx()
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)

        resp = api.push_transaction(data=trx.dumps())

        self.assertTrue('transaction_id' in resp.text)

    def test_action_updatedomain(self):
        # Example: Add another user into issue permimssion

        # Create a new domain
        name = fake_name()
        newdomain = AG.new_action('newdomain', name=name, creator=user.pub_key)
        trx = TG.new_trx()
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)
        api.push_transaction(data=trx.dumps())

        # Create manage Permission with both user
        pub_key2, priv_key2 = ecc.generate_new_pair()  # another user
        ar1 = base.AuthorizerRef('A', user.pub_key)
        ar2 = base.AuthorizerRef('A', pub_key2)
        issue_per = base.PermissionDef('issue', threshold=2)
        issue_per.add_authorizer(ar1, weight=1)
        issue_per.add_authorizer(ar2, weight=1)

        updatedomain = AG.new_action(
            'updatedomain', name=name, issue=issue_per)

        trx = TG.new_trx()
        trx.add_action(updatedomain)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(data=trx.dumps())

        self.assertTrue('transaction_id' in resp.text)

    def test_action_newgroup(self):
        name = fake_name('group')
        group_json = json.loads(group_json_raw)
        group_json['name'] = name
        group_json['group']['name'] = name
        group_json['group']['key'] = str(user.pub_key)

        newgroup = AG.new_action_from_json('newgroup', json.dumps(group_json))

        trx = TG.new_trx()
        trx.add_action(newgroup)
        trx.add_sign(user.priv_key)

        resp = api.push_transaction(data=trx.dumps())

        self.assertTrue('transaction_id' in resp.text)

    def test_action_updategroup(self):
        # It's the same as new group action
        pass

    def test_action_issuetoken(self):
        domain_name = fake_name()
        token_name = fake_name('token')

        newdomain = AG.new_action(
            'newdomain', name=domain_name, creator=user.pub_key)
        issuetoken = AG.new_action('issuetoken', domain=domain_name, names=[
                                   token_name], owner=[base.Address().set_public_key(user.pub_key)])

        trx = TG.new_trx()
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        trx = TG.new_trx()
        trx.add_action(issuetoken)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps())

        self.assertTrue('transaction_id' in resp.text)

    def test_action_transfer(self):
        domain_name = fake_name()
        token_name = fake_name('token')

        newdomain = AG.new_action(
            'newdomain', name=domain_name, creator=user.pub_key)
        issuetoken = AG.new_action('issuetoken', domain=domain_name, names=[
                                   token_name], owner=[base.Address().set_public_key(user.pub_key)])

        # create a new domain
        trx = TG.new_trx()
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        # issue a new token
        trx = TG.new_trx()
        trx.add_action(issuetoken)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        # transfer the token to pub2
        pub2, priv2 = ecc.generate_new_pair()
        transfer = AG.new_action(
            'transfer', domain=domain_name, name=token_name, to=[pub2], memo='haha')
        trx = TG.new_trx()
        trx.add_action(transfer)
        trx.add_sign(user.priv_key)  # priv_key of the owner of this token
        resp = api.push_transaction(trx.dumps())

        self.assertTrue('transaction_id' in resp.text)

    def test_action_addmeta(self):
        # create a new domain
        domain_name = fake_name()
        newdomain = AG.new_action(
            'newdomain', name=domain_name, creator=user.pub_key)
        trx = TG.new_trx()
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        ar = base.AuthorizerRef('A', user.pub_key)

        # add meta to this domain
        addmeta = AG.new_action('addmeta', meta_key='meta-key-test',
                                meta_value='meta-value-test', creator=ar, domain='domain', key=domain_name)
        trx = TG.new_trx()
        trx.add_action(addmeta)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

    def test_action_newfungible(self):
        sym_name, sym_id, sym_prec = fake_symbol()
        symbol = base.Symbol(
            sym_name=sym_name, sym_id=sym_id, precision=sym_prec)
        asset = base.new_asset(symbol)
        newfungible = AG.new_action(
            'newfungible', name=sym_name, sym_name=sym_name, sym=symbol, creator=user.pub_key, total_supply=asset(100000))

        trx = TG.new_trx()
        trx.add_action(newfungible)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_updfungible(self):
        # create a new fungible
        sym_name, sym_id, sym_prec = fake_symbol()
        symbol = base.Symbol(
            sym_name=sym_name, sym_id=sym_id, precision=sym_prec)
        asset = base.new_asset(symbol)
        newfungible = AG.new_action(
            'newfungible', name=sym_name, sym_name=sym_name, sym=symbol, creator=user.pub_key, total_supply=asset(100000))

        trx = TG.new_trx()
        trx.add_action(newfungible)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        # add pub2 into manage permission
        pub2, priv2 = ecc.generate_new_pair()
        ar1 = base.AuthorizerRef('A', user.pub_key)
        ar2 = base.AuthorizerRef('A', pub2)
        manage = base.PermissionDef('manage', threshold=2)
        manage.add_authorizer(ar1, weight=1)
        manage.add_authorizer(ar2, weight=1)

        updfungible = AG.new_action(
            'updfungible', sym_id=str(sym_id), manage=manage)
        trx = TG.new_trx()
        trx.add_action(updfungible)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_issuefungible(self):
        # Create a fungible, and then use the issue fungible with the asset
        sym_name, sym_id, sym_prec = fake_symbol()
        symbol = base.Symbol(
            sym_name=sym_name, sym_id=sym_id, precision=sym_prec)
        asset = base.new_asset(symbol)
        newfungible = AG.new_action(
            'newfungible', name=sym_name, sym_name=sym_name, sym=symbol, creator=user.pub_key, total_supply=asset(100000))

        trx = TG.new_trx()
        trx.add_action(newfungible)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        pub2, priv2 = ecc.generate_new_pair()
        issuefungible = AG.new_action(
            'issuefungible', address=base.Address().set_public_key(pub2), number=asset(100), memo='goodluck')
        trx = TG.new_trx()
        trx.add_action(issuefungible)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_transferft(self):
        # Create a fungible and issue
        sym_name, sym_id, sym_prec = fake_symbol()
        symbol = base.Symbol(
            sym_name=sym_name, sym_id=sym_id, precision=sym_prec)
        asset = base.new_asset(symbol)
        newfungible = AG.new_action(
            'newfungible', name=sym_name, sym_name=sym_name, sym=symbol, creator=user.pub_key, total_supply=asset(100000))

        trx = TG.new_trx()
        trx.add_action(newfungible)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        issuefungible = AG.new_action(
            'issuefungible', address=user.pub_key, number=asset(100), memo='goodluck')
        trx = TG.new_trx()
        trx.add_action(issuefungible)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        # Transfer asset to pub2
        pub2, priv2 = ecc.generate_new_pair()
        transferft = AG.new_action(
            'transferft', _from=user.pub_key, to=pub2, number=asset(1), memo='goodjob')
        trx = TG.new_trx()
        trx.add_action(transferft)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_newsuspend(self):
        # Create a trx with newdomain action
        newdomain = AG.new_action(
            'newdomain', name=fake_name(), creator=user.pub_key)
        trx = TG.new_trx()
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)

        newsuspend = AG.new_action(
            'newsuspend', name=fake_name(), proposer=user.pub_key, trx=trx)
        trx = TG.new_trx()
        trx.add_action(newsuspend)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_aprvsuspend(self):
        newdomain = AG.new_action(
            'newdomain', name=fake_name(), creator=user.pub_key)
        trx = TG.new_trx()
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)
        signatures = json.loads(trx.dumps())['signatures']

        suspend_name = fake_name()
        newsuspend = AG.new_action(
            'newsuspend', name=suspend_name, proposer=user.pub_key, trx=trx)
        trx = TG.new_trx()
        trx.add_action(newsuspend)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        aprvsuspend = AG.new_action(
            'aprvsuspend', name=suspend_name, signatures=signatures)
        trx = TG.new_trx()
        trx.add_action(aprvsuspend)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_cancelsuspend(self):
        newdomain = AG.new_action(
            'newdomain', name=fake_name(), creator=user.pub_key)
        trx = TG.new_trx()
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)

        suspend_name = fake_name()
        newsuspend = AG.new_action(
            'newsuspend', name=suspend_name, proposer=user.pub_key, trx=trx)
        trx = TG.new_trx()
        trx.add_action(newsuspend)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        cancelsuspend = AG.new_action('cancelsuspend', name=suspend_name)
        trx = TG.new_trx()
        trx.add_action(cancelsuspend)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_execsuspend(self):
        newdomain = AG.new_action(
            'newdomain', name=fake_name(), creator=user.pub_key)
        trx = TG.new_trx()
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)
        signatures = json.loads(trx.dumps())['signatures']

        suspend_name = fake_name()
        newsuspend = AG.new_action(
            'newsuspend', name=suspend_name, proposer=user.pub_key, trx=trx)
        trx = TG.new_trx()
        trx.add_action(newsuspend)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        aprvsuspend = AG.new_action(
            'aprvsuspend', name=suspend_name, signatures=signatures)
        trx = TG.new_trx()
        trx.add_action(aprvsuspend)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        execsuspend = AG.new_action(
            'execsuspend', name=suspend_name, executor=user.pub_key)
        trx = TG.new_trx()
        trx.add_action(execsuspend)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)


@click.command()
@click.option('--url', '-u', default='http://127.0.0.1:8888')
@click.option('--public-key', '-p', type=str, prompt='Public Key')
@click.option('--private-key', '-k', type=str, prompt='Private Key')
def main(url, public_key, private_key):
    global TG
    TG = transaction.TrxGenerator(url=url, payer=public_key)
    global user
    user = base.User.from_string(public_key, private_key)
    global api, jmzkAsset, AG
    api = api.Api(url)
    jmzkAsset = base.jmzkAsset
    AG = action.ActionGenerator()

    suite = unittest.TestLoader().loadTestsFromTestCase(Test)
    runner = unittest.TextTestRunner()
    runner.run(suite)


if __name__ == '__main__':
    main()
