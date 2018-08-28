![everiToken Logo](./logo.png)

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

## GET /v1/chain/get_head_block_header_state
Used to get latest block header state.

Response:
```
{
    "id": "00000042452cd340f429846f292686c92fcf71c5743fc10f48b4192d69f2d67d",
    "block_num": 66,
    "header": {
        "timestamp": "2018-07-27T10:18:08.500",
        "producer": "evt",
        "confirmed": 0,
        "previous": "000000416651e2ed1cf0ea97cf8563200778cc2f09ef905c1ef55fb95c6843bd",
        "transaction_mroot": "0000000000000000000000000000000000000000000000000000000000000000",
        "action_mroot": "0000000000000000000000000000000000000000000000000000000000000000",
        "schedule_version": 0,
        "header_extensions": [],
        "producer_signature": "SIG_K1_KdLVwPJECYcnoTjaPqhQwWDvFJAkpYPfRNHwAAQQDyua58nju1dpTgwKkLxkwjDivkpW4nyc1JmRrXLA9CLmFhQXAuNZko"
    },
    "dpos_proposed_irreversible_blocknum": 66,
    "dpos_irreversible_blocknum": 65,
    "bft_irreversible_blocknum": 0,
    "pending_schedule_lib_num": 0,
    "pending_schedule_hash": "73965d02d55e9fb68d87aea8b85400b8cc014dd777b1366bf123b73c72aff712",
    "pending_schedule": {
        "version": 0,
        "producers": []
    },
    "active_schedule": {
        "version": 0,
        "producers": [
            {
                "producer_name": "evt",
                "block_signing_key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
            }
        ]
    },
    "blockroot_merkle": {
        "_active_nodes": [
            "000000416651e2ed1cf0ea97cf8563200778cc2f09ef905c1ef55fb95c6843bd",
            "b4412cfcf063a3ceb483e23cbf4601e377e5de25bfe4d101d463fe3f224bdf4c",
            "db4dd95a06e9969440a211dd38741152d4259306f6a1ce0e38a694daa4019251"
        ],
        "_node_count": 65
    },
    "producer_to_last_produced": [
        [
            "evt",
            66
        ]
    ],
    "producer_to_last_implied_irb": [
        [
            "evt",
            65
        ]
    ],
    "block_signing_key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
    "confirm_count": [],
    "confirmations": []
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
        "creator": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
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
    "max_charge": 10000,
    "payer": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
    "actions": [
        {
            "name": "newdomain",
            "domain": "test2",
            "key": ".create",
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
| `newdomain` | name of domain | `.create` |
| `updatedomain` | name of domain | `.update` |
| `newgroup` | `.group` | name of new group |
| `updategroup` | `.group` | name of updating group |
| `newfungible` | `.fungible` | symbol id of new fungible assets symbol |
| `updfungible` | `.fungible` | symbol id of updating fungible assets symbol |
| `issuetoken` | name of domain | `.issue` |
| `issuefungible` | `.fungible` | symbol id of issuing fungible assets symbol |
| `transfer` | name of domain token belongs to | name of token |
| `destroytoken` | name of domain token belongs to | name of token |
| `transferft` | `.fungible` | symbol id of transfering assets symbol |
| `evt2pevt` | `.fungible` | '1' |
| `addmeta` | `.group`, `.fungible` or token's domain | group name, symbol id of fungible or token name |
| `newsuspend` | `.suspend` | proposal name of suspend transaction |
| `aprvsuspend` | `.suspend` | proposal name of suspend transaction |
| `cancelsuspend` | `.suspend` | proposal name of suspend transaction |
| `execsuspend` | `.suspend` | proposal name of suspend transaction |
| `everipass` | name of domain | name of token |
| `everipay` | `.fungible` | name of fungible assets symbol |

> For fungible actions, action's key is symbol id which is a number originally. And it need to be conveted into string.

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
        "max_charge": 10000,
        "payer": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
        "actions": [
            {
                "name": "newdomain",
                "domain": "test2",
                "key": ".create",
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
        "max_charge": 10000,
        "payer": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
        "actions": [
            {
                "name": "newdomain",
                "domain": "test2",
                "key": ".create",
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
          "domain": "test2",
          "key": ".create",
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

## GET /v1/chain/get_suspend_required_keys
This API is specific used for getting required keys for suspend transaction. Other than the normal `get_required_keys` API, this API will not throw exception when your keys don't satisfies the permission requirments for one action, but instead returns the proper keys taking part in authorizing the suspended transaction.

Request:
```
{
  "name": "suspend3",
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
    "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
  ]
}
```

## GET /v1/chain/get_charge
This API is used to  query the accurate charge for one transaction.

Request:
```
{
    "transaction": { ... },
    "sigs_num": number of signatures
}
```

Response
```
{
    "charge": 12345
}
```
> 12345 represents for '0.12345' EVT(PEVT).

## POST /v1/chain/get_transaction
Used to fetch transaction by block num and transaction id

Request:
```
{
    "block_num": 12345,
    "id": "9f2ea4b512f49d2f3ff2be24e9cc4296ee0749b33bb9b1c06ae45a664bb00397"
}
```

## POST /v1/chain/get_trx_id_for_link_id
Used to fetch transaction id and block num by one EVT-Link id.
> Only sucessful executing everiPay actions can be queried by this API.

Request:
```
{
    "link_id": "16951b9b653947955faa6c3cb3e506b6"
}
```

Response:
```
{
    "trx_id": "9f2ea4b512f49d2f3ff2be24e9cc4296ee0749b33bb9b1c06ae45a664bb00397",
    "block_num": 12345
}
```

## POST /v1/evt_link/get_trx_id_for_link_id
Used to fetch transaction id and block num by one EVT-Link id. Difference between this API with `/v1/chain/get_trx_id_for_link_id` is that this API will not response directly, but instead it will block until excepted everiPay is executed successfully or reach max waiting time.
> Only sucessful executing everiPay actions can be queried by this API.

Request:
```
{
    "link_id": "16951b9b653947955faa6c3cb3e506b6"
}
```

Response:
```
{
    "trx_id": "9f2ea4b512f49d2f3ff2be24e9cc4296ee0749b33bb9b1c06ae45a664bb00397",
    "block_num": 12345
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
    "creator": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
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
    },
    "address": "EVT0000007EWoypBxpF8dWMb3xPoGdAMpPU3wehZqmD7VDgzsGyVi"
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
    "name": "t8"
}
```

## POST /v1/evt/get_fungible
This API is used to get specific fungible.

Request:
```
{
    "id": "1"
}
```
Response:
```
{
  "name": "EVT",
  "sym_name": "EVT",
  "sym": "5,S#1",
  "creator": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
  "create_time": "2018-06-28T05:31:09",
  "issue": {
    "name": "issue",
    "threshold": 1,
    "authorizers": [{
        "ref": "[A] EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
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
  },
  "total_supply": "100000.00000 S#1",
  "current_supply": "0.00000 S#1",
  "metas": [],
  "address": "EVT000000GAei9YYTQuZsNdcK5bPQNAK4ADtgzT8x52hz4b4FnYRQ"
}
```

## POST /v1/evt/get_fungible_balance
This API is used to get assets for an address

Request:
```
{
    "address": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"
}
```
Response:
```
[
  "2.00000 S#1", "1.00000 S#2"
]
```

You can also only query assets for specific symbol id

Request:
```
{
    "address": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
    "sym_id": 1
}
```
Response:
```
"2.00000 S#1"
```

## POST /v1/evt/get_suspend
This API is usded to get specific suspend proposal

Request:
```
{
    "name": "suspend3"
}
```

Response: 
```
{
  "name": "suspend3",
  "proposer": "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
  "status": "proposed",
  "trx": {
    "expiration": "2018-07-03T07:34:14",
    "ref_block_num": 23618,
    "ref_block_prefix": 1259088709,
    "max_charge": 10000,
    "payer": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
    "actions": [{
        "name": "newdomain",
        "domain": "test4",
        "key": ".create",
        "data": {
          "name": "test4",
          "creator": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
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
        },
        "hex_data": "000000000000000000000000189f077d0002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf000000008052e74c01000000010100000002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf000000000000000100000000b298e982a40100000001020000000000000000000000000000000000000000000000000000000000000000000000000000000000000001000000000094135c6801000000010100000002c0ded2bc1f1305fb0faac5e6c03ee3a1924234985427b6167ca569d13df435cf000000000000000100"
      }
    ],
    "transaction_extensions": []
  },
  "signed_keys": [
    "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"
  ],
  "signatures": [
    "SIG_K1_K1x3vANVU1H9zxKutyRUB4kHKqMLBCaohqPwEsit9oNL8j5SUgMxxgDFA7hwCz9DkrrpaLJSndqcxy3Rmy5qfQw21qHpiJ"
  ]
}
```

## POST /v1/history/get_tokens
Provide all the public keys its has and this API will response with all the tokens that account have.
> This API is only available when MONGO_DB_SUPPORT is ON.

Request:
```
{
    "keys":["EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"]
}
```
Response:
```
["cookie-t1", "cookie-t2", "cookie-t3"]
```

## POST /v1/history/get_domains
Provide all the public keys its has and this API will response with all the domains that account issue.
> This API is only available when MONGO_DB_SUPPORT is ON.

Request:
```
{
    "keys":["EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"]
}
```
Response:
```
["cookie"]
```

## POST /v1/history/get_groups
Provide all the public keys its has and this API will response with all the groups that account manage (aka. the key of group is one of account's public keys).
> This API is only available when MONGO_DB_SUPPORT is ON.

Request:
```
{
    "keys":["EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"]
}
```
Response:
```
["testgroup"]
```

## POST /v1/history/get_fungibles
Provide all the public keys its has and this API will response with all the symbol ids of the fungibles that account create.
> This API is only available when MONGO_DB_SUPPORT is ON.

Request:
```
{
    "keys":["EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"]
}
```
Response:
```
[3, 4, 5]
```

## POST /v1/history/get_actions
Query actions by domain, key and action names.
> This API is only available when MONGO_DB_SUPPORT is ON.
> key and names are optional fields and only for filtering actions. if you don't provide them, API will return all the actions.

Request:
```
{
  "domain": ".fungible",
  "key": "1",
  "names": [
    "newfungible"
  ],
  "skip": 0
  "take": 10
}

```
Response:
```
[{
    "name": "newfungible",
    "domain": ".fungible",
    "key": "1",
    "trx_id": "f0c789933e2b381e88281e8d8e750b561a4d447725fb0eb621f07f219fe2f738",
    "created_at": "2018-06-28T05:35:12",
    "data": {
      "sym": "5,EVT",
      "creator": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
      "issue": {
        "name": "issue",
        "threshold": 1,
        "authorizers": [{
            "ref": "[A] EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
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
      },
      "total_supply": "100000.00000 EVT"
    }
  }
]
```

## POST /v1/history/get_fungible_actions
Query fungible actions by address
> This API is only available when MONGO_DB_SUPPORT is ON.
> address is optional fields and only for filtering actions. if you don't provide them, API will return all the actions.

Request:
```
{
  "sym_id": 338422621,
  "addr": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
  "skip": 0
  "take": 10
}

```

Response:
```
[{
    "name": "transferft",
    "domain": ".fungible",
    "key": "338422621",
    "trx_id": "58034b28635c027f714fb01de202ae0ccefa1a4ba5bcf5f01b04fc53b79e6449",
    "data": {
      "from": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
      "to": "EVT6dpW7dAtR7YEAxatwA37sYZBdfQCtPD8Hoa1d7jnVDnCepNcM8",
      "number": "1.0000000000 S#338422621",
      "memo": "goodjob"
    },
    "created_at": "2018-08-08T08:21:44.001"
  },
  {
    "name": "issuefungible",
    "domain": ".fungible",
    "key": "338422621",
    "trx_id": "b05a0cea2093de4ca6eee0ca46ebfa0196ef6dad90a0bcc61f90b6a12bbbd30b",
    "data": {
      "address": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
      "number": "100.0000000000 S#338422621",
      "memo": "goodluck"
    },
    "created_at": "2018-08-08T08:21:44.001"
  }
]

```

## POST /v1/history/get_transaction
Query transaction by its id.
> This API is only available when MONGO_DB_SUPPORT is ON.

Request:
```
{
  "id": "f0c789933e2b381e88281e8d8e750b561a4d447725fb0eb621f07f219fe2f738"
}
```
Response:
```
{
  "id": "f0c789933e2b381e88281e8d8e750b561a4d447725fb0eb621f07f219fe2f738",
  "signatures": [
    "SIG_K1_K6hWsPBt7VfSrYDBZqCygWT8dbA6R3mpxKPjd3JUh18EQHfU55eVEkHgq8AR5odWjPXvYasZQ1LoNdaLKKhagJXXuXp3Y2"
  ],
  "compression": "none",
  "packed_trx": "bb72345b050016ed2e620001000a13e9b86a6e7100000000000000000000d0d5505206460000000000000000000000000040beabb00105455654000000000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a970000000008052e74c01000000010100000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a9700000000000000001000000000094135c6801000000010100000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a97000000000000000010000e40b5402000000054556540000000000",
  "transaction": {
    "expiration": "2018-06-28T05:31:39",
    "ref_block_num": 5,
    "ref_block_prefix": 1647242518,
    "max_charge": 10000,
    "payer": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
    "actions": [{
        "name": "newfungible",
        "domain": "fungible",
        "key": "EVT",
        "data": {
          "sym": "5,EVT",
          "creator": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
          "issue": {
            "name": "issue",
            "threshold": 1,
            "authorizers": [{
                "ref": "[A] EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
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
          },
          "total_supply": "100000.00000 EVT"
        },
        "hex_data": "05455654000000000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a970000000008052e74c01000000010100000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a9700000000000000001000000000094135c6801000000010100000003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a97000000000000000010000e40b54020000000545565400000000"
      }
    ],
    "transaction_extensions": []
  }
}
```

## POST /v1/history/get_transactions
Query transactions by posting all the public keys its has.
> This API is only available when MONGO_DB_SUPPORT is ON.

Request:
```
{
  "keys": [
    "EVT546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
    "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
    "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"
  ],
  "skip": 0,
  "take": 10
}
```
Response:
```
[{
    "id": "0925740e3be034e4ac345461d6f5b95162a7cf1578a7ec3c7b9de0e9f0f84e3c",
    "signatures": [
      "SIG_K1_K1G7PJcRaTgw8RBDVvHsj2SEPZTcV5S8KgdrSmpD1oUd6fgVdwD3jSqL7zSkaFAV2zDPsr4pYTK1QkusALsEDGXk4PUC8y"
    ],
    "compression": "none",
    "packed_trx": "9073345baf0105f3a965000100802bebd152e74c000000000000000000000000009f077d000000000000000000000000819e470164000000000000000000000000009f077d030000000000000000000000000000307c0000000000000000000000000000407c0000000000000000000000000000507c010003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a97000",
    "transaction": {
      "expiration": "2018-06-28T05:35:12",
      "ref_block_num": 431,
      "ref_block_prefix": 1705636613,
      "actions": [{
          "name": "issuetoken",
          "domain": "test",
          "key": ".issue",
          "data": {
            "domain": "test",
            "names": [
              "t1",
              "t2",
              "t3"
            ],
            "owner": [
              "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX"
            ]
          },
          "hex_data": "000000000000000000000000009f077d030000000000000000000000000000307c0000000000000000000000000000407c0000000000000000000000000000507c010003c7e3ff0060d848bd31bf53daf1d5fed7d82c9b1121394ee15dcafb07e913a970"
        }
      ],
      "transaction_extensions": []
    }
}]
```
