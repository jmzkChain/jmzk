# EVT ABI References
![everiToken Logo](./logo.png)

## Overview
The ABI defines the application interface interactive with the everiToken blockchain. Applications can use JSON RPC to communicate with everiToken. For the API references, you can check [document](API-References.md). This document will expand the details of abi json definition (ie. the `args` field you send to `/v1/chain/abi_json_to_bin`).

## Base Types
Before describing the ABI for each action, it is necessary to introduce some Base Types. Base Types are the basic types that EVT RPC interface supports. Their definitions are given in the table below.

| Type Name | Description | Additional |
|-----------|-------------|------------|
| `bool` | 8-bits unsigned integer | `0` for false, other for true |
| `int8` | 8-bits signed integer |  |
| `uint8` | 8-bits unsigned integer | |
| `int16` | 16-bits signed integer | |
| `uint16` | 16-bits unsigned integer | |
| `int32` | 32-bits signed integer | |
| `uint32` | 32-bits unsigned integer | |
| `int64` | 64-bits signed integer | |
| `uint64` | 64-bits unsigned integer | |
| `uint128` | 128-bits unsigned integer | |
| `bytes` | binary data represented in hex format | `0000abcd` represents for a 64-bits binary data |
| `string` | UTF-8 encoding string |
| `public_key` | ECC public key with `EVT` prefix | e.g. See __#1__ below |
| `signature` | ECC signature | e.g. See __#2__ below |
| `time_point_sec` | ISO format time string | e.g. `2018-03-02T12:00:00` |
| `name` | A string encoded into a 64-bits value | Max length: 13 chars and value range: `[0-9a-z.]` |
| `name128` | A string encoded into a 128-bits value | Max length: 21 chars and value range: `[0-9A-Za-z.-]` |
| `asset` | A floating number value with `symbol` as suffix | See `asset` type section below |
| `symbol` | Represents a token and contains precision and name. | See `symbol` type section below |
| `authorizer_ref` | Reference to a authorizer | Valid authorizer including an account, a group or special `OWNER` group |
| `group` | Authorize group tree | See `group` type section below |
| `address` | Address type | See `address` type section below |
| `evt_link` | EVT-Link type | Refer to this [document](EVT-Link.md) for more detail |
* __#1__: `EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX`
* __#2__: `SIG_K1_JzrdhWW46N5nFUZzTUmhg2sK4nKNGktPz2UdRz9bSAP5pY4nhicKWCuo6Uc6U7KBBwD8VfjsSxzHWT87R41xMaubnzMq8w`

ABIs are built on Base Types given above.

### `asset` Type
`asset` type is composed of two parts: the number part representing price or volume, and the symbol part describing the type name of asset.

The number part is a number containing a `.` which introduces its precision. The precision is determined by the digits after the `.`. That is, `0.300` has the precision of 3, while `0.3` only has the precision of 1. The precision of an asset should be less than __18__.

The symbol part introduces the symbol id, which is an integer number representing one unqiue fungible symbol.

Only the assets of the same type can be added up. The `EVT` asset is an asset type with the precision of 5 and `1` as symbol id. Therefore, `12.00000 S#1` is a valid `EVT` asset, but `12.000 S#1`, `12 S#1` or `12.0000 S#1` are invalid `EVT` asset due to the wrong precision.

### `symbol` Type
`symbol` type is the symbol part in `asset` type. It represents a token and contains precision and unique id. Precision is a number and should be less than __18__ and symbol id is a unique integer number.

For example, `12.00000 S#1` is a valid `EVT` asset, and it has the precision of 5 and '1' as symbol id. Its symbol expression is `5,S#1`.

Then `7,S#123` represents a asset symbol with precision of 7 and '123' as symbol id.

### `authorizer_ref` Type
For the `authorizer_ref`, it's a reference to one authorizer. Current valid authorizer including an account, a group or special `OWNER` group (aka. owners field in one token).
All the three formats will describe below:
1. Refs to an account named 'evtaccount', it starts with "[A]": `[A] evtaccount`.
2. Refs to a group named 'evtgroup', it starts with "[G]": `[G] evtgroup`.
3. Refs to `OWNER` group, it's just: `[G] .OWNER`.

### `address` Type
`address` type is used to replace old public key address, and it has three possible types.

* public key: As the same as old public key address
* reserved: 'EVT00000000000000000000000000000000000000000000000000', this type is used for representing destroyed token and cannot-updated group.
* generated: generated from three values: `prefix`(name), `key`(name128), `nonce`(uint32), this type is designed for some special usages.

All address has the length of 53 and has the prefix with 'EVT' and can be distinguished from the public key type by checking the forth char character, '0' or not. Because '0' cannot be used in normal public key representation.

Now we have two generated address usages:

#### Domain pay address
Used for the situation that domain can pay for the charge fees for its tokens.
You can get this address by call `/v1/evt/get_domain` API.

#### Fungible reserved address
Now initial fungible tokens are stored in special fungible reserved address. And the `issuefungible` will transfer fungible tokens from the reserved address to issue address.
You can get this address by call `/v1/evt/get_fungible` API.

### `group` Type
A authorizer group is a tree structure, each leaf node is a reference to one authorizer and also attached with a weight and non-leaf nodes behaves a `switch`, it has a threshold value, and only turned on if the sum of weights of all the authorized child nodes large than its threshold value. So a non-leaf node also has a weight value and a root node only has threshold value.

A sample group is defined as below:
```
{
    "name": "testgroup",
    "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
    "root": {
        "threshold": 6,
        "nodes": [
            {
                "threshold": 1,
                "weight": 3,
                "nodes": [
                    {
                        "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                        "weight": 1
                    },
                    {
                        "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                        "weight": 1
                    }
                ]
            },
            {
                "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                "weight": 3
            },
            {
                "threshold": 1,
                "weight": 3,
                "nodes": [
                    {
                        "key": "EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV",
                        "weight": 1
                    },
                    {
                        "key": "EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX",
                        "weight": 1
                    }
                ]
            }
        ]
    }
}
```
This sample group has the name `testgroup` and key `EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV`. Threshold of root node is 6 and it has 3 child nodes, with the weight is all the value of 3.

> The key of group is an `address` type. So it can be a reserved type address, representing that this group cannot be updated.

## Typedefs
Normally `typedef` is a keyword in C and C++. And it shares the same meaning here. It means that each type of the followings is the same type as its relative original type, or can be considered as an alias of the original type.

Any type with `[]` as suffix stands for an array type. For example, `int32[]` defines an array type and every single element in that array belongs to type `int32`.
A type with `?` as suffix means it is an optional type whose value can be undefined.

| Typedef Type | Original Type |
|-------------------|----------------|
| `user_id` | `public_key` |
| `user_list` | `user_id[]` |
| `group_key` | `public_key` |
| `weight_type` | `uint16` |
| `permission_name` | `name` |
| `action_name` | `name` |
| `domain_name` | `name128` |
| `domain_key` | `name128` |
| `group_name` | `name128` |
| `token_name` | `name128` |
| `account_name` | `name128` |
| `proposal_name` | `name128` |
| `fungible_name` | `name128` |
| `symbol_name` | `name128` |
| `balance_type` | `asset` |
| `symbol_id_type` | `uint32` |
| `group_def` | `group` |
| `meta_key` | `name128` |
| `meat_value` | `string` |
| `meta_list` | `meta[]` |
| `address_list` | `address[]` |

## Structures
A structure is a complex type consisted of base types or/and typedef types. Below are all the structures used in everiToken ABI interface. 

### `token_def` Struct
```
{
    "domain": `domain_name`,
    "name": `token_name`,
    "owner": `address_list`
}
```
### `authorizer_weight` Struct
```
{
    "ref": `authorizer_ref`,
    "weight": `weight_type`
}
```

### `permission_def` Struct
```
{
    "name": `permission_name`,
    "threshold": `uint32`,
    "authorizers", `authorizer_weight[]`
}
```

### `domain_def` Struct
```
{
    "name": `domain_name`,
    "creator": `user_id`,
    "create_time": `time_point_sec`,
    "issue": `permission_def`,
    "transfer": `permission_def`,
    "manage": `permission_def`,
    "metas": `meta_list`
}
```

### `fungible_def` Struct
```
{
    "name": `fungible_name`,
    "sym": `symbol_name`,
    "sym": `symbol`,
    "creator": `user_id`,
    "create_time": `time_point_sec`,
    "issue": `permission_def`,
    "manage": `permission_def`,
    "total_supply": "asset",
    "metas": "meta_list"
}
```

### `meta` Struct
```
{
    "key": `meta_key`,
    "value": `meta_value`,
    "creator": `authorizer_ref`
}
```

## Actions
There are nine kinds of actions in everiToken RPC interface, shown as below.

### `newdomain` Action
Create a new domain with name and permisson set of `issue`, `transfer` and `manage`.
```
{
    "name": `domain_name`,
    "creator": `user_id`,
    "issue", `permission_def`,
    "transfer", `permission_def`,
    "manage", `permission_def`
}
```

### `updatedomain` Action
Update a domain with new permissons, `issue`, `transfer` and `manage` are all optional.
```
{
    "name": `domain_name`,
    "issue", `permission_def?`,
    "transfer", `permission_def?`,
    "manage", `permission_def?`
}
```


### `issuetoken` Action
Issue one or more tokens in a specific domain, the default owners are also specified.
```
{
    "domain": `domain_name`,
    "names": `token_name[]`,
    "owner": `address_list`
}
```

### `transfer` Action
Transfer one token in specific domain to new owners.
```
{
    "domain": `domain_name`,
    "name": `token_name`,
    "to": `address_list`,
    "memo": `string`
}
```

### `destroytoken` Action
Destroy one token (aka. Cannot be used anymore)
```
{
    "domain": `domain_name`,
    "name": `token_name`
}
```

### `newgroup` Action
Create a new group with a name.
> `group_def` is a special type defined in Base Types section.

```
{
    "name": `group_name`,
    "group": `group_def`
}
```

### `updategroup` Action
Update one specific group's structure
```
{
    "name": `group_name`,
    "group": `group_def`
}
```

### `newfungible` Action
Create a new fungible assets definition with a specific total supply(0 means unlimited).
```
{
    "name": `fungible_name`,
    "sym_name": `symbol_name`,
    "sym": `symbol`,
    "creator": `user_id`,
    "issue": `permission_def`,
    "manage": `permission_def`,
    "total_supply": "asset"
}
```

### `updfungible` Action
Update one fungible assets definition.
```
{
    "sym_id": `symbol_id_type`,
    "issue": `permission_def?`,
    "manage": `permission_def?`
}
```

### `issuefungible` Action
Issue fungible assets
```
{
    "address": `address`,
    "number": `asset`,
    "memo": `string`
}
```

### `transferft` Action
Transfer fungible assets between addresses.
```
{
    "from": `address`,
    "to": `address`,
    "number": `asset`,
    "memo": `string`
}
```

### `evt2pevt` Action
Convert `EVT` fungible tokens to `Pined EVT`(symbol: `PEVT`) fungible tokens.
>This operation is not irreversible and `to` address is not limited to the giver.

```
{
    "from": `address`,
    "to": `address`,
    "number": `asset`,
    "memo": `string`
}
```

### `addmeta` Action
Add new metadata to one domain, group, token or fungible assets
>Creator is `authorizer_ref` which means that the creator can be one account(aka. public key) or one group.
>And it must be involved in the attached domain, token, group or fungible.

```
{
    "key": `meta_key`,
    "value": `meta_value`,
    "creator": `authorizer_ref`
}
```

### `newsuspend` Action
Add new suspend transaction (In-chain deferred-signning transaction)
> `trx` should be valid transaction defiintion

```
{
    "name": `proposal_name`,
    "proposer": `user_id`,
    "trx": `transaction`
}
```

### `aprvsuspend` Action
Approve one suspend transaction
> `signatures` are the signatures of the suspend transaction.
> And the keys used to sign transaction must be required keys while authorizing the suspend transaction.

```
{
    "name": `proposal_name`,
    "signatures": `signature[]`
}
```

### `cancelsuspend` Action
Cancel one suspend transaction
> Only the `proposer` can cancel his proposed suspend transaction.

```
{
    "name": `proposal_name`
}
```

### `execsuspend` Action
Execute one suspend transaction
> The `executor` must be one of the valid authorizers while authorizing the suspend transaction. 

```
{
    "name": `proposal_name`,
    "executor": `user_id`
}
```

### `everipass` Action
Execute everipass
```
{
    "link": `evt_link`
}
```

### `everipay` Action
Execute everipay
```
{
    "link": `evt_link`,
    "payee": `address`,
    "number": `asset`

}
```
