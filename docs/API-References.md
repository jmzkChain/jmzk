## GET /v1/chain/get_info
When getting interactive with EVT, you need to get information about chain first. Call this API to get `chain_id`, `evt_abi_version` and many other properties of current chain state.

For the `chain_id`, it's consisted of three parts splitting by `.`: `major version`, `minor version` and `patch version`. `major version` changes when there have been some break changes in API. And for `minor version`, it will be changed when some new APIs are added, but old APIs are not influenced. `patch version` is only for some bug-fixed changes.

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
This API is used to convert this abi formatted json data into binary data. And it also can be used to check if your input action data is well-formatted and correct. The `action` below is the name of one action and args is the data definitions.

For the abi details, please check this [document](ABI-References.md)

If request is valid then it will response with the binary data.

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
After have checked actions are correct, it's time to build the transaction structure. As said before, a transaction consists several actions. A example transaction is like below:

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
Ignore other fields you can see the `actions` fields is a array of `action` definition.
The `ref_block_num` and `ref_block_prefix` are the reference fields that corresponding to the head block. They can be calculated from the `chain_id` which you got from the previous call to `/v1/chain/get_info`.

`chain_id` is a 256-long binary data and is represented in hex format. If we assume `chain_id` is a big number represented in little-endian encoding, then the `ref_block_num` is one 16-bits number represented in big-endian and it equals to the 6-7 bytes(index starts at zero) of `chain_id`.

`ref_block_prefix` is one 32-bits number in little-endian and it equals to the 12-15 bytes of `chain_id`.

`expiration` is the time at which a transaction expires. If the time a transaction being executed is later than this value, execution will be turned out failed.

`transaction_extensions` is the field should be ignored now, it may be used in later release.

For the fields of action, here is one quick reference guide.

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

After all that work, you can send transaction definition using this API to the chain, then chain will response with the digest of the transaction. You can then sign this digest with your private key.

Response:
```
{
  "digest": "e58e12b2069e736b922c0e4a7eb39af477d000866b33f2ab899d5eacf832d4b"
}
```
This `digest` is also a 256-bits long binary data represented in hex format.

## GET /v1/chain/get_required_keys
After you got the digest of one transaction, you can then sign the digest with your private key. In EVT each transaction may need to be signed with multiple signatures. You can use this API to provide all the keys you have and then query which keys are needed.

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
After signed digest of a transaction, you can just push signed transaction!

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