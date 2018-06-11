# ABI References
## Overview
The ABI defines the application interface interactive with the blockchain. Applications can use JSON RPC to communicate with everiToken. For the API references, you can check this [document](API-References.md). This document will expand the details of `action` definition.

In EVT, one transaction consists of multiple actions. Each action represents one event in the blockchain. The structure of an action is defined like below:
```
{
    "name": "xxx",
    "domain": "...",
    "key": "...",
    "data": { ... }
}
```

Each action first has one field `name` which indicates its type. And `domain` and `key` fields specify where this action is applied to.
`data` field of an action defines the event details and according to the different type of actions(aka. different names), each action has its own specific structure of data, which is also named `abi`.

## Base Types
Before describing the ABI for each action, it's needed to introduce some base types. Base types are the basic types that EVT RPC interface supports. The table below is all the base types definitions.

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
| `authorizer_ref` | Reference to a authorizer | Valid authorizer including an account, a group or special `OWNER` group |
| `group` | Authorize group tree | See `group` type section below |
* __#1__: `EVT8MGU4aKiVzqMtWi9zLpu8KuTHZWjQQrX475ycSxEkLd6aBpraX`
* __#2__: `SIG_K1_JzrdhWW46N5nFUZzTUmhg2sK4nKNGktPz2UdRz9bSAP5pY4nhicKWCuo6Uc6U7KBBwD8VfjsSxzHWT87R41xMaubnzMq8w`

These are all the base types and ABIs are built on these types.

### `asset` Type
`asset` type is composed of two parts: the number part, which can be price or volume, and the symbol part is the type name of asset.

The number part is a number plus a `.` which introduces its precision. The precision is determined by the digits after the `.`. It means that `0.300` has the precision of 3 but `0.3` only has the precision of 1. The precision of an asset should be less than __18__.

The symbol introduces the name of this asset, and it's consisted of uppercase alphabets. Length of the symbol name should be less than 7 chars.

Both the precision and symbol makes up one type of asset. Only the same type of asset can be added up. The `EVT` asset is a type of asset with the precision of 4 and `EVT` as symbol name. So `12.0000 EVT` is a valid `EVT` asset but `12.000 EVT`, `12 EVT` and `12.00000 EVT` are all invalid `EVT` asset.

### `authorizer_ref` Type
For the `authorizer_ref`, it's a reference to one authorizer. Current valid authorizer including an account, a group or special `OWNER` group (aka. owners field in one token).
All the three formats will describe below:
1. Refs to an account named 'evtaccount', it starts with "[A]": `[A] evtaccount`.
2. Refs to a group named 'evtgroup', it starts with "[G]": `[G] evtgroup`.
3. Refs to `OWNER` group, it's just: `[G] OWNER`.

### `group` Type
A authorizer group is a tree structure, each leaf node is a reference to one authorizer and also attached with a weight and non-leaf nodes behaves a `switch`, it has a threshold value, and only turned on if the sum of weights of all the authorized child nodes large than its threshold value. So a non-leaf node also has a weight value and a root node only has threshold value.

A sample group defined as below:
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
This sample group has the name `testgroup` and key `EVT6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV`. Threshold of root node is 6 and it has three child nodes, with the weight is all the value of 3.

## Typedefs
Normally `typedef` is a keyword in C and C++. And it shares the same meaning here. It means that the each type of the followings are the same type as one original type. Only have different names.

And type with `[]` as suffix means is a array type, like `int32[]` means it a array type that its element type is `int32`.
Type with `?` as suffix means is a optional type which its value can be not provided.

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
| `balance_type` | `asset` |
| `group_def` | `group` |

## Structures
Structure is the complex type consisted of base or typedef types. Below are all the structures used in everiToken ABI interface. 

### `token_def` Struct
```
{
    "domain": `domain_name`,
    "name": `token_name`,
    "owner": `user_list`
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
    "issuer": `user_id`,
    "issue_time": `time_point_sec`,
    "issue", `permission_def`,
    "transfer", `permission_def`,
    "manage", `permission_def`
}
```

### `account_def` Struct
```
{
    "name": `account_name`,
    "creator": `account_name`,
    "create_time": `time_point_sec`,
    "balance": `balance_type`,
    "frozen_balance": `balance_type`
}
```

## Actions
Below are all the actions defined in everiToken RPC interface.

### `newdomain` Action
Create a new domain with  name and set `issue`, `transfer` and `manage` permissions.
```
{
    "name": `domain_name`,
    "issuer": `user_id`,
    "issue", `permission_def`,
    "transfer", `permission_def`,
    "manage", `permission_def`
}
```

### `issuetoken` Action
Issue one or more tokens in a specific domain, the default owners are also specified.
```
{
    "domain": `domain_name`,
    "names": `token_name[]`,
    "owner": `user_list`
}
```

### `transfer` Action
Transfer one token in specific domain to new owners.
```
{
    "domain": `domain_name`,
    "name": `token_name`,
    "to": `user_list`
}
```

### `newgroup` Action
Create a new group with a name.
> NOTICE: `group_def` is a special type defined in Base Types section. 
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

### `newaccount` Action
Create a new account with a name and owners.
```
{
    "name": `account_name`,
    "owner": `user_list`
}
```

### `updateowner` Action
Update owners of a specific account.
```
{
    "name": `account_name`,
    "owner": `user_list`
}
```

### `transferevt` Action
Transfer `EVT` asset between accounts.
```
{
    "from": `account_name`,
    "to": `account_name`,
    "amount": `balance_type`
}
```