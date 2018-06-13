import action
import json
from pyevt import ecc, abi

class Transcation:
    def __init__(self, ChainId, BlockId, **kwargs):
        self.chain_id = ChainId
        self.block_id = BlockId
        self.kwargs = kwargs
        self.actions = []
        self.transcation_extensions = []

    def addAction(self, _action):
        self.actions.append(_action.dict())
    
    def dumps(self):
        ret = {
            **self.kwargs,
            "ref_block_num": self.block_id.ref_block_num(),
            "ref_block_prefix": self.block_id.ref_block_prefix(),
            "actions": self.actions,
            "transcation_extensions": self.transcation_extensions
        }
        return json.dumps(ret)



def get_sign_transcation(priv_keys, transcation):
    digest = abi.trx_json_to_digest(transcation.dumps(), transcation.chain_id)
    signs = [each_priv_key.sign_hash(digest) for each_priv_key in priv_keys]
    ret = {
        "signatures": signs,
        "compression": "none",
        "transaction": transcation
    }
    return json.dumps(ret)
