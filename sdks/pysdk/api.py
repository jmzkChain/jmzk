import requests


class Api:
    def __init__(self, host='https://testnet1.everitoken.io:8888'):
        self.host = host
        self.urls = {
            'abi_json_to_bin': '/v1/chain/abi_json_to_bin',
            'trx_json_to_digest': '/v1/chain/trx_json_to_digest',
            'get_required_keys': '/v1/chain/get_required_keys',
            'push_transaction': '/v1/chain/push_transaction',
            'get_domain': '/v1/evt/get_domain',
            'get_group': '/v1/evt/get_group',
            'get_token': '/v1/evt/get_token',
            'get_account': '/v1/evt/get_account',
            'get_my_tokens': '/v1/evt/get_my_tokens',
            'get_my_domains': '/v1/evt/get_my_domains',
            'get_my_groups': '/v1/evt/get_my_groups'
        }

    def __getattr__(self, api_name):
        def ret_func(data):
            return requests.post(self.host+self.urls[api_name], data=data)
        return ret_func

    def get_info(self):
        return requests.get(self.host+'/v1/chain/get_info').text
