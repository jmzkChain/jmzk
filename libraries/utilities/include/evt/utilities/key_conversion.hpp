/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

#include <optional>
#include <string>
#include <fc/crypto/elliptic.hpp>

namespace jmzk { namespace utilities {

std::string                         key_to_wif(const fc::sha256& private_secret);
std::string                         key_to_wif(const fc::ecc::private_key& key);
std::optional<fc::ecc::private_key> wif_to_key(const std::string& wif_key);

}} // end namespace jmzk::utilities
