#include "contracts_tests.hpp"
#include <evt/chain/address.hpp>

name128
get_bonus_db_key(uint64_t sym_id, uint64_t bonus_id) {
    uint128_t v = bonus_id;
    v |= ((uint128_t)sym_id << 64);
    return v;
}

TEST_CASE_METHOD(contracts_test, "passive_bonus_test", "[contracts]") {
    auto spb = setpsvbonus();
    spb.sym = evt_sym();
    spb.rate = 0.15;
    spb.base_charge = asset(0, get_sym());
    spb.dist_threshold = asset(1'00000, get_sym());  // 1.00000

    auto actkey = name128::from_number(get_sym_id());
    auto keyseeds = std::vector<name>{ N(key2), N(payer) };

    // key of action is invalid
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), action_authorize_exception);

    spb.sym = get_sym();
    // evt cannot be used to set passive bonus
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_exception);

    // rate is not valid
    spb.rate = 0;
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_percent_value_exception);

    spb.rate = 1.1;
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_percent_value_exception);

    spb.rate = percent_type("0.15");
    // rules is empty
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_exception);

    spb.base_charge = asset(-1, get_sym());
    // base charge cannot be negative
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_asset_exception);

    spb.base_charge    = asset(10, get_sym());
    spb.dist_threshold = asset(1'00000, get_sym());  // 1.00000

    auto rule1     = dist_fixed_rule();
    rule1.receiver = address();
    rule1.amount   = asset(-10'00000, get_sym());

    spb.rules.emplace_back(rule1);
    // receiver is not valid
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_receiver_exception);

    rule1.receiver = tester::get_public_key("r1");
    spb.rules[0]   = rule1;
    // amount cannot be negative
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_asset_exception);

    rule1.amount = asset(10'00000, get_sym());
    spb.rules[0] = rule1;
    // dist threshold not fit the rules (10 > 1)
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_exception);

    spb.dist_threshold = asset(20'00000, get_sym());

    auto rule2     = dist_fixed_rule();
    rule2.receiver = tester::get_public_key("r2");
    rule2.amount   = asset(15'00000, get_sym());

    spb.rules.emplace_back(rule2);
    // dist threshold not fit the rules (10 + 15 > 20)
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_exception);

    spb.dist_threshold = asset(50'00000, get_sym());

    // dist threshold not consumes all by the rules (remains 50 - 10 - 15)
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_not_fullfill);

    auto rule3     = dist_percent_rule();
    rule3.receiver = dist_stack_receiver(asset(-1'00000, evt_sym()));
    rule3.percent  = percent_type("0.15");
    spb.rules.emplace_back(rule3);

    // threshold cannot be negative
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_asset_exception);

    rule3.receiver = dist_stack_receiver(asset(1'00000, symbol(5, 100)));
    spb.rules[2] = rule3;
    // bonus tokens cannot be found
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_receiver_exception);

    rule3.receiver = dist_stack_receiver(asset(1'00000, evt_sym()));
    rule3.percent  = percent_type("0.6");
    spb.rules[2]   = rule3;
    // exceed all the dist threshold 50 < (10 + 15 + 50*0.6)
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_exception);

    rule3.percent = percent_type("1.2");
    spb.rules[2] = rule3;
    // percent is not valid
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_percent_value_exception);

    rule3.percent = percent_type("0.3");
    spb.rules[2] = rule3;
    // not fullfill all the dist threshold
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_not_fullfill);

    auto rule4     = dist_rpercent_rule();
    rule4.receiver = tester::get_public_key("r4");
    rule4.percent  = percent_type("0.99");
    spb.rules.emplace_back(rule4);
    // not fullfill all the remainning percents
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_not_fullfill);
    
    auto rule5_fake     = dist_fixed_rule();
    rule5_fake.receiver = tester::get_public_key("r4");
    rule5_fake.amount   = asset(15'00000, get_sym());
    spb.rules.emplace_back(rule5_fake);

    // cannot declare fixed rule after percent rule
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_order_exception);

    auto rule5 = dist_rpercent_rule();
    rule5.receiver = dist_stack_receiver(asset(0, get_sym()));
    rule5.percent  = percent_type("0.6");
    spb.rules[4]   = rule5;

    // exceed remaining percents
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_percent_value_exception);

    rule5.percent = percent_type("0.0000001");
    spb.rules[4]  = rule5;
    // result is less than 1 unit (50-10-15-50*0.3 = 10, 10*0.99 = 9.9, 10 * 0.0000001 = 0.000001)
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_percent_result_exception);

    rule5.percent = percent_type("0.01");
    spb.rules[4]  = rule5;

    spb.charge_threshold = asset(0, get_sym());
    // charge threshold shouln't be zero
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_asset_exception);

    spb.charge_threshold = asset(200, get_sym());
    spb.minimum_charge   = asset(400, get_sym());

    // minimum charge is large than charge threshold
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_exception);

    spb.charge_threshold = asset(20000, get_sym());
    spb.minimum_charge   = asset(1000, get_sym());

    spb.methods.emplace_back(passive_method{name("transferft"), passive_method_type::outside_amount});
    spb.methods.emplace_back(passive_method{name("transfer"), passive_method_type::outside_amount});
    // transfer is not valid action
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_method_exeption);

    spb.methods.erase(spb.methods.cbegin() + 1);  // transfer
    spb.methods.emplace_back(passive_method{name("everipay"), passive_method_type::within_amount});

    // fine
    CHECK_NOTHROW(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer));

    my_tester->produce_block();

    // dupe passive bonus
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_dupe_exception);
}

TEST_CASE_METHOD(contracts_test, "passive_bonus_fees_test", "[contracts]") {
    auto& tokendb = my_tester->control->token_db();
    CHECK(tokendb.exists_token(token_type::bonus, std::nullopt, get_bonus_db_key(get_sym_id(), 0)));

    auto tf   = transferft();
    tf.from   = key;
    tf.to     = tester::get_public_key(N(to1));
    tf.number = asset(1000, get_sym());

    auto actkey = name128::from_number(get_sym_id());
    auto bonus_addr = address(N(.bonus), actkey, 0);

    // fees: 0.15 * 1000 = 15, actual: 1000
    my_tester->push_action(action(N128(.fungible), actkey, tf), key_seeds, payer);

    {
        property bonus, to;
        READ_DB_ASSET(bonus_addr, get_sym(), bonus);
        READ_DB_ASSET(tf.to, get_sym(), to);

        CHECK(bonus.amount == 1000);
        CHECK(to.amount == 1000);
    }

    tf.to = tester::get_public_key(N(to2));
    tf.number = asset(1'00000, get_sym());
    // fees: 0.15 * 1'00000 = '15000, actual: '15010
    my_tester->push_action(action(N128(.fungible), actkey, tf), key_seeds, payer);

    {
        property bonus, to;
        READ_DB_ASSET(bonus_addr, get_sym(), bonus);
        READ_DB_ASSET(tf.to, get_sym(), to);

        CHECK(bonus.amount == 1000 + 15010);
        CHECK(to.amount == 1'00000);
    }

    tf.to = tester::get_public_key(N(to3));
    tf.number = asset(2'00000, get_sym());
    // fees: 0.15 * 2'00000 = '30000, actual: '20000
    my_tester->push_action(action(N128(.fungible), actkey, tf), key_seeds, payer);

    {
        property bonus, to;
        READ_DB_ASSET(bonus_addr, get_sym(), bonus);
        READ_DB_ASSET(tf.to, get_sym(), to);

        CHECK(bonus.amount == 1000 + 15010 + 20000);
        CHECK(to.amount == 2'00000);
    }

    my_tester->produce_block();
}

TEST_CASE_METHOD(contracts_test, "passive_bonus_dist_test", "[contracts]") {
    auto& tokendb = my_tester->control->token_db();
    CHECK(tokendb.exists_token(token_type::bonus, std::nullopt, get_bonus_db_key(get_sym_id(), 0)));

    auto actkey     = name128::from_number(get_sym_id());
    auto bonus_addr = address(N(.bonus), actkey, 0);

    auto dpb     = distpsvbonus();
    dpb.sym      = evt_sym();
    dpb.deadline = my_tester->control->head_block_time();

    auto keyseeds = std::vector<name>{ N(key2), N(payer) };
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, dpb), keyseeds, payer), action_authorize_exception);

    dpb.sym = get_sym();
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, dpb), keyseeds, payer), bonus_unreached_dist_threshold);

    {
        property bonus;
        READ_DB_ASSET(bonus_addr, get_sym(), bonus);
        CHECK(bonus.amount == 1000 + 15010 + 20000);
    }

    // total: 0.2 * 300 = 60
    for(int i = 0; i < 300; i++) {
        auto tf   = transferft();
        tf.from   = key;
        tf.to     = tester::get_public_key(N(to3));
        tf.number = asset(2'00000, get_sym());

        my_tester->push_action(action(N128(.fungible), actkey, tf), key_seeds, payer);
        my_tester->produce_block();
    }

    {
        property bonus;
        READ_DB_ASSET(bonus_addr, get_sym(), bonus);
        CHECK(bonus.amount == 1000 + 15010 + 20000 + 20000 * 300);
    }

    my_tester->push_action(action(N128(.bonus), actkey, dpb), keyseeds, payer);

    my_tester->produce_block();
}