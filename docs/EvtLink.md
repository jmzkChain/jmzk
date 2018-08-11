# Documentation of EvtLink / everiPass / everiPay

## Intro

### What's `EvtLink`, `everiPay` and `everiPass`

`everiPass` / `everiPay` is a brand new way to do face-to-face payment or to validate ownership of a token (for example, to validate the ownership of a ticket when going through a gateway). People use their wallet app (or even a webpage) to generate a dynamic QR Code as a proof that he/she is the owner of specific token (both NFTs / FTs).

Here is a example UI of `everiPass`:

![everiPass](./pass-qr.png)

These QR Codes is encoded using a text which has a format called `EvtLink`. It is a compact format to contains all the information needed for transactions in a short text.

`EvtLink` is also used to generate `payee QR Code`.

### Highlights

#### everiPay / everiPass

everiPay/everiPass is a payment method born for face-to-face micropayments using everiToken public block chain.

everiPay/everiPass includes the standard of QR code generation and the definition of communication protocol. Based on everiToken public blockchain with five characteristics: 

- Instant Clearance, A transaction is a settlement.
- Decentralization: P2P payment, no centralized platform, no one can modify the data on chain, everyone can participate in pricing.
- Most secure: The data and content in the block chain can not be forged or tampered with, so as to maximize the protection of user's property security.
- Most Convenient: Even if you can’t connect to the Internet, you can complete the transaction. Payer / Payee doesn't need to input the amount of money manually. Payer and payee will receive notification as soon as the transaction is successful.
- Compatible: everiPay/everiPass support all Tokens supported by everiToken, not only currency but also tokens and points, even a key to open a door, and you can use it almost everywhere, just with your phone. 
- Fast: The everiToken has achieved high TPS, we think that a transaction can be completed within 1 - 3 seconds considering the situation of equipment and network.

Based on the above six characteristics, everiPay/everiPass can provide the most secure, most convenient and enjoyable services in face-to-face payments.

For `everiPay / everiPass`, payee must use a app that supports parsing EvtLink and pushing transactions to `everiToken`. It is easy as we provide easy-to-use APIs and code examples for developers. It is similar to add AliPay / WeChat support for your store, but even much easier.

#### Payee Code

`Payee Code` is a static QR Code containing the address of the token receiver. It's another way to pay tokens based on everiToken. It does not support many features comparing to everiPay, for example, payers must connect to Internet for payment, and payers & payees must input amount of money manually and they won't receive notification when payment is finished automatically.

However, payees don't need to use apps that supports this payment method. In fact, what payees needed to do is just using a wallet on their phone to check if they received the money from the payer. It is suitable for very small stores or persons.

## How do users use everiPay / everiPass

### For everiPay

1. The owner of some token (both NFTs and FTs) use his / her wallet app (or even a single webpage) to generate a series of QR codes (each of them represents a `EvtLink`). 
   - The QR code will keep changing every several seconds. Old codes will expire very soon (currently about 20 seconds).
   - The QR code is made up of current time, the id of the token he / she wants to use and owner's signature for the link.
2. Payee use a everiPass-compatible scanner / a mobile phone or any other kinds of QR code scanner to read the code and get the decoded `EvtLink` inside it. The link is then included in a transaction and pushed by the machine / phone / scanner.
3. The BP received the transaction with the `EvtLink` inside and check the signatures and then execute actions in the link.

### For everiPass

Almost the same as `everiPay` except for:

- Normally the scanner is fixedly installed on a gateway or some similar machine.
- After scanning, no transfer is executed. The chain just check the ownership of the token to make sure he / she has the permission to pass the gateway / door. Destroying the token after scanning automatically is supported and optional.

## Technical Details

### How does `EvtLink` technically work for `everiPay` / `everiPass`

everiToken public chain use `everipass` action and `everipay` action to execute the transaction of `evtLink`. it also provides a struct named `evt_link` to represent `EvtLink`. For detail information, please refer to the API / ABI documentation of `everiToken`. 

Payers don't need to send transactions directly. After payees receive the link, they sould wrap them in a a action and push them to the chain. 

After payees pushing actions to the chain, the chain nodes will synchronize the result. At the same time, the device of payers should continuously querying transaction id of the EvtLink it created by calling a API called ``

### Developing based on `EvtLink`

`evtjs` has full support for `EvtLink` and `everiPay / everiPass / payee code`. It is recommended to use `evtjs` as the groundwork to build your project.

### Process `EvtLink` manually

We also publish the structure of `EvtLink` so you can process it manually.

#### Structure of EvtLink

Each `EvtLink` has the struct as below:

```xml
[https://evt.li/]<base42segmentsStream>[_<base42signaturesList>]
```

`-base42signaturesList` is required for `everiPass / everiPay` and not necessary for `payeeCode`.

`[https://evt.li/]` is optional prefix.

We use `base42` encoding because this encoding of QR Code is very efficient.

#### base42 Encoding

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

#### Segments Stream

`Segments stream` is a binary structure which has a structure as below:

```xml
    <flag><segment><segment><segment><segment><segment>....
```

`Flag` is the place to set some proterties of the link. Different flag can be added together. So `7` means  ( 1 + 2 + 4 ). This table shows the detail:

| flag | meaning |
| ---  | --- |
|     1    |   protocol version 1 (required)
|    2    |   everiPass
|     4    |   everiPay
|     8    |   should destory the NFT after validate the token in everiPass
|      16   |   payee's QR code

Below is the struct of each `segment`:

```xml
    <typeKey><value>
```

`typeKey` is a unsigned byte.

Different `typeKey` has different `data types` for its `value`.


| from | to (included) | data type |
| --- | --- | --- |
 | 0   | 20 |   1-byte unsigned integer
 | 21 |40   | 2-byte unsigned integer (BE)
 | 41 |90  |  4-byte unsigned integer (BE)
 | 91 |155 |  string
 | 156|165 |  uuid
 | 166|180 |  byte string
 | 180|255    |  remained


Here is a brief reference of common used `typeKey` for convenient.

| typeKey | flag | description of value |
| --- | --- | --- |
| `42` | | (uint32) unix timestamp in seconds |
| `43` | | (uint32) max allowed amount for everiPay |
| `91` | | (string) domain name to be validated in everiPass |
| `92` | | (string) token name to be validated in everiPass |
| `93` | | (string) symbol name to be paid in everiPay (for example: "5,EVT") |
| `94` | | (string) max allowed amount for payment (optionl, string format remained only for amount >= 2 ^ 32) |
| `95` | | (string) public key (address) for receiving points or coins |
| `156` | | (uuid) link id(128-bit) |

#### base42signaturesList

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

#### Example

Here is an examples of valid `EvtLink`:

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
