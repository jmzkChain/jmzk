/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

namespace jmzk { namespace chain { namespace contracts {

/**
 * Implements setpsvbonus and distpsvbonus actions
 */

namespace internal {

enum class bonus_check_type {
    natural = 0,
    positive
};

auto check_n_rtn = [](auto& asset, auto sym, auto ctype) -> decltype(asset) {
    jmzk_ASSERT2(asset.sym() == sym, bonus_asset_exception, "Invalid symbol of assets, expected: {}, provided:  {}", sym, asset.sym());
    switch(ctype) {
    case bonus_check_type::natural: {
        jmzk_ASSERT2(asset.amount() >= 0, bonus_asset_exception, "Invalid amount of assets, must be natural number. Provided: {}", asset);
        break;
    }
    case bonus_check_type::positive: {
        jmzk_ASSERT2(asset.amount() > 0, bonus_asset_exception, "Invalid amount of assets, must be positive. Provided: {}", asset);
        break;
    }
    }  // switch
    
    return asset;
};

void
check_bonus_receiver(const token_database& tokendb, const dist_receiver& receiver) {
    switch(receiver.type()) {
    case dist_receiver_type::address: {
        auto& addr = receiver.get<address>();
        jmzk_ASSERT2(addr.is_public_key(), bonus_receiver_exception, "Only public key address can be used for receiving bonus now.");
        break;
    }
    case dist_receiver_type::ftholders: {
        auto& sr     = receiver.get<dist_stack_receiver>();
        auto  sym_id = sr.threshold.symbol_id();

        check_n_rtn(sr.threshold, sr.threshold.sym(), bonus_check_type::natural);
        jmzk_ASSERT2(tokendb.exists_token(token_type::fungible, std::nullopt, sym_id),
            bonus_receiver_exception, "Provided bonus tokens, which has sym id: {}, used for receiving is not existed", sym_id);
        break;
    }
    } // switch
}

auto get_percent_string = [](auto& per) {
    percent_type p = per * 100;
    return fmt::format("{} %", p.str(5));
};

template<typename T>
void
check_bonus_rules(const token_database& tokendb, const T& rules, asset amount) {
    auto sym            = amount.sym();
    auto remain         = amount.amount();
    auto remain_percent = percent_type(0);
    auto index          = 0;

    for(auto& rule : rules) {
        switch(rule.type()) {
        case dist_rule_type::fixed: {
            jmzk_ASSERT2(remain_percent == 0, bonus_rules_order_exception,
                "Rule #{} is not valid, fix rule should be defined in front of remain-percent rules", index);
            auto& fr  = rule.template get<dist_rule_type::fixed>();
            // check receiver
            check_bonus_receiver(tokendb, fr.receiver);
            // check sym and > 0
            auto& frv = check_n_rtn(fr.amount, sym, bonus_check_type::positive);
            // check large than remain
            jmzk_ASSERT2(frv.amount() <= remain, bonus_rules_exception,
                "Rule #{} is not valid, its required amount: {} is large than remainning: {}", index, frv, asset(remain, sym));
            remain -= frv.amount();
            break;
        }
        case dist_rule_type::percent: {
            jmzk_ASSERT2(remain_percent == 0, bonus_rules_order_exception,
                "Rule #{} is not valid, percent rule should be defined in front of remain-percent rules", index);
            auto& pr = rule.template get<dist_rule_type::percent>();
            // check receiver
            check_bonus_receiver(tokendb, pr.receiver);

            auto p = (percent_type)pr.percent;
            // check valid precent
            jmzk_ASSERT2(p > 0 && p <= 1, bonus_percent_value_exception,
                "Rule #{} is not valid, precent value should be in range (0,1]", index);
            auto prv = (int64_t)boost::multiprecision::floor(p * real_type(amount.amount()));
            // check large than remain
            jmzk_ASSERT2(prv <= remain, bonus_rules_exception,
                "Rule #{} is not valid, its required amount: {} is large than remainning: {}", index, asset(prv, sym), asset(remain, sym));
            // check percent result is large than minial unit of asset
            jmzk_ASSERT2(prv >= 1, bonus_percent_result_exception,
                "Rule #{} is not valid, the amount for this rule shoule be as least large than one unit of asset, but it's zero now.", index);
            remain -= prv;
            break;
        }
        case dist_rule_type::remaining_percent: {
            jmzk_ASSERT2(remain > 0, bonus_rules_exception, "There's no bonus left for reamining-percent rule to distribute");
            auto& pr = rule.template get<dist_rule_type::remaining_percent>();
            // check receiver
            check_bonus_receiver(tokendb, pr.receiver);

            auto p = (percent_type)pr.percent;
            // check valid precent
            jmzk_ASSERT2(p > 0 && p <= 1, bonus_percent_value_exception, "Precent value should be in range (0,1]");
            auto prv = (int64_t)boost::multiprecision::floor(p * real_type(remain));
            // check percent result is large than minial unit of asset
            jmzk_ASSERT2(prv >= 1, bonus_percent_result_exception,
                "Rule #{} is not valid, the amount for this rule shoule be as least large than one unit of asset, but it's zero now.", index);
            remain_percent += p;
            jmzk_ASSERT2(remain_percent <= 1, bonus_percent_value_exception, "Sum of remaining percents is large than 100%, current: {}", get_percent_string(remain_percent));
            break;
        }
        }  // switch
        index++;
    }

    if(remain > 0) {
        jmzk_ASSERT2(remain_percent == 1, bonus_rules_not_fullfill,
            "Rules are not fullfill amount, total: {}, remains: {}, remains precent fill: {}", amount, asset(remain, sym), get_percent_string(remain_percent));
    }
}

void
check_passive_methods(const execution_context& exec_ctx, const passive_methods& methods) {
    for(auto& it : methods) {
        // check if it's a valid action
        jmzk_ASSERT2(it.action == N(transferft) || it.action == N(everipay), bonus_method_exception,
            "Only `transferft` and `everipay` are valid for method options");
    }
}

dist_rules_v2
to_rules_v2(const dist_rules& rules_v1) {
    auto rules = dist_rules_v2();
    rules.reserve(rules_v1.size());

    for(auto& rule : rules_v1) {
        switch(rule.type()) {
        case dist_rule_type::fixed: {
            auto& fr  = rule.get<dist_fixed_rule>();
            rules.emplace_back(fr);
            break;
        }
        case dist_rule_type::percent: {
            auto& pr = rule.get<dist_percent_rule>();
            rules.emplace_back(dist_percent_rule_v2 {
                .receiver = pr.receiver,
                .percent  = percent_slim(pr.percent)
            });
            break;
        }
        case dist_rule_type::remaining_percent: {
            auto& pr = rule.get<dist_rpercent_rule>();
            rules.emplace_back(dist_rpercent_rule_v2 {
                .receiver = pr.receiver,
                .percent  = percent_slim(pr.percent)
            });
            break;
        }
        }  // swtich
    }

    return rules;
}

} // namespace internal

jmzk_ACTION_IMPL_BEGIN(setpsvbonus) {
    using namespace internal;

    auto spbact = context.act.data_as<ACT>();
    try {
        auto sym_id = 0u;
        if constexpr (jmzk_ACTION_VER() == 1) {
            sym_id = spbact.sym.id();
            jmzk_ASSERT(context.has_authorized(N128(.bonus), name128::from_number(sym_id)), action_authorize_exception,
                "Invalid authorization fields in action(domain and key).");
        }
        else {
            sym_id = spbact.sym_id;
            jmzk_ASSERT(context.has_authorized(N128(.psvbonus), name128::from_number(spbact.sym_id)), action_authorize_exception,
                "Invalid authorization fields in action(domain and key).");
        }

        DECLARE_TOKEN_DB()

        auto fungible = make_empty_cache_ptr<fungible_def>();
        READ_DB_TOKEN(token_type::fungible, std::nullopt, sym_id, fungible, unknown_fungible_exception,
            "Cannot find FT with sym id: {}", sym_id);

        if constexpr (jmzk_ACTION_VER() == 1) {
            jmzk_ASSERT2(fungible->sym == spbact.sym, bonus_symbol_exception, "Symbol provided is not the same as FT");
        }
        auto sym = fungible->sym;

        jmzk_ASSERT(sym != jmzk_sym(), bonus_symbol_exception, "Passive bonus cannot be registered in jmzk");
        jmzk_ASSERT(sym != pjmzk_sym(), bonus_symbol_exception, "Passive bonus cannot be registered in Pinned jmzk");

        jmzk_ASSERT2(!tokendb.exists_token(token_type::psvbonus, std::nullopt, get_psvbonus_db_key(sym.id(), kPsvBonus)),
            bonus_dupe_exception, "It's now allowd to update passive bonus currently.");

        auto rate = (percent_type)spbact.rate;
        jmzk_ASSERT2(rate > 0 && rate <= 1, bonus_percent_value_exception,
            "Rate of passive bonus should be in range (0,1]");

        auto pb   = passive_bonus();
        pb.sym_id = sym.id();
        pb.rate   = percent_slim(spbact.rate);
        
        pb.base_charge = check_n_rtn(spbact.base_charge, sym, bonus_check_type::natural);
        if(spbact.charge_threshold.has_value()) {
            pb.charge_threshold = check_n_rtn(*spbact.charge_threshold, sym, bonus_check_type::positive);
        }
        if(spbact.minimum_charge.has_value()) {
            pb.minimum_charge = check_n_rtn(*spbact.minimum_charge, sym, bonus_check_type::natural);
            if(spbact.charge_threshold.has_value()) {
                jmzk_ASSERT2(*spbact.minimum_charge < *spbact.charge_threshold, bonus_rules_exception,
                    "Minimum charge should be less than charge threshold");
            }
        }
        pb.dist_threshold = check_n_rtn(spbact.dist_threshold, sym, bonus_check_type::positive);

        jmzk_ASSERT2(spbact.rules.size() > 0, bonus_rules_exception, "Rules for passive bonus cannot be empty");
        check_bonus_rules(tokendb, spbact.rules, spbact.dist_threshold);

        if constexpr (jmzk_ACTION_VER() == 1) {
            pb.rules = to_rules_v2(spbact.rules);
        }
        else {
            pb.rules = std::move(spbact.rules);
        }

        check_passive_methods(context.control.get_execution_context(), spbact.methods);
        pb.methods = std::move(spbact.methods);
        
        pb.round = 0;
        ADD_DB_TOKEN(token_type::psvbonus, pb);

        // add passive bonus slim for quick read
        auto pbs        = passive_bonus_slim();
        pbs.sym_id      = sym.id();
        pbs.rate        = percent_slim(pb.rate);
        pbs.base_charge = pb.base_charge.amount();
        pbs.methods     = pb.methods;

        if(pb.charge_threshold.has_value()) {
            pbs.charge_threshold = pb.charge_threshold->amount();
        }
        if(pb.minimum_charge.has_value()) {
            pbs.minimum_charge = pb.minimum_charge->amount();
        }
        ADD_DB_TOKEN(token_type::psvbonus, pbs);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

namespace internal {

struct pubkey_hasher {
    size_t
    operator()(const std::string& key) const {
        return fc::city_hash_size_t(key.data(), key.size());
    }
};

template<typename T>
struct no_hasher {
    size_t
    operator()(const T v) const {
        static_assert(sizeof(v) <= sizeof(size_t));
        return (size_t)v;
    }
};

// it's a very special map that holds the hash value as key
// so the hasher method simplely return the key directly
// key is the hash(pubkey) and value is the amount of asset
using holder_slim_map = google::dense_hash_map<uint32_t, int64_t, no_hasher<uint32_t>>;
// map for storing the pubkeys of collision
using holder_coll_map = std::unordered_map<std::string, int64_t, pubkey_hasher>;

struct holder_dist {
public:
    holder_dist() {
        slim.set_empty_key(0);
    }

public:
    symbol_id_type  sym_id;
    holder_slim_map slim;
    holder_coll_map coll;
    int64_t         total;
};

void
build_holder_dist(const token_database& tokendb, symbol sym, holder_dist& dist) {
    dist.sym_id = sym.id();
    tokendb.read_assets_range(sym.id(), 0, [&dist](auto& k, auto&& v) {
        property prop;
        extract_db_value(v, prop);

        auto h  = fc::city_hash32(k.data(), k.size());
        auto it = dist.slim.emplace(h, prop.amount);
        if(it.second == false) {
            // meet collision
            dist.coll.emplace(std::string(k.data(), k.size()), prop.amount);
        }
        dist.total += prop.amount;

        return true;
    });
};

using holder_dists = small_vector<holder_dist, 4>;

struct bonusdist {
    uint32_t          created_at;    // utc seconds
    uint32_t          created_index; // action index at that time
    int64_t           total;         // total amount for bonus
    holder_dists      holders;
    time_point_sec    deadline;
    optional<address> final_receiver;
};

name128
get_psvbonus_dist_db_key(uint64_t sym_id, uint64_t round) {
    uint128_t v = round;
    v |= ((uint128_t)sym_id << 64);
    return v;
}

}  // namespace internal

jmzk_ACTION_IMPL_BEGIN(distpsvbonus) {
    using namespace internal;

    auto spbact = context.act.data_as<ACT>();
    try {
        jmzk_ASSERT(context.has_authorized(N128(.psvbonus), name128::from_number(spbact.sym_id)), action_authorize_exception,
            "Invalid authorization fields in action(domain and key).");

        DECLARE_TOKEN_DB()

        auto pb = make_empty_cache_ptr<passive_bonus>();
        READ_DB_TOKEN(token_type::psvbonus, std::nullopt, get_psvbonus_db_key(spbact.sym_id, kPsvBonus), pb, unknown_bonus_exception,
            "Cannot find passive bonus registered for fungible token with sym id: {}.", spbact.sym_id);

        auto sym = pb->dist_threshold.sym();

        property pbonus;
        READ_DB_ASSET_NO_THROW(get_psvbonus_address(spbact.sym_id, 0), sym, pbonus);
        jmzk_ASSERT2(pbonus.amount >= pb->dist_threshold.amount(), bonus_unreached_dist_threshold,
            "Distribution threshold: {} is unreached, current: {}", pb->dist_threshold, asset(pbonus.amount, sym));

        auto bd = bonusdist();
        for(auto& rule : pb->rules) {
            auto ftrev = std::optional<dist_stack_receiver>();

            switch(rule.type()) {
            case dist_rule_type::fixed: {
                auto& fr = rule.template get<dist_fixed_rule>();
                if(fr.receiver.type() == dist_receiver_type::ftholders) {
                    ftrev = fr.receiver.template get<dist_stack_receiver>();
                }
                break;
            }
            case dist_rule_type::percent:
            case dist_rule_type::remaining_percent: {
                rule.visit([&ftrev](auto& pr) {
                    if(pr.receiver.type() == dist_receiver_type::ftholders) {
                        ftrev = pr.receiver.template get<dist_stack_receiver>();
                    }
                });
                break;
            }
            }  // switch

            if(ftrev.has_value()) {
                auto dist = holder_dist();
                build_holder_dist(tokendb, ftrev->threshold.sym(), dist);
                bd.holders.emplace_back(std::move(dist));
            }
        }

        bd.created_at     = context.control.pending_block_time().sec_since_epoch();
        bd.created_index  = context.get_index_of_trx();
        bd.deadline       = spbact.deadline;
        bd.final_receiver = spbact.final_receiver;

        pb->round++;
        pb->deadline = spbact.deadline;
        UPD_DB_TOKEN(token_type::psvbonus, *pb);

        auto dbv = make_db_value(bd);
        tokendb_cache.put_token(token_type::psvbonus_dist, action_op::add, std::nullopt, get_psvbonus_db_key(spbact.sym_id, pb->round), dbv);

        // transfer all the FTs from cllected address to distribute address of current round
        transfer_fungible(context, get_psvbonus_address(spbact.sym_id, 0), get_psvbonus_address(spbact.sym_id, pb->round), asset(pbonus.amount, pbonus.sym), N(distpsvbonus), false /* pay bonus */);
    }
    jmzk_CAPTURE_AND_RETHROW(tx_apply_exception);
}
jmzk_ACTION_IMPL_END()

}}} // namespace jmzk::chain::contracts

FC_REFLECT(jmzk::chain::contracts::internal::holder_dist, (sym_id)(slim)(coll)(total));
FC_REFLECT(jmzk::chain::contracts::internal::bonusdist, (created_at)(created_index)(holders)(deadline)(final_receiver));
