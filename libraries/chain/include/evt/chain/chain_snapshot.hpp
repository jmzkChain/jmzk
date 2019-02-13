/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <evt/chain/exceptions.hpp>

namespace evt { namespace chain {

struct chain_snapshot_header {
    /**
     * Version history
     *   1: initial version
     */

    static constexpr uint32_t minimum_compatible_version = 1;
    static constexpr uint32_t current_version            = 1;

    uint32_t version = current_version;

    void
    validate() const {
        auto min = minimum_compatible_version;
        auto max = current_version;
        EVT_ASSERT(version >= min && version <= max,
                   snapshot_validation_exception,
                   "Unsupported version of chain snapshot: ${version}. Supported version must be between ${min} and ${max} inclusive.",
                   ("version", version)("min", min)("max", max));
    }
};

}}  // namespace evt::chain

FC_REFLECT(evt::chain::chain_snapshot_header, (version))