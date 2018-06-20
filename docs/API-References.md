# EVT API References
## GET /v1/chain/get_info
Before interacting with everiToken, you need to get some chain information first. Call this API to get `chain_id`, `evt_abi_version` and many other properties of current chain state.

For the `evt_api_version`, it is consisted of three parts splitted by `.`: `major version`, `minor version` and `patch version`. `major version` changes when there have been some break changes in API. And for `minor version`, it will be changed when some new APIs are added, but old APIs will not be influenced. `patch version` is only for some bug-fixed changes.

Response:
```
{
  "server_version": "2286c798",
  "chain_id": "bb248d6319e51ad38502cc8ef8fe607eb5ad2cd0be2bdc0e6e30a506761b8636",
  "evt_api_version": "1.2.0",
  "head_block_num": 184197,
  "last_irreversible_block_num": 184196,
  "last_irreversible_block_id": "0002cf84749271ebcf3756486b0a8d5bcc39f1a977961502185d9177ca41261e",
  "head_block_id": "0002cf853428e9db7dcb34d475ad158b9209111f8d7d1db82e47571a433b0334",
  "head_block_time": "2018-06-08T01:55:43",
  "head_block_producer": "evt",
  "recent_slots": "",
  "participation_rate": "0.00000000000000000"
}
```

## POST /v1/chain/abi_json_to_bin
This API is used to convert the abi formatted json data into binary data. And it also can be used to check if your input action data is well-formatted and correct. The `action` below is the name of one action and args is the json definitions.

For the abi references, please check [document](ABI-References.md)

If a request is valid, it will response with the binary data.

Request:
```
{
    "action": "newdomain",
    "args": {
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
}
```
Response:
```
{
    "binargs": "000000000000000000000000009f077d0003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a9700000000000a5317601000000010100000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a97083f7a3000000000100000000572d3ccdcd010000000102000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100000000002866a69101000000010100000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a97083f7a3000000000100"
}
```

## POST /v1/chain/trx_json_to_digest
After verifying the correctness of actions, it is time to build up the transaction structure. As mentioned before, a transaction consists several actions. An example is given:

Request:
```
{
    "expiration": "2018-05-20T12:25:51",
    "ref_block_num": 8643,
    "ref_block_prefix": 842752750,
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
```
In everiToken, a transaction consists of multiple actions. Each action represents an event in the blockchain. The structure of an action is defined as below:
```
{
    "name": "xxx",
    "domain": "...",
    "key": "...",
    "data": "hex-binary-data"
}
```
Each action first has one field `name` which indicates its type. The action types are documented in the `Actions` section in this [document](ABI-References.md). The `domain` and `key` fields specify where the certain action will be applied to. More details about these two fields you can check the table below.

`data` field is the hex binary data you got from the `/v1/chain/abi_json_to_bin` API.

`ref_block_num` and `ref_block_prefix` are the reference fields corresponding to the head block. They can be calculated from the `head_block_id` which you got from the previous call to `/v1/chain/get_info`.

`head_block_id` is a 256-long binary data and represented in hex format. If we assume `head_block_id` as a big number represented in little-endian encoding, then the `ref_block_num` is one 16-bit number represented in big-endian and it equals to the No.6-7 bytes(whose index starts from zero) of `head_block_id`.

`ref_block_prefix` is a 32-bit number in little-endian and it equals to the No.12-15 bytes of `chain_id`.

`expiration` is the time at which a transaction expires. If the time a transaction being executed at is later than this value, the execution will fail.

`transaction_extensions` is the field should be ignored now, it may be used in further release.

For the fields of an action, here is a quick reference guide.

| Action Name | Domain | Key |
|-------------|--------|-----|
| `newdomain` | `domain` | name of new domain |
| `updatedomain` | `domain` | name of updating domain |
| `newgroup` | `group` | name of new group |
| `updategroup` | `group` | name of updating group |
| `newaccount` | `account` | name of new account |
| `updateowner` | `account` | name of updating account |
| `transferevt` | `account` | name of giving account |
| `issuetoken` | name of domain | `issue` |
| `transfer` | name of domain token belongs to | name of token |
| `addmeta` | `domain`, `group` or name of domain | domain name, group name or token name |

After all that work, you can send transaction definition using this API to the chain, then the chain will response with the digest of the transaction. You can then sign this digest with your private key.

Response:
```
{
  "digest": "e58e12b2069e736b922c0e4a7eb39af477d000866b33f2ab899d5eacf832d4b"
}
```
This `digest` is also a 256-bit long binary data represented in hex format.

## GET /v1/chain/get_required_keys
After you got the digest of one transaction, you can sign the digest with your private key. In everiToken, each transaction may need to be signed with multiple signatures. You can use this API to provide all the keys you have and then query which keys are required.

Request:
```
{
    "transaction": {
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
    },
    "available_keys": [
        "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
        "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
        "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"
    ]
}
```
Response:
```
{
    "required_keys": [
        "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"
    ]
}
```

## POST /v1/chain/push_transaction
After signing the digest of a transaction, you can just push the signed transaction right away!

Request:
```
{
    "signatures": [
        "SIG_K1_JzrdhWW46N5nFUZzTUmhg2sK4nKNGktPz2UdRz9bSAP5pY4nhicKWCuo6Uc6U7KBBwD8VfjsSxzHWT87R41xMaubnzMq8w"
    ],
    "compression": "none",
    "transaction": {
        "expiration": "2018-05-20T12:25:51",
        "ref_block_num": 8643,
        "ref_block_prefix": 842752750,
        "max_net_usage_words": 0,
        "max_cpu_usage_ms": 0,
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
}
```
Response:
```
{
  "transaction_id": "82a9781d5059746f53ccfad299f649a816951b9b653947955faa6c3cb3e506b6",
  "processed": {
    "id": "82a9781d5059746f53ccfad299f649a816951b9b653947955faa6c3cb3e506b6",
    "receipt": {
      "status": "executed"
    },
    "elapsed": 508,
    "net_usage": 0,
    "scheduled": false,
    "action_traces": [{
        "receipt": {
          "act_digest": "f63951f2521d1c778b64fc9b58bd7f4985e16e50d84d98c98c59557df60096d5",
          "global_sequence": 6
        },
        "act": {
          "name": "newdomain",
          "domain": "domain",
          "key": "test2",
          "data": {
            "name": "test2",
            "issuer": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
            "issue": {
              "name": "issue",
              "threshold": 1,
              "authorizers": [{
                  "ref": "[A] EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                  "weight": 1
                }
              ]
            },
            "transfer": {
              "name": "transfer",
              "threshold": 1,
              "authorizers": [{
                  "ref": "[G] OWNER",
                  "weight": 1
                }
              ]
            },
            "manage": {
              "name": "manage",
              "threshold": 1,
              "authorizers": [{
                  "ref": "[A] EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                  "weight": 1
                }
              ]
            }
          },
          "hex_data": "000000000000000000000000109f077d0003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a9700000000000a5317601000000010100000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a9706d4859000000000100000000572d3ccdcd010000000102000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100000000002866a69101000000010100000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a9706d4859000000000100"
        },
        "elapsed": 348,
        "console": "",
        "trx_id": "82a9781d5059746f53ccfad299f649a816951b9b653947955faa6c3cb3e506b6"
      }
    ],
    "except": null
  }
}
```

## POST /v1/evt/get_domain
This API is used to get specific domain.

Request:
```
{
    "name": "cookie"
}
```
Response:
```
{
    "name": "cookie",
    "issuer": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
    "issue_time": "2018-06-09T09:06:27",
    "issue": {
        "name": "issue",
        "threshold": 1,
        "authorizers": [{
                "ref": "[A] EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                "weight": 1
            }
        ]
    },
    "transfer": {
        "name": "transfer",
        "threshold": 1,
        "authorizers": [{
                "ref": "[G] OWNER",
                "weight": 1
            }
        ]
    },
    "manage": {
        "name": "manage",
        "threshold": 1,
        "authorizers": [{
                "ref": "[A] EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                "weight": 1
            }
        ]
    }
}
```

## POST /v1/evt/get_group
This API is used to get specific group.

Request:
```
{
    "name": "testgroup"
}
```
Response:
```
{
    "name": "testgroup",
    "key": "EVT5RsxormWcjvVBvEdQFonu5RNG4js8Zvz9pTjABLZaYxo6NNbSJ",
    "root": {
        "threshold": 6,
        "weight": 0,
        "nodes": [{
                "threshold": 1,
                "weight": 3,
                "nodes": [{
                        "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                        "weight": 1
                    }, {
                        "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                        "weight": 1
                    }
                ]
            }, {
                "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                "weight": 3
            }, {
                "threshold": 1,
                "weight": 3,
                "nodes": [{
                        "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                        "weight": 1
                    }, {
                        "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                        "weight": 1
                    }
                ]
            }
        ]
    }
}
```

## POST /v1/evt/get_token
This API is used to get specific token in specific domain
Request:
```
{
    "domain": "cookie",
    "name": "t8",
    "owner": ["EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"]
}
```

## POST /v1/evt/get_account
This API is used to get specific account.

Request:
```
{
    "name": "harry"
}
```
Response:
```
{
    "name": "harry",
    "creator": "evt",
    "create_time": "2018-06-08T13:53:26",
    "balance": "1.0000 EVT",
    "frozen_balance": "0.0000 EVT",
    "owner": ["EVT7WLUYMcF6XYYrogGzphNxhyUyBfNAVEw9jkH6YWroGPadwXCkN"]
}
```

## POST /v1/evt/get_my_tokens
This API is special for wallet application and mongo_db_plugin is needed to be enabled.
The wallet should use the account's private keys to sign sha256('everiWallet') and this API will response with all the tokens that account have.

Request:
```
{
    "signatures":["SIG_K1_KAgh7zpYtSb1T53odh2xJtcCAXkTRS3qZ3Dj8DvPyx17AsEqxz79tSwKTjRzQbqiQvLyjQayXeLwT2nThkL6RUj9vBGtvo"]
}
```
Response:
```
["cookie-t1", "cookie-t2", "cookie-t3"]
```

## POST /v1/evt/get_my_domains
This API is special for wallet application and mongo_db_plugin is needed to be enabled.
The wallet should use the account's private keys to sign sha256('everiWallet') and this API will response with all the domains that account issue.

Request:
```
{
    "signatures":["SIG_K1_KAgh7zpYtSb1T53odh2xJtcCAXkTRS3qZ3Dj8DvPyx17AsEqxz79tSwKTjRzQbqiQvLyjQayXeLwT2nThkL6RUj9vBGtvo"]
}
```
Response:
```
["cookie"]
```

## POST /v1/evt/get_my_groups
This API is special for wallet application and mongo_db_plugin is needed to be enabled.
The wallet should use the account's private keys to sign sha256('everiWallet') and this API will response with all the groups that account manage (aka. the key of group is one of account's public keys).

Request:
```
{
    "signatures":["SIG_K1_KAgh7zpYtSb1T53odh2xJtcCAXkTRS3qZ3Dj8DvPyx17AsEqxz79tSwKTjRzQbqiQvLyjQayXeLwT2nThkL6RUj9vBGtvo"]
}
```
Response:
```
["testgroup"]
```