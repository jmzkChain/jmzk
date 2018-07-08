import datetime
import json
import random
import string
import sys
import unittest

from pyevt import abi, ecc, libevt

from . import action, api, base, transaction

host_url = sys.argv[-1] if len(sys.argv) > 1 else 'http://118.31.58.10:8888'
api = api.Api(host_url)
EvtAsset = base.EvtAsset
AG = action.ActionGenerator()


def fake_name(prefix='test'):
    return prefix + ''.join(random.choice(string.hexdigits) for _ in range(8)) + str(int(datetime.datetime.now().timestamp()))[:-5]


def fake_symbol():
    prec = random.randint(3, 10)
    name = ''.join(random.choice(string.ascii_letters[26:]) for _ in range(6))
    return name, prec


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


class Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        libevt.init_lib()

    def test_action_newdomain(self):
        pub_key, priv_key = ecc.generate_new_pair()
        name = fake_name()

        newdomain = AG.new_action('newdomain', name=name, creator=pub_key)

        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newdomain)
        trx.add_sign(priv_key)

        api.push_transaction(data=trx.dumps())

        resp = api.get_domain('''{"name": "%s"}''' % (name)).json()
        self.assertTrue(pub_key.to_string() == resp['creator'])

    def test_action_updatedomain(self):
        # Example: Add another user into issue permimssion

        # Create a new domain
        pub_key, priv_key = ecc.generate_new_pair()
        name = fake_name()
        newdomain = AG.new_action('newdomain', name=name, creator=pub_key)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
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

        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(updatedomain)
        trx.add_sign(priv_key)
        api.push_transaction(data=trx.dumps())

        resp = api.get_domain('{"name": "%s"}' % (name)).json()
        self.assertTrue({'ref': ar2.value(), 'weight': 1}
                        in resp['issue']['authorizers'])

    def test_action_newgroup(self):
        pub_key, priv_key = ecc.generate_new_pair()
        name = fake_name('group')
        group_json = json.loads(group_json_raw)
        group_json['name'] = name
        group_json['group']['name'] = name
        group_json['group']['key'] = str(pub_key)

        newgroup = AG.new_action_from_json('newgroup', json.dumps(group_json))

        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newgroup)
        trx.add_sign(priv_key)

        api.push_transaction(data=trx.dumps()).text

        resp = api.get_group('''{"name": "%s"}''' % (name)).json()
        self.assertTrue(pub_key.to_string() == resp['key'])

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

        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newdomain)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        trx = transaction.Transaction()
        trx.set_header(url=host_url)
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

    def test_action_transfer(self):
        pub_key, priv_key = ecc.generate_new_pair()
        domain_name = fake_name()
        token_name = fake_name('token')

        newdomain = AG.new_action(
            'newdomain', name=domain_name, creator=pub_key)
        issuetoken = AG.new_action('issuetoken', domain=domain_name, names=[
                                   token_name], owner=[pub_key])

        # create a new domain
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newdomain)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        # issue a new token
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(issuetoken)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        # transfer the token to pub2
        pub2, priv2 = ecc.generate_new_pair()
        transfer = AG.new_action(
            'transfer', domain=domain_name, name=token_name, to=[pub2], memo='haha')
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
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

    def test_action_addmeta(self):
        pub_key, priv_key = ecc.generate_new_pair()
        # create a new domain
        domain_name = fake_name()
        newdomain = AG.new_action(
            'newdomain', name=domain_name, creator=pub_key)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newdomain)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        ar = base.AuthorizerRef('A', pub_key)

        # add meta to this domain
        addmeta = AG.new_action('addmeta', meta_key='meta-key-test',
                                meta_value='meta-value-test', creator=ar, domain='domain', key=domain_name)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(addmeta)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

    def test_action_newfungible(self):
        pub_key, priv_key = ecc.generate_new_pair()
        sym_name, sym_prec = fake_symbol()
        symbol = base.Symbol(name=sym_name, precision=sym_prec)
        asset = base.new_asset(symbol)
        newfungible = AG.new_action(
            'newfungible', sym=symbol, creator=pub_key, total_supply=asset(100000))

        trx = transaction.Transaction()
        trx.set_header(url=host_url)
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

        trx = transaction.Transaction()
        trx.set_header(url=host_url)
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
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
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

        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newfungible)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        pub2, priv2 = ecc.generate_new_pair()
        issuefungible = AG.new_action(
            'issuefungible', address=pub2, number=asset(100), memo='goodluck')
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
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

        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newfungible)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        issuefungible = AG.new_action(
            'issuefungible', address=pub_key, number=asset(100), memo='goodluck')
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(issuefungible)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        # Transfer asset to pub2
        pub2, priv2 = ecc.generate_new_pair()
        transferft = AG.new_action(
            'transferft', _from=pub_key, to=pub2, number=asset(1), memo='goodjob')
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(transferft)
        trx.add_sign(priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_newdelay(self):
        user = base.User()
        # Create a trx with newdomain action
        newdomain = AG.new_action(
            'newdomain', name=fake_name(), creator=user.pub_key)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)

        newdelay = AG.new_action(
            'newdelay', name=fake_name(), proposer=user.pub_key, trx=trx)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newdelay)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_approvedelay(self):
        user = base.User()
        newdomain = AG.new_action(
            'newdomain', name=fake_name(), creator=user.pub_key)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)
        signatures = json.loads(trx.dumps())['signatures']

        delay_name = fake_name()
        newdelay = AG.new_action(
            'newdelay', name=delay_name, proposer=user.pub_key, trx=trx)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newdelay)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        approve = AG.new_action(
            'approvedelay', name=delay_name, signatures=signatures)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(approve)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_canceldelay(self):
        user = base.User()
        newdomain = AG.new_action(
            'newdomain', name=fake_name(), creator=user.pub_key)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)

        delay_name = fake_name()
        newdelay = AG.new_action(
            'newdelay', name=delay_name, proposer=user.pub_key, trx=trx)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newdelay)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        canceldelay = AG.new_action('canceldelay', name=delay_name)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(canceldelay)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)

    def test_action_executedelay(self):
        user = base.User()
        newdomain = AG.new_action(
            'newdomain', name=fake_name(), creator=user.pub_key)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newdomain)
        trx.add_sign(user.priv_key)
        signatures = json.loads(trx.dumps())['signatures']

        delay_name = fake_name()
        newdelay = AG.new_action(
            'newdelay', name=delay_name, proposer=user.pub_key, trx=trx)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(newdelay)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        approve = AG.new_action(
            'approvedelay', name=delay_name, signatures=signatures)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(approve)
        trx.add_sign(user.priv_key)
        api.push_transaction(trx.dumps())

        executedelay = AG.new_action(
            'executedelay', name=delay_name, executor=user.pub_key)
        trx = transaction.Transaction()
        trx.set_header(url=host_url)
        trx.add_action(executedelay)
        trx.add_sign(user.priv_key)
        resp = api.push_transaction(trx.dumps()).text
        self.assertTrue('transaction_id' in resp)


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(Test)
    runner = unittest.TextTestRunner()
    runner.run(suite)
