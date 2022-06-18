/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <functional>

#include <fc/scoped_exit.hpp>

#include <boost/dynamic_bitset.hpp>
#include <boost/range/algorithm/find.hpp>

#include <jmzk/chain/controller.hpp>
#include <jmzk/chain/config.hpp>
#include <jmzk/chain/execution_context_impl.hpp>
#include <jmzk/chain/token_database_cache.hpp>
#include <jmzk/chain/types.hpp>
#include <jmzk/chain/producer_schedule.hpp>
#include <jmzk/chain/contracts/types.hpp>
#include <jmzk/chain/contracts/lua_engine.hpp>
#include <jmzk/utilities/parallel_markers.hpp>

namespace jmzk { namespace chain {

using namespace contracts;

class authority_checker;

namespace internal {

enum permission_type { kIssue = 0, kTransfer, kManage, kWithdraw };
enum toke_type { kNFT = 0, kFT };

template<uint64_t>
struct check_authority {};

#define READ_DB_TOKEN(TYPE, PREFIX, KEY, VPTR, EXCEPTION, FORMAT, ...)       \
    try {                                                                    \
        using vtype = typename decltype(VPTR)::element_type;                 \
        VPTR = tokendb_cache_.template read_token<vtype>(TYPE, PREFIX, KEY); \
    }                                                                        \
    catch(token_database_exception&) {                                       \
        jmzk_THROW2(EXCEPTION, FORMAT, __VA_ARGS__);                          \
    }

}  // namespace internal

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
    const controller&            control_;
    const jmzk_execution_context& exec_ctx_;
    const public_keys_set&       signing_keys_;
    const uint32_t               max_recursion_depth_;

    token_database_cache&           tokendb_cache_;
    boost::dynamic_bitset<uint64_t> used_keys_;
    bool                            check_script_;

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
        uint32_t add_weight(uint32_t weight) { total_weight_ += weight; return total_weight_; }

    private:
        authority_checker* checker_;
        uint32_t           total_weight_ = 0;
    };

public:
    template<uint64_t> friend struct internal::check_authority;

public:
    authority_checker(const controller& control, const jmzk_execution_context& exec_ctx, const public_keys_set& signing_keys, uint32_t max_recursion_depth, bool check_script = true)
        : control_(control)
        , exec_ctx_(exec_ctx)
        , signing_keys_(signing_keys)
        , max_recursion_depth_(max_recursion_depth)
        , tokendb_cache_(control.token_db_cache())
        , used_keys_(signing_keys.size(), false)
        , check_script_(check_script) {}

private:
    template<int Permission>
    void
    get_domain_permission(const domain_name& domain_name, std::function<void(const permission_def&)>&& cb) {
        using namespace internal;

        auto domain = make_empty_cache_ptr<domain_def>();
        READ_DB_TOKEN(token_type::domain, std::nullopt, domain_name, domain, unknown_domain_exception, "Cannot find domain: {}", domain_name);

        if constexpr(Permission == kIssue) {
            cb(domain->issue);
        }
        else if constexpr(Permission == kTransfer) {
            cb(domain->transfer);
        }
        else if constexpr(Permission == kManage) {
            cb(domain->manage);
        }
    }

    template<int Permission>
    void
    get_fungible_permission(const symbol_id_type sym_id, std::function<void(const permission_def&)>&& cb) {
        using namespace internal;

        auto fungible = make_empty_cache_ptr<fungible_def>();
        READ_DB_TOKEN(token_type::fungible, std::nullopt, sym_id, fungible, unknown_fungible_exception, "Cannot find fungible with symbol id: {}", sym_id);

        if constexpr(Permission == kIssue) {
            cb(fungible->issue);
        }
        else if constexpr(Permission == kTransfer) {
            cb(fungible->transfer);
        }
        else if constexpr(Permission == kManage) {
            cb(fungible->manage);
        }
    }

    template<int Permission>
    void
    get_validator_permission(const account_name& validator_name, std::function<void(const permission_def&)>&& cb) {
        using namespace internal;

        auto validator = make_empty_cache_ptr<validator_def>();
        READ_DB_TOKEN(token_type::validator, std::nullopt, validator_name, validator, unknown_validator_exception, "Cannot find validator: {}", validator_name);

        if constexpr(Permission == kWithdraw) {
            cb(validator->withdraw);
        }
        else if constexpr(Permission == kManage) {
            cb(validator->manage);
        }
    }

    void
    get_group(const group_name& name, std::function<void(const group_def&)>&& cb) {
        auto group = make_empty_cache_ptr<group_def>();
        READ_DB_TOKEN(token_type::group, std::nullopt, name, group, unknown_group_exception, "Cannot find group: {}", name);

        cb(*group);
    }

    void
    get_nft_owners(const domain_name& domain, const name128& name, std::function<void(const address_list&)>&& cb) {
        auto token = make_empty_cache_ptr<token_def>();
        READ_DB_TOKEN(token_type::token, domain, name, token, unknown_token_exception, "Cannot find token: {} in {}", name, domain);

        cb(token->owner);
    }

    void
    get_ft_owner(const action& act, std::function<void(const address_type&)>&& cb) {
        auto ds   = fc::datastream<char*>((char*)act.data.data(), act.data.size());
        auto addr = address_type();
        fc::raw::unpack(ds, addr);

        cb(addr);
    }

    void
    get_suspend(const proposal_name& proposal, std::function<void(const suspend_def&)>&& cb) {
        auto suspend = make_empty_cache_ptr<suspend_def>();
        READ_DB_TOKEN(token_type::suspend, std::nullopt, proposal, suspend, unknown_suspend_exception, "Cannot find suspend proposal: {}", proposal);

        cb(*suspend);
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

    void
    get_validator(const account_name& validator_name, std::function<void(const validator_def&)>&& cb) {
        auto validator = make_empty_cache_ptr<validator_def>();
        READ_DB_TOKEN(token_type::validator, std::nullopt, validator_name, validator, unknown_validator_exception, "Cannot find validator: {}", validator_name);

        cb(*validator);
    }

    void
    get_script(const script_name& script_name, std::function<void(const script_def&)>&& cb) {
        auto script = make_empty_cache_ptr<script_def>();
        READ_DB_TOKEN(token_type::script, std::nullopt, script_name, script, unknown_script_exception, "Cannot find script: {}", script_name);

        cb(*script);
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

    template<int Token>
    bool
    satisfied_permission(const permission_def& permission, const action& action) {
        using namespace internal;

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
                auto visitor = weight_tally_visitor(this);
                auto visit = [this, &visitor](auto& addr) {
                    switch(addr.type()) {
                    case address_type::public_key_t: {
                        return visitor(addr.get_public_key(), 1);
                    }
                    case address_type::generated_t: {
                        if(addr.get_prefix() == N(.group)) {
                            if(this->satisfied_group(addr.get_key())) {
                                return visitor.add_weight(1);
                            }
                        }
                        break;
                    }
                    }  // switch
                    return visitor.total_weight();
                };

                if constexpr(Token == kNFT) {
                    get_nft_owners(action.domain, action.key, [&](const auto& owners) {
                        for(const auto& o : owners) {
                            visit(o);
                        }
                        if(visitor.total_weight() == owners.size()) {
                            ref_result = true;
                        }
                    });
                }
                else {
                    assert(Token == kFT);
                    get_ft_owner(action, [&](const auto& owner) {
                        if(visit(owner) == 1) {
                            ref_result = true;
                        }
                    });
                }
                break;
            }
            case authorizer_ref::group_t: {
                auto& name = ref.get_group();
                ref_result = satisfied_group(name);
                break;
            }
            case authorizer_ref::script_t: {
                if(!check_script_) {
                    ref_result = true;
                    break;
                }
                auto& name   = ref.get_script();
                auto  engine = lua_engine();
                ref_result   = engine.invoke_filter(control_, action, name);
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

    template<int Permission>
    bool
    satisfied_domain_permission(const action& action) {
        using namespace internal;

        bool result = false;
        get_domain_permission<Permission>(action.domain, [&](const auto& permission) {
            result = satisfied_permission<kNFT>(permission, action);
        });
        return result;
    }

    template<int Permission>
    bool
    satisfied_fungible_permission(const symbol_id_type sym_id, const action& action) {
        using namespace internal;

        bool result = false;
        get_fungible_permission<Permission>(sym_id, [&](const auto& permission) {
            result = satisfied_permission<kFT>(permission, action);
        });
        return result;
    }

    template<int Permission>
    bool
    satisfied_validator_permission(const account_name& validator, const action& action) {
        using namespace internal;

        bool result = false;
        get_validator_permission<Permission>(validator, [&](const auto& permission) {
            result = satisfied_permission<kFT>(permission, action);
        });
        return result;
    }

    bool
    satisfied_script(const script_name& name, const action& action) {
        auto engine = lua_engine();
        return engine.invoke_filter(control_, action, name);
    }

public:
    bool
    satisfied(const action& act) {
        using namespace internal;

        // Save the current used keys; if we do not satisfy this authority, the newly used keys aren't actually used
        auto KeyReverter = fc::make_scoped_exit([this, keys = used_keys_]() mutable {
            used_keys_ = keys;
        });

        if(act.index_ == -1) {
            act.index_ = exec_ctx_.index_of(act.name);
        }
        bool result = exec_ctx_.invoke<check_authority, bool>(act.index_, act, this);

        if(result) {
            KeyReverter.cancel();
            return true;
        }
        return false;
    }

    bool
    satisfied_key(const public_key_type& pkey) {
        using namespace internal;

        // Save the current used keys; if we do not satisfy this authority, the newly used keys aren't actually used
        auto KeyReverter = fc::make_scoped_exit([this, keys = used_keys_]() mutable {
            used_keys_ = keys;
        });

        auto vistor = weight_tally_visitor(this);
        if(vistor(pkey, 1) > 0) {
            KeyReverter.cancel();
            return true;
        }
        return false;
    }

    bool
    all_keys_used() const { return used_keys_.all(); }

    public_keys_set
    used_keys() const {
        auto range = utilities::filter_data_by_marker(signing_keys_, used_keys_, true);
        return range;
    }

    public_keys_set
    unused_keys() const {
        auto range = utilities::filter_data_by_marker(signing_keys_, used_keys_, false);
        return range;
    }

    const controller&
    get_control() const {
        return control_;
    }
};  /// authority_checker

namespace internal {

using namespace contracts;

template<>
struct check_authority<N(newdomain)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& nd     = act.data_as<add_clr_t<Type>>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(nd.creator, 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `newdomain` type.");
        return false;
    }
};

template<>
struct check_authority<N(issuetoken)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_domain_permission<kIssue>(act);
    }
};

template<>
struct check_authority<N(transfer)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_domain_permission<kTransfer>(act);
    }
};

template<>
struct check_authority<N(destroytoken)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_domain_permission<kTransfer>(act);
    }
};

template<>
struct check_authority<N(newgroup)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& ng = act.data_as<add_clr_t<Type>>();
            if(ng.group.key().is_reserved()) {
                // if group key is reserved, no need to check authority
                return true;
            }

            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(ng.group.key(), 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `newgroup` type.");
        return false;
    }
};

template<>
struct check_authority<N(updategroup)> {
    template <typename Type>
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
struct check_authority<N(updatedomain)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_domain_permission<kManage>(act);
    }
};

template<>
struct check_authority<N(newfungible)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& nf    = act.data_as<add_clr_t<Type>>();
            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(nf.creator, 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transation data is not valid, data cannot cast to `newfungible` type");
        return false;
    }
};

symbol_id_type
get_symbol_id(const name128& key) {
    auto str = (std::string)key;
    return (symbol_id_type)std::stoul(str);
}

template<>
struct check_authority<N(issuefungible)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_fungible_permission<kIssue>(get_symbol_id(act.key), act);
    }
};

template<>
struct check_authority<N(updfungible)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_fungible_permission<kManage>(get_symbol_id(act.key), act);
    }
};

template<>
struct check_authority<N(transferft)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_fungible_permission<kTransfer>(get_symbol_id(act.key), act);
    }
};

template<>
struct check_authority<N(recycleft)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_fungible_permission<kTransfer>(get_symbol_id(act.key), act);
    }
};

template<>
struct check_authority<N(destroyft)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_fungible_permission<kTransfer>(get_symbol_id(act.key), act);
    }
};


template<>
struct check_authority<N(jmzk2pjmzk)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_fungible_permission<kTransfer>(get_symbol_id(act.key), act);
    }
};

template<>
struct check_authority<N(blackaddr)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_group(checker->get_control().get_genesis_state().evt_org.name());
    }
};

template<>
struct check_authority<N(newsuspend)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& ns    = act.data_as<add_clr_t<Type>>();
            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(ns.proposer, 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `newsuspend` type.");
        return false;
    }
};

template<>
struct check_authority<N(aprvsuspend)> {
    template <typename Type>
    static bool
    invoke(const action&, authority_checker*) {
        // will check signatures when applying
        return true;
    }
};

template<>
struct check_authority<N(cancelsuspend)> {
    template <typename Type>
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
struct check_authority<N(execsuspend)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& es    = act.data_as<add_clr_t<Type>>();
            auto vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(es.executor, 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `execsuspend` type.");
        return false;
    }
};

template<>
struct check_authority<N(addmeta)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& am = act.data_as<add_clr_t<Type>>();

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
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `addmeta` type.");
    }
};

template<>
struct check_authority<N(everipass)> {
    template <typename Type>
    static bool
    invoke(const action&, authority_checker*) {
        // check authority when apply
        return true;
    }
};

template<>
struct check_authority<N(everipay)> {
    template <typename Type>
    static bool
    invoke(const action&, authority_checker*) {
        // check authority when apply
        return true;
    }
};

template<>
struct check_authority<N(prodvote)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& pv = act.data_as<add_clr_t<Type>>();

            bool result = false;
            checker->get_producer_key(pv.producer, [&](auto& key) {
                auto vistor = authority_checker::weight_tally_visitor(checker);
                if(vistor(key, 1) == 1) {
                    result = true;
                }
            });
            return result;
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `prodvote` type.");
    }
};

template<>
struct check_authority<N(updsched)> {
    template <typename Type>
    static bool
    invoke(const action&, authority_checker* checker) {
        return checker->satisfied_group(checker->get_control().get_genesis_state().jmzk_org.name());
    }
};

template<>
struct check_authority<N(newlock)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& nl     = act.data_as<add_clr_t<Type>>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(nl.proposer, 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `newlock` type.");
        return false;
    }
};

template<>
struct check_authority<N(aprvlock)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& al     = act.data_as<add_clr_t<Type>>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(al.approver, 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `aprvlock` type.");
        return false;
    }
};

template<>
struct check_authority<N(tryunlock)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& tl     = act.data_as<add_clr_t<Type>>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(tl.executor, 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `tryunlock` type.");
        return false;
    }
};

template<>
struct check_authority<N(paycharge)> {
    template <typename Type>
    static bool
    invoke(const action&, authority_checker*) {
        // not allowed user to use this action
        return false;
    }
};

template<>
struct check_authority<N(paybonus)> {
    template <typename Type>
    static bool
    invoke(const action&, authority_checker*) {
        // not allowed user to use this action
        return false;
    }
};

template<>
struct check_authority<N(setpsvbonus)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_fungible_permission<kManage>(get_symbol_id(act.key), act);
    }
};

template<>
struct check_authority<N(distpsvbonus)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_fungible_permission<kManage>(get_symbol_id(act.key), act);
    }
};

template<>
struct check_authority<N(newstakepool)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_group(checker->get_control().get_genesis_state().jmzk_org.name());
    }
};

template<>
struct check_authority<N(updstakepool)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_group(checker->get_control().get_genesis_state().jmzk_org.name());
    }
};

template<>
struct check_authority<N(newvalidator)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& nv     = act.data_as<add_clr_t<Type>>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(nv.creator, 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `newvalidator` type.");
        return false;
    }
};

template<>
struct check_authority<N(valiwithdraw)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        return checker->satisfied_validator_permission<kWithdraw>(act.key, act);
    }
};

template<>
struct check_authority<N(recvstkbonus)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        bool result = false;
        checker->get_validator(act.key, [&](const auto& validator) {
            auto& key    = validator.signer;
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(validator.signer, 1) == 1) {
                result = true;
            }
        });
        return result;
    }
};

template<>
struct check_authority<N(staketkns)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& st     = act.data_as<add_clr_t<Type>>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(st.staker, 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `staketkns` type.");
        return false;
    }
};

template<>
struct check_authority<N(unstaketkns)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& st     = act.data_as<add_clr_t<Type>>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(st.staker, 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `unstaketkns` type.");
        return false;
    }
};

template<>
struct check_authority<N(toactivetkns)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            auto& st     = act.data_as<add_clr_t<Type>>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(st.staker, 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `toactivetkns` type.");
        return false;
    }
};

template<>
struct check_authority<N(newscript)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        try {
            if(act.key.reserved()) {
                return checker->satisfied_group(checker->get_control().get_genesis_state().jmzk_org.name());
            }

            auto& as     = act.data_as<add_clr_t<Type>>();
            auto  vistor = authority_checker::weight_tally_visitor(checker);

            if(vistor(as.creator, 1) == 1) {
                return true;
            }
        }
        jmzk_RETHROW_EXCEPTIONS(action_type_exception, "transaction data is not valid, data cannot cast to `addscript` type.");
        return false;
    }
};

template<>
struct check_authority<N(updscript)> {
    template <typename Type>
    static bool
    invoke(const action& act, authority_checker* checker) {
        if(act.key.reserved()) {
            return checker->satisfied_group(checker->get_control().get_genesis_state().jmzk_org.name());
        }
        
        bool result = false;
        checker->get_script(act.key, [&](const auto& script) {           
            auto  vistor = authority_checker::weight_tally_visitor(checker);
            if(vistor(script.creator, 1) == 1) {
                result = true;
            }
        });
        return result;
    }
};


}  // namespace internal

}}  // namespace jmzk::chain
