/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <functional>

#include <evt/chain/types.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/utilities/parallel_markers.hpp>

#include <fc/scoped_exit.hpp>

#include <boost/range/algorithm/find.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>

namespace evt { namespace chain {

using namespace evt::chain::contracts;

/**
* @brief This class determines whether a set of signing keys are sufficient to satisfy an authority or not
*
* To determine whether an authority is satisfied or not, we first determine which keys have approved of a message, and
* then determine whether that list of keys is sufficient to satisfy the authority. This class takes a list of keys and
* provides the @ref satisfied method to determine whether that list of keys satisfies a provided authority.
*
*/
class authority_checker {
private:
    const flat_set<public_key_type>& _signing_keys;
    const token_database&            _token_db;
    const uint32_t                   _max_recursion_depth;
    vector<bool>                     _used_keys;

    struct weight_tally_visitor {
        using result_type = uint32_t;

        authority_checker& checker;
        uint32_t total_weight = 0;

        weight_tally_visitor(authority_checker& checker)
           : checker(checker) {}

        uint32_t operator()(const key_weight& permission) {
            return this->operator()(permission.key, permission.weight);
        }

        uint32_t operator()(const public_key_type& key, const weight_type weight) {
            auto itr = boost::find(checker._signing_keys, key);
            if (itr != checker._signing_keys.end()) {
                checker._used_keys[itr - checker._signing_keys.begin()] = true;
                total_weight += weight;
            }
            return total_weight;
        }
    };

public:
    authority_checker(const flat_set<public_key_type>& signing_keys, const token_database& token_db, uint32_t max_recursion_depth)
      : _signing_keys(signing_keys),
        _token_db(token_db),
        _max_recursion_depth(max_recursion_depth),
        _used_keys(signing_keys.size(), false)
    {}

private:
    void
    get_permission(const domain_name& domain, const action_name name, std::function<void(const permission_def&)>&& cb) {
        _token_db.read_domain(domain, [&](const auto& domain) {
            if(name == N(issuetoken)) {
                cb(domain.issue);
            }
            else if(name == N(transfer)) {
                cb(domain.transfer);
            }
            else if(name == N(updatedomain)) {
                cb(domain.manage);
            }
        });
    }

    void
    get_group(const group_id& id, std::function<void(const group_def&)>&& cb) {
        _token_db.read_group(id, cb);
    }

    void
    get_owner(const domain_name& domain, const name128& name, std::function<void(const user_list&)>&& cb) {
        if(domain == N128(account)) {
            _token_db.read_account(name, [&](const auto& account) { cb(account.owner); });
        }
        else {
            _token_db.read_token(domain, name, [&](const auto& token) { cb(token.owner); });
        }
    }

private:
    bool
    satisfied_domain(const action& action) {
        if(action.name == N(newdomain)) {
            try {
                auto nd = action.data_as<contracts::newdomain>();
                weight_tally_visitor vistor(*this);
                if(vistor(nd.issuer, 1) == 1) {
                    return true;
                }
            } EVT_RETHROW_EXCEPTIONS(chain_type_exception, "transaction data is not valid, data cannot cast to `newdomain` type.");
        }
        else if(action.name == N(updatedomain)) {
            return satisfied_permission(action, action.key);
        }
        return false;
    }

    bool
    satisfied_group(const action& action) {
        if(action.name == N(newgroup)) {
            try {
                auto ng = action.data_as<contracts::newgroup>();
                weight_tally_visitor vistor(*this);
                if(vistor(ng.group.key(), 1) == 1) {
                    return true;
                }
            } EVT_RETHROW_EXCEPTIONS(chain_type_exception, "transaction data is not valid, data cannot cast to `newgroup` type.");
        }
        else if(action.name == N(updategroup)) {
            bool result = false;
            auto gid = group_id(action.key);
            get_group(gid, [&](const auto& group) {
                auto& gkey = group.key();
                weight_tally_visitor vistor(*this);
                if(vistor(gkey, 1) == 1) {
                    result = true;
                }
            });
            return result;
        }
        return false;
    }

    bool
    satisfied_account(const action& action) {
        if(action.name == N(newaccount)) {
            try {
                auto na = action.data_as<contracts::newaccount>();
                weight_tally_visitor vistor(*this);
                for(auto& o : na.owner) {
                    vistor(o, 1);
                }
                if(vistor.total_weight == na.owner.size()) {
                    return true;
                }
            } EVT_RETHROW_EXCEPTIONS(chain_type_exception, "transation data is not valid, data cannot cast to `newaccount` type")
        }
        else if(action.name == N(updateowner)) {
            bool result = false;
            get_owner(action.domain, (name128)action.key, [&](const auto& owner) {
                weight_tally_visitor vistor(*this);
                for(auto& o : owner) {
                    vistor(o, 1);
                }
                if(vistor.total_weight == owner.size()) {
                    result = true;
                }
            });
            return result;
        }
        else if(action.name == N(transferevt)) {
            bool result = false;
            try {
                auto te = action.data_as<contracts::transferevt>();
                get_owner(N128(account), te.from, [&](const auto& owner) {
                    weight_tally_visitor vistor(*this);
                    for(auto& o : owner) {
                        vistor(o, 1);
                    }
                    if(vistor.total_weight == owner.size()) {
                        result = true;
                    }
                });
            } EVT_RETHROW_EXCEPTIONS(chain_type_exception, "transation data is not valid, data cannot cast to `transferevt` type")
            return result;
        }
        return false;
    }

    bool
    satisfied_node(const group& group, const group::node& node, uint32_t depth) {
        FC_ASSERT(depth < _max_recursion_depth);
        FC_ASSERT(!node.is_leaf());
        weight_tally_visitor vistor(*this);
        group.visit_node(node, [&](const auto& n) {
            FC_ASSERT(!n.is_root());
            if(n.is_leaf()) {
                vistor(group.keys_[n.index], n.weight);
            }
            else {
                if(satisfied_node(group, n, depth + 1)) {
                    vistor.total_weight += n.weight;
                }
            }
            if(vistor.total_weight >= node.threshold) {
                return false;  // no need to visit more nodes
            }
            return true;
        });
        if(vistor.total_weight >= node.threshold) {
            return true;
        }
        return false;
    }

    bool
    satisfied_permission(const action& action, const domain_name& domain) {
        bool result = false;
        get_permission(domain, action.name, [&](const auto& permission) {
            uint32_t total_weight = 0;
            for(const auto& aw : permission.authorizers) {
                auto& ref = aw.ref;
                bool ref_result = false;
                if(ref.is_account_ref()) {
                    weight_tally_visitor vistor(*this);
                    auto& key = ref.get_account();
                    if(vistor(key, 1) == 1) {
                        ref_result = true;
                    }
                }
                else {
                    FC_ASSERT(ref.is_group_ref());
                    auto& gid = ref.get_group();
                    if(gid.empty()) {
                        // owner group
                        get_owner(domain, (name128)action.key, [&](const auto& owner) {
                            weight_tally_visitor vistor(*this);
                            for (const auto& o : owner) {
                                vistor(o, 1);
                            }
                            if (vistor.total_weight == owner.size()) {
                                ref_result = true;
                            }
                        });
                    }
                    else {
                        get_group(gid, [&](const auto& group) {
                            if(satisfied_node(group, group.root(), 0)) {
                              ref_result = true;
                            }
                        });
                    }
                }
                if(ref_result) {
                    total_weight += aw.weight;
                    if(total_weight >= permission.threshold) {
                        result = true;
                        return;
                    }
                }
            }
        });
        return result;
    }

    bool
    satisfied_token(const action& action) {
        return satisfied_permission(action, action.domain);
    }

public:
    bool
    satisfied(const action& action) {
      // Save the current used keys; if we do not satisfy this authority, the newly used keys aren't actually used
      auto KeyReverter = fc::make_scoped_exit([this, keys = _used_keys] () mutable {
         _used_keys = keys;
      });
      bool result = false;

      if(action.domain == N128(domain)) {
          result = satisfied_domain(action);
      }
      else if(action.domain == N128(group)) {
          result = satisfied_group(action);
      }
      else if(action.domain == N128(account)) {
          result = satisfied_account(action);
      }
      else {
          result = satisfied_token(action);
      }
      if(result) {
          KeyReverter.cancel();
          return true;
      }
      return false;
    }

    bool all_keys_used() const { return boost::algorithm::all_of_equal(_used_keys, true); }

    flat_set<public_key_type>
    used_keys() const {
      auto range = utilities::filter_data_by_marker(_signing_keys, _used_keys, true);
      return {range.begin(), range.end()};
    }

    flat_set<public_key_type>
    unused_keys() const {
      auto range = utilities::filter_data_by_marker(_signing_keys, _used_keys, false);
      return {range.begin(), range.end()};
    }
}; /// authority_checker

}} // namespace evt::chain
