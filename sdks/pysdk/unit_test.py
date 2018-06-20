import datetime
import json
import unittest

from pyevt import abi, ecc, libevt

import action
import api
import base
import transaction

api = api.Api()
EvtAsset = base.EvtAsset
AG = action.ActionGenerator()


def fake_name(prefix='test'):
    return prefix + str(int(datetime.datetime.now().timestamp()))


class Test(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        libevt.init_lib()

    def test_action_newdomain(self):
        pub_key, priv_key = ecc.generate_new_pair()
        name = fake_name()

        newdomain = AG.new_action('newdomain', name=name, issuer=pub_key)

        trx = transaction.Transaction()
        trx.add_action(newdomain)
        trx.add_sign(priv_key)

        api.push_transaction(data=trx.dumps())

        resp = api.get_domain('''{"name": "%s"}''' % (name)).json()
        self.assertTrue(pub_key.to_string() == resp['issuer'])

    def test_action_updatedomain(self):
        # Example: Add another user into manage permimssion

        # Create a new domain
        pub_key, priv_key = ecc.generate_new_pair()
        name = fake_name()
        newdomain = AG.new_action('newdomain', name=name, issuer=pub_key)
        trx = transaction.Transaction()
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
        trx.add_action(updatedomain)
        trx.add_sign(priv_key)
        api.push_transaction(data=trx.dumps())

        resp = api.get_domain('{"name": "%s"}' % (name)).json()
        self.assertTrue({'ref': ar2.value(), 'weight': 1}
                        in resp['issue']['authorizers'])

    def test_action_newgroup(self):
        pub_key, priv_key = ecc.generate_new_pair()
        name = fake_name('group')
        with open('group.json', 'r') as f:
            group_json = json.load(f)
        group_json['name'] = name
        group_json['group']['name'] = name
        group_json['group']['key'] = str(pub_key)

        newgroup = AG.new_action_from_json('newgroup', json.dumps(group_json))

        trx = transaction.Transaction()
        trx.add_action(newgroup)
        trx.add_sign(priv_key)

        api.push_transaction(data=trx.dumps()).text

        resp = api.get_group('''{"name": "%s"}''' % (name)).json()
        self.assertTrue(pub_key.to_string() == resp['key'])

    def test_action_updategroup(self):
        # It's the same as new group action
        pass

    def test_action_newaccount(self):
        pub1, priv1 = ecc.generate_new_pair()
        pub2, priv2 = ecc.generate_new_pair()
        name = fake_name('account')

        newaccount = AG.new_action('newaccount', name=name, owner=[pub1, pub2])

        trx = transaction.Transaction()
        trx.add_action(newaccount)
        trx.add_sign(priv1)
        trx.add_sign(priv2)

        api.push_transaction(data=trx.dumps())

        resp = api.get_account('''{"name": "%s"}''' % (name)).json()
        self.assertTrue(resp['name'] == name)

    def test_action_updateowner(self):
        pub1, priv1 = ecc.generate_new_pair()
        pub2, priv2 = ecc.generate_new_pair()
        name = fake_name('account')

        # create a new account. Owner is pub1
        newaccount = AG.new_action('newaccount', name=name, owner=[pub1])
        trx = transaction.Transaction()
        trx.add_action(newaccount)
        trx.add_sign(priv1)
        api.push_transaction(trx.dumps())

        # update the owner, change the owner to be [pub1,pub2]
        updateowner = AG.new_action(
            'updateowner', name=name, owner=[pub1, pub2])
        trx = transaction.Transaction()
        trx.add_action(updateowner)
        trx.add_sign(priv1)  # sign with only original owner's priv_key
        resp = api.push_transaction(trx.dumps())

        resp = api.get_account('''{"name": "%s"}''' % (name)).json()
        self.assertTrue(pub2.to_string() in resp['owner'])

    def test_action_transferevt(self):
        pub1, priv1 = ecc.generate_new_pair()
        account1 = fake_name('account1')
        pub2, priv2 = ecc.generate_new_pair()
        account2 = fake_name('account2')

        # Create two new account. Also test trx with multiple actions
        newaccount1 = AG.new_action('newaccount', name=account1, owner=[pub1])
        newaccount2 = AG.new_action('newaccount', name=account2, owner=[pub2])
        trx = transaction.Transaction()
        trx.add_action(newaccount1)
        trx.add_action(newaccount2)
        trx.add_sign(priv1)
        trx.add_sign(priv2)
        api.push_transaction(data=trx.dumps())

        # Each new account has 1.00000 EVT
        # Try to transfer 0.01000 EVT from account1 to account2
        transferevt = AG.new_action(
            'transferevt', _from=account1, to=account2, amount=EvtAsset('0.01000'))
        trx = transaction.Transaction()
        trx.add_action(transferevt)
        trx.add_sign(priv1)  # sign with account1's owner's priv_key
        api.push_transaction(data=trx.dumps())

        # After Transfer, account1 should have 0.99000 EVT, and account2 should have 1.01000 EVT
        ac1 = api.get_account('''{"name": "%s"}''' %
                              (account1)).json()['balance']
        ac2 = api.get_account('''{"name": "%s"}''' %
                              (account2)).json()['balance']
        self.assertTrue(ac1 == '0.99000 EVT' and ac2 == '1.01000 EVT')

    def test_action_issuetoken(self):
        pub_key, priv_key = ecc.generate_new_pair()
        domain_name = fake_name()
        token_name = fake_name('token')

        newdomain = AG.new_action(
            'newdomain', name=domain_name, issuer=pub_key)
        issuetoken = AG.new_action('issuetoken', domain=domain_name, names=[
                                   token_name], owner=[pub_key])

        trx = transaction.Transaction()
        trx.add_action(newdomain)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        trx = transaction.Transaction()
        trx.add_action(issuetoken)
        trx.add_sign(priv_key)
        resp = api.push_transaction(trx.dumps())

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
            'newdomain', name=domain_name, issuer=pub_key)
        issuetoken = AG.new_action('issuetoken', domain=domain_name, names=[
                                   token_name], owner=[pub_key])

        # create a new domain
        trx = transaction.Transaction()
        trx.add_action(newdomain)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        # issue a new token
        trx = transaction.Transaction()
        trx.add_action(issuetoken)
        trx.add_sign(priv_key)
        api.push_transaction(trx.dumps())

        # transfer the token to pub2
        pub2, priv2 = ecc.generate_new_pair()
        transfer = AG.new_action(
            'transfer', domain=domain_name, name=token_name, to=[pub2])
        trx = transaction.Transaction()
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


if __name__ == '__main__':
    unittest.main()
