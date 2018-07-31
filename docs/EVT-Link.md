# Technical Documentation of EVT-Link / everiPass / everiPay

## Intro

`everiPass` / `everiPay` is a brand new way to go through a gateway or do payment. People use their wallet to generate a dynamic QR Code as a proof that he/she is the owner of specific token (both NFTs / FTs).

Here is a example UI of `everiPass`:

![everiPass](pass-qr.png)

We call the text which is used to generate the QR Code `EVT-Link`.

`EVT-Link` is also used to generate payee QR Code.

## How to use everiPay / Pass

1. The owner of some token (both NFTs and FTs) use his / her wallet to generate a series of QR codes (each of them represents a `EVT-Link`). 
   - The QR code will keep changing every several seconds. Old codes will expire very soon.
   - The QR code is made up of current time, the name of the token he / she wants to use and owner's signature for the link.
2. Payee use a everiPass-compatible scanner / a mobile phone or any other kinds of QR code scanner to read the code and get the decoded `EVT-Link` inside it. The link is then included in a transaction and pushed by the machine / phone / scanner.
3. The BP received the transaction with the `EVT-Link` inside and check the signatures and actions in the link.

## SDKs

`evtjs` has full support for `EVT-Link` and `everiPay / everiPass`.

## On-Chain

`everipass` and `everipay` actions are used to execute the transaction of `EVT-Link`. A struct named `evt_link` is used to represent `EVT-Link`. For detail information, please refer to the API / ABI documents.

## Structure of EVT-Link

Each `EVT-Link` has the struct as below:

```xml
[https://evt.li/]<base42segmentsStream>[_<base42signaturesList>]
```

`-base42signaturesList` is required for `everiPass / everiPay` and not necessary for `payeeCode`.

`[https://evt.li/]` is optional prefix.

We use `base42` encoding because this encoding of QR Code is very efficient.

## base42 Encoding

Base42 is a encoding for binary to string converting. It is similar to `hexadecimal`, but use 42 as its base and use a different alphabet. The chars in the alphabet are matched with the chars in encoding of QR Code's alphanumeric mode so it's efficient. Thereby the size of QR Code could be smaller.

Below is the alphabet of `base42`:

```
0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ$+-/:*
```

Note that the index of a char (from zero) means the value of the char too. So A is `10` and : is `41`.

For binary starts with one or more zero, you should add `0` to encoded result.

For example, for this byte array:

```
[ 0, 0, 0, 2, 41, 109, 0, 82, 0 ]
```

The encoded result is `000AD1KQVMO`. The count of prefixed zero is the same as the count of prefixed zero in original byte array.

## Segments Stream

`Segments stream` is a binary structure which has a structure as below:

```xml
    <flag><segment><segment><segment><segment><segment>....
```

`Flag` is the place to set some proterties of the link. Different flag can be added together. So `7` means  ( 1 + 2 + 4 ). This table shows the detail:

| flag | meaning |
| ---  | --- |
|  1   | protocol version 1 (required)
|  2   | everiPass
|  4   | everiPay
|  8   | should destory the NFT after validate the token in everiPass
| 16   | payee's QR code

Below is the struct of each `segment`:

```xml
    <typeKey><value>
```

`typeKey` is a unsigned byte.

Different `typeKey` has different `data types` for its `value`.


| from | to (included) | data type |
| --- | --- | --- |
| 0   |  20 | 1-byte unsigned integer
| 21  |  40 | 2-byte unsigned integer (BE)
| 41  |  90 | 4-byte unsigned integer (BE)
| 91  | 155 | string
| 156 | 165 | uuid
| 166 | 180 | byte string
| 180 | 255 | remained


Here is a brief reference of common used `typeKey` for convenient.

| typeKey | flag | description of value |
| --- | --- | --- |
|  `42` | | (uint32) unix timestamp in seconds |
|  `43` | | (uint32) max allowed amount for everiPay |
|  `91` | | (string) domain name to be validated in everiPass |
|  `92` | | (string) token name to be validated in everiPass |
|  `93` | | (string) symbol name to be paid in everiPay (for example: "5,EVT") |
|  `94` | | (string) max allowed amount for payment (optionl, string format remained only for amount >= 2 ^ 32) |
|  `95` | | (string) public key (address) for receiving points or coins |
| `156` | | (uuid) link id(128-bit) |

## base42signaturesList

Signatures is encoded the same way as the segments. If it is a `multisign`, the decoded `base42signaturesList` will put them one by one:

```
<65-byte sign><65-byte sign><65-byte sign><65-byte sign>...
```

For each signature, it has a fixed 65-byte length and a structure as follow:

```
<recoverParam><r><s>
```

`recoverParam` is `uint8`;
`r` and `s` has a length of 32-byte.

## Example

Here is an examples of valid `EVT-Link`:

```
0DFYZXZO9-:Y:JLF*3/4JCPG7V1346OZ:R/G2M93-2L*BBT9S0YQ0+JNRIW95*HF*94J0OVUN$KS01-GZ-N7FWK9_FXXJORONB7B58VU9Z2MZKZ5*:NP3::K7UYKD:Y9I1V508HBQZK2AE*ZS85PJZ2N47/41LQ-MZ/4Q6THOX**YN0VMQ*3/CG9-KX2:E7C-OCM*KJJT:Z7640Q6B*FWIQBYMDPIXB4CM:-8*TW-QNY$$AY5$UA3+N-7L/ZSDCWO1I7M*3Q6*SMAYOWWTF5RJAJ:NG**8U5J6WC2VM5Z:OLZPVJXX*12I*6V9FL1HX095$5:$*C3KGCM3FIS-WWRE14E:7VYNFA-3QCH5ULZJ*CRH91BTXIK-N+J1
```

Use `evtjs` we can parse this link and get its structure as below: 

```json
{
  "flag": 11,
  "segments": [
    {
      "typeKey": 42,
      "value": 1532709368
    },
    {
      "typeKey": 91,
      "value": "nd1532709365718"
    },
    {
      "typeKey": 92,
      "value": "tk3065418732.2981"
    },
    {
      "typeKey": 156,
      "value": {
        "type": "Buffer",
        "data": [
          139,
          90,
          90,
          91,
          249,
          106,
          190,
          191,
          63,
          143,
          113,
          132,
          245,
          34,
          161,
          185
        ]
      }
    }
  ],
  "publicKeys": [
    "EVT6Qz3wuRjyN6gaU3P3XRxpnEZnM4oPxortemaWDwFRvsv2FxgND",
    "EVT6MYSkiBHNDLxE6JfTmSA1FxwZCgBnBYvCo7snSQEQ2ySBtpC6s",
    "EVT7bUYEdpHiKcKT9Yi794MiwKzx5tGY3cHSh4DoCrL4B2LRjRgnt"
  ],
  "signatures": [
    "SIG_K1_K6UKhSMgMdZkm1M6JUNaK6XBGgvpVWuexhUzrg9ARgJCsWiN2A5PeH9K9YUpuE8ZArYXvSWMwBSEVh8dFhHPriQh6raEVc",
    "SIG_K1_KfdYEC6GnvgkrDPLPN4tFsTACc4nnpEopBdwBsg9fwzG8zu489hCma5gYeW3zsvabbCfMQL4vu9QVbyTHHDLjp43NCNFtD",
    "SIG_K1_K3CZKdq28aNkGwU9bL57aW45kvWj3CagGgarShLYFg8MVoTTHRbXZwPvyfBf9WN93VGXBPDLdFMmtbKA814XVvQ3QZRVJn"
  ]
}
```

In this example there are 3 signatures in it. We can infer the public keys which are used to sign on the link. It has a `flag` of 11 (1 + 2 + 8) which means `version 1`, `everiPass` and `auto destory`.
