/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

namespace evt { namespace chain { namespace contracts {

/**
 * Implements addmeta, paycharge, paybonus, prodvote and updsched actions
 */

namespace internal {

bool
check_involved_node(const group_def& group, const group::node& node, const public_key_type& key) {
    auto result = false;
    group.visit_node(node, [&](const auto& n) {
        if(n.is_leaf()) {
            if(group.get_leaf_key(n) == key) {
                result = true;
                // find one, return false to stop iterate group
                return false;
            }
            return true;
        }
        if(check_involved_node(group, n, key)) {
            result = true;
            // find one, return false to stop iterate group
            return false;
        }
        return true;
    });
    return result;
}

auto check_involved_permission = [](auto& tokendb_cache, const auto& permission, const auto& creator) {
    for(auto& a : permission.authorizers) {
        auto& ref = a.ref;
        switch(ref.type()) {
        case authorizer_ref::account_t: {
            if(creator.is_account_ref() && ref.get_account() == creator.get_account()) {
                return true;
            }
            break;
        }
        case authorizer_ref::group_t: {
            const auto& name = ref.get_group();
            if(creator.is_account_ref()) {
                auto gp = make_empty_cache_ptr<group_def>();
                READ_DB_TOKEN(token_type::group, std::nullopt, name, gp, unknown_group_exception, "Cannot find group: {}", name);
                if(check_involved_node(*gp, gp->root(), creator.get_account())) {
                    return true;
                }
            }
            else {
                if(name == creator.get_group()) {
                    return true;
                }
            }
        }
        }  // switch
    }
    return false;
};

auto check_involved_domain = [](auto& tokendb_cache, const auto& domain, auto pname, const auto& creator) {
    switch(pname) {
    case N(issue): {
        return check_involved_permission(tokendb_cache, domain.issue, creator);
    }
    case N(transfer): {
        return check_involved_permission(tokendb_cache, domain.transfer, creator);
    }
    case N(manage): {
        return check_involved_permission(tokendb_cache, domain.manage, creator);
    }
    }  // switch
    return false;
};

auto check_involved_fungible = [](auto& tokendb_cache, const auto& fungible, auto pname, const auto& creator) {
    switch(pname) {
    case N(manage): {
        return check_involved_permission(tokendb_cache, fungible.manage, creator);
    }
    }  // switch
    return false;
};

auto check_involved_group = [](const auto& group, const auto& key) {
    if(group.key().is_public_key() && group.key().get_public_key() == key) {
        return true;
    }
    return false;
};

auto check_involved_owner = [](const auto& token, const auto& key) {
    for(auto& addr : token.owner) {
        if(addr.is_public_key() && addr.get_public_key() == key) {
            return true;
        }
    }
    return false;
};

auto check_involved_creator = [](const auto& target, const auto& key) {
    return target.creator == key;
};

template<typename T>
bool
check_duplicate_meta(const T& v, const meta_key& key) {
    if(std::find_if(v.metas.cbegin(), v.metas.cend(), [&](const auto& meta) { return meta.key == key; }) != v.metas.cend()) {
        return true;
    }
    return false;
}

template<>
bool
check_duplicate_meta<group_def>(const group_def& v, const meta_key& key) {
    if(std::find_if(v.metas_.cbegin(), v.metas_.cend(), [&](const auto& meta) { return meta.key == key; }) != v.metas_.cend()) {
        return true;
    }
    return false;  
}

auto check_meta_key_reserved = [](const auto& key) {
    EVT_ASSERT(!key.reserved(), meta_key_exception, "Meta-key is reserved and cannot be used");
};

}  // namespace internal

EVT_ACTION_IMPL_BEGIN(addmeta) {
    using namespace internal;

    const auto& act   = context.act;
    auto&       amact = context.act.data_as<add_clr_t<ACT>>();
    try {
        DECLARE_TOKEN_DB()

        if(act.domain == N128(.group)) {  // group
            check_meta_key_reserved(amact.key);

            auto gp = make_empty_cache_ptr<group_def>();
            READ_DB_TOKEN(token_type::group, std::nullopt, act.key, gp, unknown_group_exception, "Cannot find group: {}", act.key);

            EVT_ASSERT2(!check_duplicate_meta(*gp, amact.key), meta_key_exception,"Metadata with key: {} already exists.", amact.key);
            if(amact.creator.is_group_ref()) {
                EVT_ASSERT(amact.creator.get_group() == gp->name_, meta_involve_exception, "Only group itself can add its own metadata");
            }
            else {
                // check involved, only group manager(aka. group key) can add meta
                EVT_ASSERT(check_involved_group(*gp, amact.creator.get_account()), meta_involve_exception,
                    "Creator is not involved in group: ${name}.", ("name",act.key));
            }
            gp->metas_.emplace_back(meta(amact.key, amact.value, amact.creator));
            UPD_DB_TOKEN(token_type::group, *gp);
        }
        else if(act.domain == N128(.fungible)) {  // fungible
            if(amact.key.reserved()) {
                EVT_ASSERT(check_reserved_meta(amact, fungible_metas), meta_key_exception, "Meta-key is reserved and cannot be used");
            }

            auto fungible = make_empty_cache_ptr<fungible_def>();
            READ_DB_TOKEN(token_type::fungible, std::nullopt, (symbol_id_type)std::stoul((std::string)act.key), fungible,
                unknown_fungible_exception, "Cannot find fungible with symbol id: {}", act.key);

            EVT_ASSERT(!check_duplicate_meta(*fungible, amact.key), meta_key_exception,
                "Metadata with key ${key} already exists.", ("key",amact.key));
            
            if(amact.creator.is_account_ref()) {
                // check involved, only creator or person in `manage` permission can add meta
                auto involved = check_involved_creator(*fungible, amact.creator.get_account())
                    || check_involved_fungible(tokendb_cache, *fungible, N(manage), amact.creator);
                EVT_ASSERT(involved, meta_involve_exception, "Creator is not involved in fungible: ${name}.", ("name",act.key));
            }
            else {
                // check involved, only group in `manage` permission can add meta
                EVT_ASSERT(check_involved_fungible(tokendb_cache, *fungible, N(manage), amact.creator), meta_involve_exception,
                    "Creator is not involved in fungible: ${name}.", ("name",act.key));
            }
            fungible->metas.emplace_back(meta(amact.key, amact.value, amact.creator));
            UPD_DB_TOKEN(token_type::fungible, *fungible);
        }
        else if(act.key == N128(.meta)) {  // domain
            if(amact.key.reserved()) {
                EVT_ASSERT(check_reserved_meta(amact, domain_metas), meta_key_exception, "Meta-key is reserved and cannot be used");
            }

            auto domain = make_empty_cache_ptr<domain_def>();
            READ_DB_TOKEN(token_type::domain, std::nullopt, act.domain, domain, unknown_domain_exception,
                "Cannot find domain: {}", act.domain);

            EVT_ASSERT(!check_duplicate_meta(*domain, amact.key), meta_key_exception,
                "Metadata with key ${key} already exists.", ("key",amact.key));
            // check involved, only person involved in `manage` permission can add meta
            EVT_ASSERT(check_involved_domain(tokendb_cache, *domain, N(manage), amact.creator), meta_involve_exception,
                "Creator is not involved in domain: ${name}.", ("name",act.key));

            domain->metas.emplace_back(meta(amact.key, amact.value, amact.creator));
            UPD_DB_TOKEN(token_type::domain, *domain);
        }
        else {  // token
            check_meta_key_reserved(amact.key);

            auto token = make_empty_cache_ptr<token_def>();
            READ_DB_TOKEN(token_type::token, act.domain, act.key, token, unknown_token_exception, "Cannot find token: {} in {}", act.key, act.domain);

            EVT_ASSERT(!check_token_destroy(*token), token_destroyed_exception, "Metadata cannot be added on destroyed token.");
            EVT_ASSERT(!check_token_locked(*token), token_locked_exception, "Metadata cannot be added on locked token.");
            EVT_ASSERT(!check_duplicate_meta(*token, amact.key), meta_key_exception, "Metadata with key ${key} already exists.", ("key",amact.key));

            auto domain = make_empty_cache_ptr<domain_def>();
            READ_DB_TOKEN(token_type::domain, std::nullopt, act.domain, domain, unknown_domain_exception, "Cannot find domain: {}", amact.key);

            if(amact.creator.is_account_ref()) {
                // check involved, only person involved in `issue` and `transfer` permissions or `owners` can add meta
                auto involved = check_involved_owner(*token, amact.creator.get_account())
                    || check_involved_domain(tokendb_cache, *domain, N(issue), amact.creator)
                    || check_involved_domain(tokendb_cache, *domain, N(transfer), amact.creator);
                EVT_ASSERT(involved, meta_involve_exception, "Creator is not involved in token ${domain}-${name}.", ("domain",act.domain)("name",act.key));
            }
            else {
                // check involved, only group involved in `issue` and `transfer` permissions can add meta
                auto involved = check_involved_domain(tokendb_cache, *domain, N(issue), amact.creator)
                    || check_involved_domain(tokendb_cache, *domain, N(transfer), amact.creator);
                EVT_ASSERT(involved, meta_involve_exception, "Creator is not involved in token ${domain}-${name}.", ("domain",act.domain)("name",act.key));
            }
            token->metas.emplace_back(meta(amact.key, amact.value, amact.creator));
            UPD_DB_TOKEN(token_type::token, *token);
        }
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(paycharge) {
    using namespace internal;
    
    auto& pcact = context.act.data_as<add_clr_t<ACT>>();
    try {
        DECLARE_TOKEN_DB()

        property evt, pevt;
        READ_DB_ASSET_NO_THROW_NO_NEW(pcact.payer, pevt_sym(), pevt);
        auto paid = std::min((int64_t)pcact.charge, pevt.amount);
        if(paid > 0) {
            pevt.amount -= paid;
            PUT_DB_ASSET(pcact.payer, pevt);
        }

        if(paid < pcact.charge) {
            READ_DB_ASSET_NO_THROW_NO_NEW(pcact.payer, evt_sym(), evt);
            auto remain = pcact.charge - paid;
            if(evt.amount < (int64_t)remain) {
                EVT_THROW2(charge_exceeded_exception,"There are only {} and {} left, but charge is {}",
                    asset(evt.amount, evt_sym()), asset(pevt.amount, pevt_sym()), asset(pcact.charge, evt_sym()));
            }
            evt.amount -= remain;
            PUT_DB_ASSET(pcact.payer, evt);
        }

        auto  pbs  = context.control.pending_block_state();
        auto& prod = pbs->get_scheduled_producer(pbs->header.timestamp).block_signing_key;

        property bp;
        READ_DB_ASSET_NO_THROW(address(prod), evt_sym(), bp);
        // give charge to producer
        bp.amount += pcact.charge;
        PUT_DB_ASSET(prod, bp);
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(paybonus) {
    // empty body
    // will not execute here
    assert(false);
    return;
}
EVT_ACTION_IMPL_END()

namespace internal {

auto update_chain_config = [](auto& conf, auto key, auto v) {
    switch(key.value) {
    case N128(network-charge-factor): {
        conf.base_network_charge_factor = v;
        break;
    }
    case N128(storage-charge-factor): {
        conf.base_storage_charge_factor = v;
        break;
    }
    case N128(cpu-charge-factor): {
        conf.base_cpu_charge_factor = v;
        break;
    }
    case N128(global-charge-factor): {
        conf.global_charge_factor = v;
        break;
    }
    default: {
        EVT_THROW2(prodvote_key_exception, "Configuration key: {} is not valid", key);
    }
    } // switch
};

}  // namespace internal

EVT_ACTION_IMPL_BEGIN(prodvote) {
    using namespace internal;

    auto& pvact = context.act.data_as<add_clr_t<ACT>>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.prodvote), pvact.key), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        EVT_ASSERT(pvact.value > 0 && pvact.value < 1'000'000, prodvote_value_exception, "Invalid prodvote value: ${v}", ("v",pvact.value));

        auto  conf     = context.control.get_global_properties().configuration;
        auto& sche     = context.control.active_producers();
        auto& exec_ctx = context.control.get_execution_context();

        DECLARE_TOKEN_DB()

        auto updact = false;
        auto act    = name();

        // test if it's action-upgrade vote and wheather action is valid
        {
            auto key = pvact.key.to_string();
            if(boost::starts_with(key, "action-")) {
                try {
                    act = name(key.substr(7));
                }
                catch(name_type_exception&) {
                    EVT_THROW2(prodvote_key_exception, "Invalid action name provided: {}", key.substr(7));
                }

                auto cver = exec_ctx.get_current_version(act);
                auto mver = exec_ctx.get_max_version(act);
                EVT_ASSERT2(pvact.value >= cver && pvact.value <= exec_ctx.get_max_version(act), prodvote_value_exception,
                    "Provided version: {} for action: {} is not valid, should be in range ({},{}]", pvact.value, act, cver, mver);
                updact = true;
            }
        }

        auto pkey = sche.get_producer_key(pvact.producer);
        EVT_ASSERT(pkey.has_value(), prodvote_producer_exception, "${p} is not a valid producer", ("p",pvact.producer));

        auto map = make_empty_cache_ptr<flat_map<public_key_type, int64_t>>();
        READ_DB_TOKEN_NO_THROW(token_type::prodvote, std::nullopt, pvact.key, map);

        if(map == nullptr) {
            auto newmap = flat_map<public_key_type, int64_t>();
            newmap.emplace(*pkey, pvact.value);

            map = tokendb_cache.put_token<std::add_rvalue_reference_t<decltype(newmap)>, true>(
                token_type::prodvote, action_op::put, std::nullopt, pvact.key, std::move(newmap));
        }
        else {
            auto it = map->emplace(*pkey, pvact.value);
            if(it.second == false) {
                // existed
                EVT_ASSERT2(it.first->second != pvact.value, prodvote_value_exception, "Value voted for {} is the same as previous voted", pvact.key);
                it.first->second = pvact.value;
            }
            tokendb_cache.put_token(token_type::prodvote, action_op::put, std::nullopt, pvact.key, *map);
        }

        auto is_prod = [&](auto& pk) {
            for(auto& p : sche.producers) {
                if(p.block_signing_key == pk) {
                    return true;
                }
            }
            return false;
        };

        auto values = std::vector<int64_t>();
        for(auto& it : *map) {
            if(is_prod(it.first)) {
                values.emplace_back(it.second);
            }
        }

        auto limit = (int64_t)values.size(); 
        if(values.size() != sche.producers.size()) {
            limit = ::ceil(2.0 * sche.producers.size() / 3.0);
            if((int64_t)values.size() <= limit) {
                // if the number of votes is equal or less than 2/3 producers
                // don't update
                return;
            }
        }

        if(!updact) {
            // general global config updates, find the median and update
            int64_t nv = 0;

            // find median
            if(values.size() % 2 == 0) {
                auto it1 = values.begin() + values.size() / 2 - 1;
                auto it2 = values.begin() + values.size() / 2;

                std::nth_element(values.begin(), it1 , values.end());
                std::nth_element(values.begin(), it2 , values.end());

                nv = ::floor((*it1 + *it2) / 2);
            }
            else {
                auto it = values.begin() + values.size() / 2;
                std::nth_element(values.begin(), it , values.end());

                nv = *it;
            }

            update_chain_config(conf, pvact.key, nv);
            context.control.set_chain_config(conf);
        }
        else {
            // update action version
            // find the all the votes which vote-version is large than current version
            // and update version with the version which has more than 2/3 votes of producers
            auto cver = exec_ctx.get_current_version(act);
            auto map  = flat_map<int, int>();  // maps version to votes
            for(auto& v : values) {
                if(v > cver) {
                    map[v] += 1;
                }
            }
            for(auto& it : map) {
                if(it.second >= limit) {
                    exec_ctx.set_version(act, it.first);
                    break;
                }
            }
        }
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

EVT_ACTION_IMPL_BEGIN(updsched) {
    auto usact = context.act.data_as<ACT>();
    try {
        EVT_ASSERT(context.has_authorized(N128(.prodsched), N128(.update)), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");
        context.control.set_proposed_producers(std::move(usact.producers));
    }
    EVT_CAPTURE_AND_RETHROW(tx_apply_exception);
}
EVT_ACTION_IMPL_END()

}}} // namespace evt::chain::contracts
