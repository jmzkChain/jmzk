/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once
#include <functional>

#include <fc/scoped_exit.hpp>

#include <boost/dynamic_bitset.hpp>
#include <boost/range/algorithm/find.hpp>

#include <evt/chain/controller.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/contracts/types_invoker.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/producer_schedule.hpp>
#include <evt/utilities/parallel_markers.hpp>

namespace evt { namespace chain {

class authority_checker;

namespace __internal {

template<typename T>
struct check_authority {};

}  // namespace __internal

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
    const controller&               control_;
    const public_keys_type&         signing_keys_;
    const token_database&           token_db_;
    const uint32_t                  max_recursion_depth_;
    boost::dynamic_bitset<uint64_t> used_keys_;

public:
    struct weight_tally_visitor {
    public:
        weight_tally_visitor(authority_checker* checker)
            : checker_(checker) {}

        uint32_t
        operator()(const key_weight& permission) {
            return this->operator()(permission.key, permission.weight);
        }

        uint32_t
        operator()(const address& addr, const weight_type weight) {
            if(!addr.is_public_key()) {
                // will not add weight when it's not valid public key address
                return total_weight_;
            }
            return this->operator()(addr.get_public_key(), weight);
        }

        uint32_t
        operator()(const public_key_type& key, const weight_type weight) {
            auto itr = boost::find(checker_->signing_keys_, key);
            if(itr != checker_->signing_keys_.end()) {
                checker_->used_keys_[itr - checker_->signing_keys_.begin()] = true;
                total_weight_ += weight;
            }
            return total_weight_;
        }

    public:
        uint32_t total_weight() const { return total_weight_; }
        void add_weight(uint32_t weight) { total_weight_ += weight; }

    private:
        authority_checker* checker_;
        uint32_t           total_weight_ = 0;
    };

public:
    template<typename> friend struct __internal::check_authority;

public:
    authority_checker(const controller& control, const public_keys_type& signing_keys, const token_database& token_db, uint32_t max_recursion_depth)
        : control_(control)
        , signing_keys_(signing_keys)
        , token_db_(token_db)
        , max_recursion_depth_(max_recursion_depth)
        , used_keys_(signing_keys.size(), false) {}

private:
    void
    get_domain_permission(const domain_name& domain_name, const permission_name name, std::function<void(const permission_def&)>&& cb) {
        domain_def domain;
        token_db_.read_domain(domain_name, domain);
        if(name == N(issue)) {
            cb(domain.issue);
        }
        else if(name == N(transfer)) {
            cb(domain.transfer);
        }
        else if(name == N(manage)) {
            cb(domain.manage);
        }
    }

    void
    get_fungible_permission(const symbol_id_type sym_id, const permission_name name, std::function<void(const permission_def&)>&& cb) {
        fungible_def fungible;
        token_db_.read_fungible(sym_id, fungible);
        if(name == N(issue)) {
            cb(fungible.issue);
        }
        else if(name == N(manage)) {
            cb(fungible.manage);
        }
    }

    void
    get_group(const group_name& name, std::function<void(const group_def&)>&& cb) {
        group_def group;
        token_db_.read_group(name, group);
        cb(group);
    }

    void
    get_owner(const domain_name& domain, const name128& name, std::function<void(const address_list&)>&& cb) {
        token_def token;
        token_db_.read_token(domain, name, token);
        cb(token.owner);
    }

    void
    get_suspend(const proposal_name& proposal, std::function<void(const suspend_def&)>&& cb) {
        suspend_def suspend;
        token_db_.read_suspend(proposal, suspend);
        cb(suspend);
    }

    void
    get_producer_key(const account_name& producer_name, std::function<void(const public_key_type&)>&& cb) {
        auto& sche = control_.active_producers();
        for(auto& p : sche.producers) {
            if(p.producer_name == producer_name) {
                cb(p.block_signing_key);
                return;
            }
        }
        return;
    } 

private:
    bool
    satisfied_node(const group& group, const group::node& node, uint32_t depth) {
        FC_ASSERT(depth < max_recursion_depth_);
        FC_ASSERT(!node.is_leaf());
        auto vistor = weight_tally_visitor(this);
        group.visit_node(node, [&](const auto& n) {
            FC_ASSERT(!n.is_root());
            if(n.is_leaf()) {
                vistor(group.get_leaf_key(n), n.weight);
            }
            else {
                if(satisfied_node(group, n, depth + 1)) {
                    vistor.add_weight(n.weight);
                }
            }
            if(vistor.total_weight() >= node.threshold) {
                return false;  // no need to visit more nodes
            }
            return true;
        });
        if(vistor.total_weight() >= node.threshold) {
            return true;
        }
        return false;
    }

    bool
    satisfied_group(const group_name& name) {
        bool result = false;
        get_group(name, [&](const auto& group) {
            if(satisfied_node(group, group.root(), 0)) {
                result = true;
            }
        });
        return result;
    }

    bool
    satisfied_permission(const permission_def& permission, const action& action) {
        uint32_t total_weight = 0;
        for(const auto& aw : permission.authorizers) {
            auto& ref        = aw.ref;
            bool  ref_result = false;

            switch(ref.type()) {
            case authorizer_ref::account_t: {
                auto  vistor = weight_tally_visitor(this);
                auto& key    = ref.get_account();
                if(vistor(key, 1) == 1) {
                    ref_result = true;
                }
                break;
            }
            case authorizer_ref::owner_t: {
                get_owner(action.domain, action.key, [&](const auto& owner) {
                    auto vistor = weight_tally_visitor(this);
                    for(const auto& o : owner) {
                        vistor(o, 1);
                    }
                    if(vistor.total_weight() == owner.size()) {
                        ref_result = true;
                    }
                });
                break;
            }
            case authorizer_ref::group_t: {
                auto& name = ref.get_group();
                ref_result = satisfied_group(name);
                break;
            }
            }  // switch

            if(ref_result) {
                total_weight += aw.weight;
                if(total_weight >= permission.threshold) {
                    return true;
                }
            }
        }
        return false;
    }

    bool
    satisfied_domain_permission(const action& action, const permission_name& name) {
        bool result = false;
        get_domain_permission(action.domain, name, [&](const auto& permission) {
            result = satisfied_permission(permission, action);
        });
        return result;
    }

    bool
    satisfied_fungible_permission(const symbol_id_type sym_id, const action& action, const permission_name& name) {
        bool result = false;
        get_fungible_permission(sym_id, name, [&](const auto& permission) {
            result = satisfied_permission(permission, action);
        });
        return result;
    }

public:
    bool
    satisfied(const action& act) {
        using namespace __internal;

        // Save the current used keys; if we do not satisfy this authority, the newly used keys aren't actually used
        auto KeyReverter = fc::make_scoped_exit([this, keys = used_keys_]() mutable {
            used_keys_ = keys;
        });

        bool result = types_invoker<bool, check_authority>::invoke(act.name, act, this);

        if(result) {
            KeyReverter.cancel();
            return true;
        }
        return false;
    }

    bool
    all_keys_used() const { return used_keys_.all(); }

    public_keys_type
    used_keys() const {
        auto range = utilities::filter_data_by_marker(signing_keys_, used_keys_, true);
        return range;
    }

    public_keys_type
    unused_keys() const {
        auto range = utilities::filter_data_by_marker(signing_keys_, used_keys_, false);
        return range;
    }

    const controller&
    get_control() const {
        return control_;
    }
};  /// authority_checker

namespace __internal {

using namespace contracts;

template<>
struct check_authority<newdomain> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& nd     = act.data_as<const newdomain&>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(nd.creator, 1) == 1) {
                return true;
            }
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `newdomain` type.");
        return false;
    }
};

template<>
struct check_authority<issuetoken> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_domain_permission(act, N(issue));
    }
};

template<>
struct check_authority<transfer> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_domain_permission(act, N(transfer));
    }
};

template<>
struct check_authority<destroytoken> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_domain_permission(act, N(transfer));
    }
};

template<>
struct check_authority<newgroup> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& ng = act.data_as<const newgroup&>();
            if(ng.group.key().is_reserved()) {
                // if group key is reserved, no need to check authority
                return true;
            }

            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(ng.group.key(), 1) == 1) {
                return true;
            }
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `newgroup` type.");
        return false;
    }
};

template<>
struct check_authority<updategroup> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        bool result = false;
        checker->get_group(act.key, [&](const auto& group) {
            auto& gkey   = group.key();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(gkey, 1) == 1) {
                result = true;
            }
        });
        return result;
    }
};

template<>
struct check_authority<updatedomain> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_domain_permission(act, N(manage));
    }
};

template<>
struct check_authority<newfungible> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& nf    = act.data_as<const newfungible&>();
            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(nf.creator, 1) == 1) {
                return true;
            }
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transation data is not valid, data cannot cast to `newfungible` type");
        return false;
    }
};

symbol_id_type
get_symbol_id(const name128& key) {
    auto str = (std::string)key;
    return (symbol_id_type)std::stoul(str);
}

template<>
struct check_authority<issuefungible> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_fungible_permission(get_symbol_id(act.key), act, N(issue));
    }
};

template<>
struct check_authority<updfungible> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_fungible_permission(get_symbol_id(act.key), act, N(manage));
    }
};

template<>
struct check_authority<transferft> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& tf    = act.data_as<const transferft&>();
            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(tf.from, 1) == 1) {
                return true;
            }
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transation data is not valid, data cannot cast to `transferft` type");
        return false;
    }
};

template<>
struct check_authority<recycleft> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& rf    = act.data_as<const recycleft&>();
            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(rf.address, 1) == 1) {
                return true;
            }
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transation data is not valid, data cannot cast to `recycleft` type");
        return false;
    }
};

template<>
struct check_authority<destroyft> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& rf    = act.data_as<const destroyft&>();
            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(rf.address, 1) == 1) {
                return true;
            }
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transation data is not valid, data cannot cast to `destroyft` type");
        return false;
    }
};


template<>
struct check_authority<evt2pevt> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& ep    = act.data_as<const evt2pevt&>();
            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(ep.from, 1) == 1) {
                return true;
            }
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transation data is not valid, data cannot cast to `transferft` type");
        return false;
    }
};

template<>
struct check_authority<newsuspend> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& ns    = act.data_as<const newsuspend&>();
            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(ns.proposer, 1) == 1) {
                return true;
            }
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `newsuspend` type.");
        return false;
    }
};

template<>
struct check_authority<aprvsuspend> {
    static bool
    invoke(const action&, authority_checker*) {
        // will check signatures when applying
        return true;
    }
};

template<>
struct check_authority<cancelsuspend> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        bool result = false;
        checker->get_suspend(act.key, [&](const auto& suspend) {
            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(suspend.proposer, 1) == 1) {
                result = true;
            }
        });
        return result;
    }
};

template<>
struct check_authority<execsuspend> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& es    = act.data_as<const execsuspend&>();
            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(es.executor, 1) == 1) {
                return true;
            }
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `execsuspend` type.");
        return false;
    }
};

template<>
struct check_authority<addmeta> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& am = act.data_as<const addmeta&>();

            auto& ref        = am.creator;
            bool  ref_result = false;

            switch(ref.type()) {
            case authorizer_ref::account_t: {
                auto  vistor = authority_checker::weight_tally_visitor(checker);
                auto& key    = ref.get_account();
                if(vistor(key, 1) == 1) {
                    ref_result = true;
                }
                break;
            }
            case authorizer_ref::owner_t: {
                return false;
            }
            case authorizer_ref::group_t: {
                auto& name = ref.get_group();
                checker->get_group(name, [&](const auto& group) {
                    if(checker->satisfied_node(group, group.root(), 0)) {
                        ref_result = true;
                    }
                });
                break;
            }
            }  // switch
            return ref_result;
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `addmeta` type.");
    }
};

template<>
struct check_authority<everipass> {
    static bool
    invoke(const action&, authority_checker*) {
        // check authority when apply
        return true;
    }
};

template<>
struct check_authority<everipay> {
    static bool
    invoke(const action&, authority_checker*) {
        // check authority when apply
        return true;
    }
};

template<>
struct check_authority<prodvote> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& pv = act.data_as<const prodvote&>();

            bool result = false;
            checker->get_producer_key(pv.producer, [&](auto& key) {
                auto vistor = authority_checker::weight_tally_visitor(checker);
                if(vistor(key, 1) == 1) {
                    result = true;
                }
            });
            return result;
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `prodvote` type.");
    }
};

template<>
struct check_authority<updsched> {
    static bool
    invoke(const action&, authority_checker* checker) {
        return checker->satisfied_group(checker->get_control().get_genesis_state().evt_org.name());
    }
};

template<>
struct check_authority<newlock> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& nl     = act.data_as<const newlock&>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(nl.proposer, 1) == 1) {
                return true;
            }
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `newlock` type.");
        return false;
    }
};

template<>
struct check_authority<aprvlock> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& al     = act.data_as<const aprvlock&>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(al.approver, 1) == 1) {
                return true;
            }
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `aprvlock` type.");
        return false;
    }
};

template<>
struct check_authority<tryunlock> {
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& tl     = act.data_as<const tryunlock&>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(tl.executor, 1) == 1) {
                return true;
            }
        }
        EVT_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `tryunlock` type.");
        return false;
    }
};

template<>
struct check_authority<paycharge> {
    static bool
    invoke(const action&, authority_checker*) {
        // not allowed user to use this action
        return false;
    }
};

}  // namespace __internal

}}  // namespace evt::chain
