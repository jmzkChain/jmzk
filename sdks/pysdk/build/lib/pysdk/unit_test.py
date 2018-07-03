import datetime
import json
import random
import string
import time
import unittest

from pyevt import abi, ecc, libevt

import action
import api
import base
import transaction

host_url = 'http://118.31.58.10:8888'
api = api.Api(host_url)
EvtAsset = base.EvtAsset
AG = action.ActionGenerator()


def fake_name(prefix='test'):
    return prefix + str(int(datetime.datetime.now().timestamp()))


def fake_symbol():
    prec = random.randint(3, 10)
    name = ''.join(random.choice(string.ascii_letters[26:]) for _ in range(6))
    return name, prec


class Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        libevt.init_lib()

    def test_action_newdomain(self):
        pub_key, priv_key = ecc.generate_new_pair()
        name = fake_name()

        newdomain = AG.new_action('newdomain', name=name, creator=pub_key)

        trx = transaction.Transaction(host_url)
        trx.add_action(newdomain)
        trx.add_sign(priv_key)

        api.push_transaction(data=trx.dumps())

        resp = api.get_domain('''{"name": "%s"}''' % (name)).json()
        self.assertTrue(pub_key.to_string() == resp['creator'])
        time.sleep(1)

    def test_action_updatedomain(self):
        # Example: Add another user into issue permimssion

        # Create a new domain
        pub_key, priv_key = ecc.generate_new_pair()
        name = fake_name()
        newdomain = AG.new_action('newdomain', name=name, creator=pub_key)
        trx = transaction.Transaction(host_url)
        trx.add_action(newdomain)
        trx.add_sign(priv_key)
        api.push_transaction(data=trx.dumps())

        # Create manage Permission with both user
        pub_key2, priv_key2 = ecc.generate_new_pair()  # another user
        ar1 = base.AuthorizerRef('A', pub_key)
        ar2 = base.AuthorizerRef('A', pub_key2)
        issue_per = base.PermissionDef('issue', threshold=2)
        issue_per.add_authorizer(ar1, weight=1)
        issue_per.add_authorizer(ar2, weight=1)

        updatedomain = AG.new_action(
            'updatedomain', name=name, issue=issue_per)
        trx = transaction.Transaction(host_url)
        trx.add_action(updatedomain)
        trx.add_sign(priv_key)
        api.push_transaction(data=trx.dumps())

        resp = api.get_domain('{"name": "%s"}' % (name)).json()
        self.assertTrue({'ref': ar2.value(), 'weight': 1}
                        in resp['issue']['authorizers'])
        time.sleep(1)

    def test_action_newgroup(self):
        pub_key, priv_key = ecc.generate_new_pair()
        name = fake_name('group')
        with open('group.json', 'r') as f:
            group_json = json.load(f)
        group_json['name'] = name
        group_json['group']['name'] = name
        group_json['group']['key'] = str(pub_key)

        newgroup = AG.new_action_from_json('newgroup', json.dumps(group_json))

        trx = transaction.Transaction(host_url)
        trx.add_action(newgroup)
        trx.add_sign(priv_key)

        api.push_transaction(data=trx.dumps()).text

        resp = api.get_group('''{"name": "%s"}''' % (name)).json()
        self.assertTrue(pub_key.to_string() == resp['key'])
        time.sleep(1)

    def test_action_updategroup(self):
        # It's the same as new group action
        pass

    def test_action_issuetoken(self):
        pub_key, priv_key = ecc.generate_new_pair()
        domain_name = fake_name()
        token_name = fake_name('token')

        newdomain = AG.new_action(
            'newdomain', name=domain_name, creator=pub_key)
        issuetoken = AG.new_action('issuetoken', domain=domain_name, names=[
                                   token_name], owner=[pub_key])

        trx = transaction.Transaction(host_url)
        trx.add_action(newdomain)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        trx = transaction.Transaction(host_url)
        trx.add_action(issuetoken)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        resp = api.get_token(
            '''
            {
                "name": "%s",
                "domain": "%s"
            }
                
        ''' % (token_name, domain_name)).json()
        self.assertTrue(resp['domain'] == domain_name)
        time.sleep(1)

    def test_action_transfer(self):
        pub_key, priv_key = ecc.generate_new_pair()
        domain_name = fake_name()
        token_name = fake_name('token')

        newdomain = AG.new_action(
            'newdomain', name=domain_name, creator=pub_key)
        issuetoken = AG.new_action('issuetoken', domain=domain_name, names=[
                                   token_name], owner=[pub_key])

        # create a new domain
        trx = transaction.Transaction(host_url)
        trx.add_action(newdomain)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        # issue a new token
        trx = transaction.Transaction(host_url)
        trx.add_action(issuetoken)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        # transfer the token to pub2
        pub2, priv2 = ecc.generate_new_pair()
        transfer = AG.new_action(
            'transfer', domain=domain_name, name=token_name, to=[pub2], memo='haha')
        trx = transaction.Transaction(host_url)
        trx.add_action(transfer)
        trx.add_sign(priv_key)  # priv_key of the owner of this token
        api.push_transaction(trx.dumps())

        resp = api.get_token(
            '''
            {
                "name": "%s",
                "domain": "%s"
            }
                
        ''' % (token_name, domain_name)).json()
        self.assertTrue(resp['owner'][0] == pub2.to_string())
        time.sleep(1)

    def test_action_addmeta(self):
        pub_key, priv_key = ecc.generate_new_pair()
        # create a new domain
        domain_name = fake_name()
        newdomain = AG.new_action(
            'newdomain', name=domain_name, creator=pub_key)
        trx = transaction.Transaction(host_url)
        trx.add_action(newdomain)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        # add meta to this domain
        addmeta = AG.new_action('addmeta', meta_key='meta-key-test',
                                meta_value='meta-value-test', creator=pub_key, domain='domain', key=domain_name)
        trx = transaction.Transaction(host_url)
        trx.add_action(addmeta)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())
        time.sleep(1)

    def test_action_newfungible(self):
        pub_key, priv_key = ecc.generate_new_pair()
        sym_name, sym_prec = fake_symbol()
        symbol = base.Symbol(name=sym_name, precision=sym_prec)
        asset = base.new_asset(symbol)
        newfungible = AG.new_action(
            'newfungible', sym=symbol, creator=pub_key, total_supply=asset(100000))

        trx = transaction.Transaction(host_url)
        trx.add_action(newfungible)
        trx.add_sign(priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_updfungible(self):
        # create a new fungible
        pub_key, priv_key = ecc.generate_new_pair()
        sym_name, sym_prec = fake_symbol()
        symbol = base.Symbol(name=sym_name, precision=sym_prec)
        asset = base.new_asset(symbol)
        newfungible = AG.new_action(
            'newfungible', sym=symbol, creator=pub_key, total_supply=asset(100000))

        trx = transaction.Transaction(host_url)
        trx.add_action(newfungible)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        # add pub2 into manage permission
        pub2, priv2 = ecc.generate_new_pair()
        ar1 = base.AuthorizerRef('A', pub_key)
        ar2 = base.AuthorizerRef('A', pub2)
        manage = base.PermissionDef('manage', threshold=2)
        manage.add_authorizer(ar1, weight=1)
        manage.add_authorizer(ar2, weight=1)

        updfungible = AG.new_action('updfungible', sym=symbol, manage=manage)
        trx = transaction.Transaction(host_url)
        trx.add_action(updfungible)
        trx.add_sign(priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_issuefungible(self):
        # Create a fungible, and then use the issue fungible with the asset
        pub_key, priv_key = ecc.generate_new_pair()
        sym_name, sym_prec = fake_symbol()
        symbol = base.Symbol(name=sym_name, precision=sym_prec)
        asset = base.new_asset(symbol)
        newfungible = AG.new_action(
            'newfungible', sym=symbol, creator=pub_key, total_supply=asset(100000))

        trx = transaction.Transaction(host_url)
        trx.add_action(newfungible)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        pub2, priv2 = ecc.generate_new_pair()
        issuefungible = AG.new_action(
            'issuefungible', address=pub2, number=asset(100), memo='goodluck')
        trx = transaction.Transaction(host_url)
        trx.add_action(issuefungible)
        trx.add_sign(priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_transferft(self):
        # Create a fungible and issue
        pub_key, priv_key = ecc.generate_new_pair()
        sym_name, sym_prec = fake_symbol()
        symbol = base.Symbol(name=sym_name, precision=sym_prec)
        asset = base.new_asset(symbol)
        newfungible = AG.new_action(
            'newfungible', sym=symbol, creator=pub_key, total_supply=asset(100000))

        trx = transaction.Transaction(host_url)
        trx.add_action(newfungible)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        issuefungible = AG.new_action(
            'issuefungible', address=pub_key, number=asset(100), memo='goodluck')
        trx = transaction.Transaction(host_url)
        trx.add_action(issuefungible)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        # Transfer asset to pub2
        pub2, priv2 = ecc.generate_new_pair()
        transferft = AG.new_action(
            'transferft', _from=pub_key, to=pub2, number=asset(1), memo='goodjob')
        trx = transaction.Transaction(host_url)
        trx.add_action(transferft)
        trx.add_sign(priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)


if __name__ == '__main__':
    unittest.main()
