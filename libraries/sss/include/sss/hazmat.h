/*
 * Low level API for Daan Sprenkels' Shamir secret sharing library
 * Copyright (c) 2017 Daan Sprenkels <hello@dsprenkels.com>
 *
 * Usage of this API is hazardous and is only reserved for beings with a
 * good understanding of the Shamir secret sharing scheme and who know how
 * crypto code is implemented. If you are unsure about this, use the
 * intermediate level API. You have been warned!
 */


#ifndef sss_HAZMAT_H_
#define sss_HAZMAT_H_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define sss_KEYSHARE_LEN 33 /* 1 + 32 */


/*
 * One share of a cryptographic key which is shared using Shamir's
 * the `sss_create_keyshares` function.
 */
typedef uint8_t sss_Keyshare[sss_KEYSHARE_LEN];


/*
 * Share the secret given in `key` into `n` shares with a treshold value given
 * in `k`. The resulting shares are written to `out`.
 *
 * The share generation that is done in this function is only secure if the key
 * that is given is indeed a cryptographic key. This means that it should be
 * randomly and uniformly generated string of 32 bytes.
 *
 * Also, for performance reasons, this function assumes that both `n` and `k`
 * are *public* values.
 *
 * If you are looking for a function that *just* creates shares of arbitrary
 * data, you should use the `sss_create_shares` function in `sss.h`.
 */
void sss_create_keyshares(sss_Keyshare *out,
                          const uint8_t key[32],
                          uint8_t n,
                          uint8_t k);


/*
 * Combine the `k` shares provided in `shares` and write the resulting key to
 * `key`. The amount of shares used to restore a secret may be larger than the
 * threshold needed to restore them.
 *
 * This function does *not* do *any* checking for integrity. If any of the
 * shares not original, this will result in an invalid resored value.
 * All values written to `key` should be treated as secret. Even if some of the
 * shares that were provided as input were incorrect, the resulting key *still*
 * allows an attacker to gain information about the real key.
 *
 * This function treats `shares` and `key` as secret values. `k` is treated as
 * a public value (for performance reasons).
 *
 * If you are looking for a function that combines shares of arbitrary
 * data, you should use the `sss_combine_shares` function in `sss.h`.
 */
void sss_combine_keyshares(uint8_t key[32],
                           const sss_Keyshare *shares,
                           uint8_t k);

#ifdef __cplusplus
}
#endif

#endif /* sss_HAZMAT_H_ */
