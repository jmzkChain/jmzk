import datetime
import json

from pyevt import abi, ecc

from . import action, api, base


class Transaction:
    def __init__(self):
        self.chain_id = None
        self.block_id = None

        self.expiration = (datetime.datetime.utcnow(
        ) + datetime.timedelta(seconds=1800)).strftime('%Y-%m-%dT%H:%M:%S')
        self.actions = []
        self.transaction_extensions = []

        self.max_charge = '999999999'
        self.payer = str(base.Address())

        self.priv_keys = []
        self.signatures = []

    def set_header(self, **kwargs):
        if 'url' in kwargs:
            evtapi = api.Api(kwargs['url'])
            info = json.loads(evtapi.get_info())
            chain_id_str = info['chain_id']
            block_id_str = info['head_block_id']
            self.chain_id = abi.ChainId.from_string(chain_id_str)
            self.block_id = abi.BlockId.from_string(block_id_str)
        else:
            self.chain_id = kwargs['chain_id']
            self.block_id = kwargs['block_id']

        if 'max_charge' in kwargs:
            self.max_charge = kwargs['max_charge']

    def set_payer(self, payer):
        self.payer = payer

    def add_action(self, _action):
        self.actions.append(_action.dict())

    def add_sign(self, priv_key):
        self.priv_keys.append(priv_key)

    def dict(self):
        ret = {
            'expiration': self.expiration,
            'ref_block_num': str(self.block_id.ref_block_num()),
            'ref_block_prefix': str(self.block_id.ref_block_prefix()),
            'max_charge': self.max_charge,
            'actions': self.actions,
            'payer': self.payer,
            'transaction_extensions': self.transaction_extensions
        }
        if self.payer == None:
            del ret['payer']
        return ret

    def dumps(self):
        try:
            digest = abi.trx_json_to_digest(
                json.dumps(self.dict()), self.chain_id)
        except:
            raise Exception('Invalid transaction',
                            json.dumps(self.dict()), self.chain_id)

        self.signatures = [priv_key.sign_hash(
            digest).to_string() for priv_key in self.priv_keys]
        ret = {
            'signatures': self.signatures,
            'compression': 'none',
            'transaction': self.dict()
        }
        return json.dumps(ret)


class TrxGenerator:
    def __init__(self, url, payer):
        evtapi = api.Api(url)
        info = json.loads(evtapi.get_info())
        chain_id_str = info['chain_id']
        block_id_str = info['head_block_id']
        self.chain_id = abi.ChainId.from_string(chain_id_str)
        self.block_id = abi.BlockId.from_string(block_id_str)
        self.payer = payer

    def new_trx(self):
        trx = Transaction()
        trx.set_header(chain_id=self.chain_id, block_id=self.block_id)
        trx.set_payer(self.payer)
        return trx


def get_sign_transaction(priv_keys, transaction):
    digest = abi.trx_json_to_digest(transaction.dumps(), transaction.chain_id)
    signs = [each_priv_key.sign_hash(digest) for each_priv_key in priv_keys]
    ret = {
        'signatures': signs,
        'compression': 'none',
        'transaction': transaction
    }
    return json.dumps(ret)
