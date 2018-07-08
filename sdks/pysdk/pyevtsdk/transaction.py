import datetime
import json

from pyevt import abi, ecc

from . import action, api


class Transaction:
    def __init__(self):
        self.chain_id = None
        self.block_id = None

        self.expiration = (datetime.datetime.utcnow(
        ) + datetime.timedelta(hours=1)).strftime('%Y-%m-%dT%H:%M:%S')
        self.actions = []
        self.transaction_extensions = []

        self.priv_keys = []
        self.signatures = []

    def set_refs(self, **kwargs):
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

    def add_action(self, _action):
        self.actions.append(_action.dict())

    def add_sign(self, priv_key):
        self.priv_keys.append(priv_key)

    def dict(self):
        ret = {
            'expiration': self.expiration,
            'ref_block_num': str(self.block_id.ref_block_num()),
            'ref_block_prefix': str(self.block_id.ref_block_prefix()),
            'actions': self.actions,
            'transaction_extensions': self.transaction_extensions
        }
        return ret

    def delay_dict(self):
        ret = {
            'expiration': self.expiration,
            'ref_block_num': str(self.block_id.ref_block_num()),
            'ref_block_prefix': str(self.block_id.ref_block_prefix()),
            'actions': self.actions,
        }
        return ret

    def dumps(self):
        digest = abi.trx_json_to_digest(json.dumps(self.dict()), self.chain_id)
        self.signatures = [priv_key.sign_hash(
            digest).to_string() for priv_key in self.priv_keys]
        ret = {
            'signatures': self.signatures,
            'compression': 'none',
            'transaction': self.dict()
        }
        return json.dumps(ret)


class TrxGenerator:
    def __init__(self, url):
        evtapi = api.Api(url)
        info = json.loads(evtapi.get_info())
        chain_id_str = info['chain_id']
        block_id_str = info['head_block_id']
        self.chain_id = abi.ChainId.from_string(chain_id_str)
        self.block_id = abi.BlockId.from_string(block_id_str)

    def new_trx(self):
        trx = Transaction()
        trx.set_refs(chain_id=self.chain_id, block_id=self.block_id)
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
