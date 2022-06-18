import unittest

from . import abi
from .abi import *
from .address import *
from .ecc import *
from .jmzk_link import *


class TestPyjmzk(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        ver = libjmzk.init_lib()
        assert ver == abi.version()
        print('jmzk Api Version:', ver)

    def test_jmzkecc(self):
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

    def test_jmzkabi(self):
        j = r'''
        {
            "name": "test",
            "creator": "jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
            "issue": {
                "name": "issue",
                "threshold": 1,
                "authorizers": [{
                    "ref": "[A] jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
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
                    "ref": "[A] jmzk8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
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
        self.assertEqual(
            '000000cabd11d7f8163d5586a4bb4ef6bb8d0581f03db67a04c285bbcb83f921', block_id.to_hex_string())

        block_num = block_id.ref_block_num()
        self.assertTrue(block_num == 202)
        block_prefix = block_id.ref_block_prefix()
        self.assertTrue(block_prefix == 2253733142)

    def test_jmzkaddress(self):
        reserved_addr = Address.reserved()
        self.assertEqual(
            'jmzk00000000000000000000000000000000000000000000000000', reserved_addr.to_string())
        self.assertEqual('reserved', reserved_addr.get_type())

        pub_key = PublicKey.from_string(
            'jmzk6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3')
        public_key_addr = Address.public_key(pub_key)
        self.assertEqual(
            'jmzk6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3', public_key_addr.to_string())
        self.assertEqual('public_key', public_key_addr.get_type())

        generated_addr = Address.generated(
            'xxxxxxxxxxxx', 'xxxxxxxxxxxxxxxxxxxxx', 1234)
        prefix = generated_addr.get_prefix()
        key = generated_addr.get_key()
        nonce = generated_addr.get_nonce()
        self.assertEqual(prefix, 'xxxxxxxxxxxx')
        self.assertEqual(key, 'xxxxxxxxxxxxxxxxxxxxx')
        self.assertEqual(nonce, 1234)
        self.assertEqual('generated', generated_addr.get_type())

    def test_jmzklink(self):
        link_str = '03XBY4E/KTS:PNHVA3JP9QG258F08JHYOYR5SLJGN0EA-C3J6S:2G:T1SX7WA14KH9ETLZ97TUX9R9JJA6+06$E/_PYNX-/152P4CTC:WKXLK$/7G-K:89+::2K4C-KZ2**HI-P8CYJ**XGFO1K5:$E*SOY8MFYWMNHP*BHX2U8$$FTFI81YDP1HT'
        jmzk_link = jmzkLink.parse_from_jmzkli(link_str)
        header = jmzk_link.get_header()
        timestamp = jmzk_link.get_segment_int('timestamp')
        domain = jmzk_link.get_segment_str('domain')
        token = jmzk_link.get_segment_str('token')

        signs = jmzk_link.get_signatures()

        self.assertEqual(header, 3)
        self.assertEqual(timestamp, 1532465234)
        self.assertEqual(domain, 'nd1532465232490')
        self.assertEqual(token, 'tk3064930465.8381')

        pub_key, priv_key = generate_new_pair()
        jmzk_link_1 = jmzkLink()
        jmzk_link_1.set_header(3)
        jmzk_link_1.add_segment_int('timestamp', 1532465234)
        jmzk_link_1.add_segment_str('domain', 'nd1532465232490')
        jmzk_link_1.add_segment_str('token', 'tk3064930465.8381')
        jmzk_link_1.sign(priv_key)

        self.assertEqual(jmzk_link_1.get_segment_int('timestamp'), 1532465234)
        self.assertEqual(jmzk_link.get_segment_str('domain'), 'nd1532465232490')

        jmzk_link = jmzkLink.parse_from_jmzkli('0DFYZXZO9-:Y:JLF*3/4JCPG7V1346OZ:R/G2M93-2L*BBT9S0YQ0+JNRIW95*HF*94J0OVUN$KS01-GZ-N7FWK9_FXXJORONB7B58VU9Z2MZKZ5*:NP3::K7UYKD:Y9I1V508HBQZK2AE*ZS85PJZ2N47/41LQ-MZ/4Q6THOX**YN0VMQ*3/CG9-KX2:E7C-OCM*KJJT:Z7640Q6B*FWIQBYMDPIXB4CM:-8*TW-QNY$$AY5$UA3+N-7L/ZSDCWO1I7M*3Q6*SMAYOWWTF5RJAJ:NG**8U5J6WC2VM5Z:OLZPVJXX*12I*6V9FL1HX095$5:$*C3KGCM3FIS-WWRE14E:7VYNFA-3QCH5ULZJ*CRH91BTXIK-N+J1')
        link_id = jmzk_link.get_link_id()
        self.assertEqual(link_id[0], 139)
        self.assertEqual(link_id[1], 90)
        self.assertEqual(link_id[15], 185)

        jmzk_link.set_link_id(bytes(
            [139-1, 90, 90, 91, 249, 106, 190, 191, 63, 143, 113, 132, 245, 34, 161, 185]))
        self.assertEqual(jmzk_link.get_link_id()[0], 138)

        jmzk_link.set_link_id_rand()


if __name__ == '__main__':
    unittest.main()
