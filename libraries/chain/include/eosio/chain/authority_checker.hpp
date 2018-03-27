/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/contracts/types.hpp>

#include <eosio/utilities/parallel_markers.hpp>

#include <fc/scoped_exit.hpp>

#include <boost/range/algorithm/find.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>

namespace eosio { namespace chain {

   /**
    * @brief This class determines whether a set of signing keys are sufficient to satisfy an authority or not
    *
    * To determine whether an authority is satisfied or not, we first determine which keys have approved of a message, and
    * then determine whether that list of keys is sufficient to satisfy the authority. This class takes a list of keys and
    * provides the @ref satisfied method to determine whether that list of keys satisfies a provided authority.
    *
    * @tparam F A callable which takes a single argument of type @ref AccountPermission and returns the corresponding
    * authority
    */
   template<typename GetPermissionFunc, typename GetGroupFunc, typename GetOwnerFunc>
   class authority_checker {
      private:
         vector<public_key_type>    signing_keys;
         vector<bool>               _used_keys;
         GetPermissionFunc          _get_permission_func;
         GetGroupFunc               _get_group_func;
         GetOwnerFunc               _get_owner_func;

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
               auto itr = boost::find(checker.signing_keys, key);
               if (itr != checker.signing_keys.end()) {
                  checker._used_keys[itr - checker.signing_keys.begin()] = true;
                  total_weight += weight;
               }
               return total_weight;
            }
         };

      public:
         authority_checker( const flat_set<public_key_type>& signing_keys, GetPermissionFunc&& gpf, GetGroupFunc&& ggf, GetOwnerFunc&& gof)
            :  signing_keys(signing_keys.begin(), signing_keys.end()),
              _used_keys(signing_keys.size(), false),
              _get_permission_func(std::forward<GetPermissionFunc>(gpf)),
              _get_group_func(std::forward<GetGroupFunc>(ggf)),
              _get_owner_func(std::forward<GetOwnerFunc>(gof))
         {}

         bool satisfied(const action& action) {
            // Save the current used keys; if we do not satisfy this authority, the newly used keys aren't actually used
            auto KeyReverter = fc::make_scoped_exit([this, keys = _used_keys] () mutable {
               _used_keys = keys;
            });
            
            bool result = false;
            _get_permission_func(action.domain, action.name, [&](const auto& permission) {
                uint32_t total_weight = 0;
                for(const auto& group_ref : permission.groups) {
                    bool gresult = false;
                    if(group_ref.id == 0) {
                        // is owner group, special handle this
                        _get_owner_func(action.domain, action.key, [&](const auto& owners) {
                            weight_tally_visitor vistor(*this);
                            for(const auto& owner : owners) {
                                vistor(owner, 1);
                            }
                            if(vistor.total_weight == owners.size()) {
                                gresult = true;
                                return;
                            }
                        });
                    }
                    else {
                        // normal group
                        _get_group_func(group_ref.id, [&](const auto& group) {
                            weight_tally_visitor vistor(*this);
                            for(const auto& kw : group.keys) {
                                if(vistor(kw) >= group.threshold) {
                                    gresult = true;
                                    return;
                                }
                            }
                        });
                    }
                    if(gresult) {
                        total_weight += group_ref.weight;
                        if(total_weight >= permission.threshold) {
                            result = true;
                            return;
                        }
                    }
                }
            });
            if(result) {
                KeyReverter.cancel();
                return true;
            }
            return false;
         }

         bool all_keys_used() const { return boost::algorithm::all_of_equal(_used_keys, true); }

         flat_set<public_key_type> used_keys() const {
            auto range = utilities::filter_data_by_marker(signing_keys, _used_keys, true);
            return {range.begin(), range.end()};
         }
         flat_set<public_key_type> unused_keys() const {
            auto range = utilities::filter_data_by_marker(signing_keys, _used_keys, false);
            return {range.begin(), range.end()};
         }
   }; /// authority_checker

   template<typename GetPermissionFunc, typename GetGroupFunc, typename GetOwnerFunc>
   auto make_auth_checker(const flat_set<public_key_type>& signing_keys, GetPermissionFunc&& gpf, GetGroupFunc&& ggf, GetOwnerFunc&& gof) {
      return authority_checker<GetPermissionFunc, GetGroupFunc, GetOwnerFunc>
        (signing_keys, std::forward<GetPermissionFunc>(gpf), std::forward<GetGroupFunc>(ggf), std::forward<GetOwnerFunc>(gof));
   }

} } // namespace eosio::chain
