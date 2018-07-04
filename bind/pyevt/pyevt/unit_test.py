import unittest

from . import abi
from .abi import *
from .ecc import *


class TestPyEVT(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        libevt.init_lib()

    def test_evtecc(self):
        pub_key, priv_key = generate_new_pair()
        pub_key_string = pub_key.to_string()
        pub_key_from_priv = priv_key.get_public_key()
        pub_key_string2 = pub_key_from_priv.to_string()
        self.assertTrue(pub_key_string == pub_key_string2)

        pub_key2 = PublicKey.from_string(pub_key_string)
        pub_key_string2 = pub_key2.to_string()
        self.assertTrue(pub_key_string == pub_key_string2)

        priv_key_string = priv_key.to_string()
        priv_key2 = PrivateKey.from_string(priv_key_string)
        priv_key_string2 = priv_key2.to_string()
        self.assertTrue(priv_key_string == priv_key_string2)

        check_sum = Checksum.from_string('hello world')
        check_sum.to_string()
        sign = priv_key.sign_hash(check_sum)
        sign.to_string()
        pub_key3 = PublicKey.recover(sign, check_sum)
        pub_key_string3 = pub_key3.to_string()
        self.assertTrue(pub_key_string3 == pub_key_string)

    def test_evtabi(self):
        j = r'''
        {
        "name": "test",
        "issuer": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
        "issue": {
            "name": "issue",
            "threshold": 1,
            "authorizers": [{
                "ref": "[A] EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                "weight": 1
            }]
        },
        "transfer": {
            "name": "transfer",
            "threshold": 1,
            "authorizers": [{
                "ref": "[G] OWNER",
                "weight": 1
            }]
        },
        "manage": {
            "name": "manage",
            "threshold": 1,
            "authorizers": [{
                "ref": "[A] EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                "weight": 1
            }]
        }
        }
        "transfer": {
            "name": "transfer",
            "threshold": 1,
            "authorizers": [{
                "ref": "[G] OWNER",
                "weight": 1
            }]
        },
        "manage": {
            "name": "manage",
            "threshold": 1,
            "authorizers": [{
                "ref": "[A] EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                "weight": 1
            }]
        }
        }
        '''

        j2 = r'''
        {
        "expiration": "2018-05-20T12:25:51",
        "ref_block_num": 8643,
        "ref_block_prefix": 842752750,
        "delay_sec": 0,
        "actions": [
            {
                "name": "newdomain",
                "domain": "domain",
                "key": "test2",
                "data": "000000000000000000000000109f077d0003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a9700000000000a5317601000000010100000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a9706d4859000000000100000000572d3ccdcd010000000102000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100000000002866a69101000000010100000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a9706d4859000000000100"
            }
        ],
        "transaction_extensions": []
        }
        '''
        bin = json_to_bin('newdomain', j)
        json = bin_to_json('newdomain', bin)
        chain_id = ChainId.from_string(
            'bb248d6319e51ad38502cc8ef8fe607eb5ad2cd0be2bdc0e6e30a506761b8636')
        digest = abi.trx_json_to_digest(j2, chain_id)

        block_id = BlockId.from_string(
            '000000cabd11d7f8163d5586a4bb4ef6bb8d0581f03db67a04c285bbcb83f921')
        self.assertEqual('000000cabd11d7f8163d5586a4bb4ef6bb8d0581f03db67a04c285bbcb83f921', block_id.to_hex_string())

        block_num = block_id.ref_block_num()
        self.assertTrue(block_num == 202)
        block_prefix = block_id.ref_block_prefix()
        self.assertTrue(block_prefix == 2253733142)


if __name__ == '__main__':
    unittest.main()
