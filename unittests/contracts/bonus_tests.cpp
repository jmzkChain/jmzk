#include "contracts_tests.hpp"

TEST_CASE_METHOD(contracts_test, "passive_bonus_test", "[contracts]") {
    auto spb = setpsvbouns();
    spb.sym = evt_sym();
    spb.rate = 0.15;
    spb.base_charge = asset(0, get_sym());
    spb.dist_threshold = asset(1'00000, get_sym());  // 1.00000

    auto actkey = name128::from_number(get_sym_id());
    auto keyseeds = std::vector<name128>{ N(key2), N(payer) };

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

    spb.rate = 0.15;
    // rules is empty
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_exception);

    spb.base_charge = asset(-1, get_sym());
    // base charge cannot be negative
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_asset_exception);

    spb.base_charge    = asset(0, get_sym());
    spb.dist_threshold = asset(1'00000, get_sym());  // 1.00000

    auto rule1 = dist_fixed_rule();
    rule1.receiver = address();
    rule1.amount   = asset(-10'00000, get_sym());

    spb.rules.emplace_back(rule1);
    // receiver is not valid
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_receiver_exception);

    rule1.receiver = tester::get_public_key("r1");
    spb.rules[0] = rule1;
    // amount cannot be negative
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_asset_exception);

    rule1.amount = asset(10'00000, get_sym());
    spb.rules[0] = rule1;
    // dist threshold not fit the rules (10 > 1)
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_exception);

    spb.dist_threshold = asset(20'00000, get_sym());

    auto rule2 = dist_fixed_rule();
    rule2.receiver = tester::get_public_key("r2");
    rule2.amount   = asset(15'00000, get_sym());

    spb.rules.emplace_back(rule2);
    // dist threshold not fit the rules (10 + 15 > 20)
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_exception);

    spb.dist_threshold = asset(50'00000, get_sym());

    // dist threshold not consumes all by the rules (remains 50 - 10 - 15)
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_rules_not_fullfill);

    auto rule3 = dist_percent_rule();
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
    rule3.percent = percent_type("0.6");
    spb.rules[2] = rule3;
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

    auto rule4 = dist_rpercent_rule();
    rule4.receiver = tester::get_public_key("r4");
    rule4.percent = percent_type("0.99");
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
    rule5.percent = percent_type("0.6");
    spb.rules[4] = rule5;

    // exceed remaining percents
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_percent_value_exception);

    rule5.percent = percent_type("0.0000001");
    spb.rules[4] = rule5;
    // result is less than 1 unit (50-10-15-50*0.3 = 10, 10*0.99 = 9.9, 10 * 0.0000001 = 0.000001)
    CHECK_THROWS_AS(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer), bonus_percent_result_exception);

    rule5.percent = percent_type("0.01");
    spb.rules[4] = rule5;
    CHECK_NOTHROW(my_tester->push_action(action(N128(.bonus), actkey, spb), keyseeds, payer));
}
