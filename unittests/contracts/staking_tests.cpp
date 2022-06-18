#include "contracts_tests.hpp"

namespace mp = boost::multiprecision;

TEST_CASE_METHOD(contracts_test, "newstakepool_test", "[contracts][staking]") {
    auto test_data = R"=====(
    {
      "sym_id": 1,
      "purchase_threshold": "5.00000 S#1",
      "demand_r": 5,
      "demand_t": 5,
      "demand_q": 5,
      "demand_w": 5,
      "fixed_r": 5,
      "fixed_t": 5
    }
    )=====";

    auto var = fc::json::from_string(test_data);

    signed_transaction trx;
    trx.actions.emplace_back(my_tester->get_action(N(newstakepool), N128(.fungible), N128(1), var.get_object()));

    my_tester->set_transaction_headers(trx, payer, 1'000'000, base_tester::DEFAULT_EXPIRATION_DELTA);
    for(auto& au : key_seeds) {
        trx.sign(tester::get_private_key(au), my_tester->control->get_chain_id());
    }
    trx.sign(fc::crypto::private_key(std::string("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3")), my_tester->control->get_chain_id());

    CHECK_NOTHROW(my_tester->push_transaction(trx));

    auto& tokendb = my_tester->control->token_db();
    CHECK(EXISTS_TOKEN(stakepool, 1));
    stakepool_def stakepool_;
    READ_TOKEN(stakepool, 1, stakepool_);

    // check data
    CHECK(stakepool_.sym_id == 1);
    CHECK(stakepool_.purchase_threshold == asset(500'000, symbol(5, 1)));
    CHECK(stakepool_.demand_r == 5);
    CHECK(stakepool_.demand_t == 5);
    CHECK(stakepool_.demand_q == 5);
    CHECK(stakepool_.demand_w == 5);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "updstakepool_test", "[contracts][staking]") {
    auto test_data = R"=====(
    {
      "sym_id": 1,
      "purchase_threshold": "5.00000 S#1",
      "demand_r": 50000000,
      "demand_t": -670,
      "demand_q": 10000,
      "demand_w": -1,
      "fixed_r": 150000,
      "fixed_t": 5000
    }
    )=====";

    auto var = fc::json::from_string(test_data);

    signed_transaction trx;
    trx.actions.emplace_back(my_tester->get_action(N(updstakepool), N128(.fungible), N128(1), var.get_object()));

    my_tester->set_transaction_headers(trx, payer, 1'000'000, base_tester::DEFAULT_EXPIRATION_DELTA);
    for(auto& au : key_seeds) {
        trx.sign(tester::get_private_key(au), my_tester->control->get_chain_id());
    }
    trx.sign(fc::crypto::private_key(std::string("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3")), my_tester->control->get_chain_id());

    CHECK_NOTHROW(my_tester->push_transaction(trx));

    auto& tokendb = my_tester->control->token_db();

    stakepool_def stakepool_;
    READ_TOKEN(stakepool, 1, stakepool_);

    // check data
    CHECK(stakepool_.sym_id == 1);
    CHECK(stakepool_.purchase_threshold == asset(500'000, symbol(5, 1)));
    CHECK(stakepool_.demand_r == 50000'000);
    CHECK(stakepool_.demand_t == -670);
    CHECK(stakepool_.demand_q == 10'000);
    CHECK(stakepool_.demand_w == -1);
    CHECK(stakepool_.fixed_r == 150'000);
    CHECK(stakepool_.fixed_t == 5'000);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "newvalidator_test", "[contracts][staking]") {
    auto test_data = R"=====(
    {
      "name": "validator",
      "creator": "jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "signer": "jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "withdraw" : {
        "name" : "withdraw",
        "threshold" : 1,
        "authorizers": [{
            "ref": "[A] jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
            "weight": 1
          }
        ]
      },
      "manage" : {
        "name" : "manage",
        "threshold" : 1,
        "authorizers": [{
            "ref": "[A] jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
            "weight": 1
          }
        ]
      },
      "commission": "0.5"
    }
    )=====";

    auto var    = fc::json::from_string(test_data);
    auto nvd    = var.as<newvalidator>();
    nvd.creator = key;
    nvd.signer  = key;
    nvd.withdraw.authorizers[0].ref.set_account(key);
    nvd.manage.authorizers[0].ref.set_account(key);
    to_variant(nvd, var);

    CHECK_NOTHROW(my_tester->push_action(N(newvalidator), N128(.staking), N128(validator), var.get_object(), key_seeds, payer));

    auto& tokendb = my_tester->control->token_db();
    CHECK(EXISTS_TOKEN(validator, "validator"));
    validator_def validator_;
    READ_TOKEN(validator, nvd.name, validator_);

    // check data
    CHECK((std::string)validator_.commission == "0.5");
    CHECK(validator_.signer == key);
    CHECK(validator_.creator == key);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "staketkns_test", "[contracts][staking]") {
    auto test_data = R"=====(
    {
      "staker": "jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "validator": "validator",
      "amount" : "500000.00000 S#1",
      "type": "active",
      "fixed_days": 5
    }
    )=====";

    auto var   = fc::json::from_string(test_data);
    auto stk   = var.as<staketkns>();
    stk.staker = key;
    to_variant(stk, var);

    auto& tokendb = my_tester->control->token_db();
    CHECK(EXISTS_TOKEN(validator, "validator"));

    // active type with fixed_days
    CHECK_THROWS_AS(my_tester->push_action(N(staketkns), N128(.staking), N128(validator), var.get_object(), key_seeds, payer), balance_exception);

    my_tester->add_money(key, asset(10'000'000'00000, jmzk_sym()));
    CHECK_THROWS_AS(my_tester->push_action(N(staketkns), N128(.staking), N128(validator), var.get_object(), key_seeds, payer), staking_days_exception);

    stk.fixed_days = 0;
    to_variant(stk, var);

    // correct: active type
    CHECK_NOTHROW(my_tester->push_action(N(staketkns), N128(.staking), N128(validator), var.get_object(), key_seeds, payer));

    validator_def validator_;
    READ_TOKEN(validator, "validator", validator_);
    CHECK(validator_.total_units == 500000);

    stakepool_def stakepool_;
    READ_TOKEN(stakepool, 1, stakepool_);
    CHECK(stakepool_.total == asset(500000'00000, jmzk_sym()));

    stk.type = stake_type::fixed;
    stk.fixed_days = 30;
    to_variant(stk, var);
    // correct: fiexd type with 9000 days
    CHECK_NOTHROW(my_tester->push_action(N(staketkns), N128(.staking), N128(validator), var.get_object(), key_seeds, payer));

    READ_TOKEN(validator, "validator", validator_);
    CHECK(validator_.total_units == 1'000'000);

    READ_TOKEN(stakepool, 1, stakepool_);
    CHECK(stakepool_.total == asset(1'000'000'00000, jmzk_sym()));

    my_tester->produce_blocks();
    my_tester->produce_block(fc::days(stk.fixed_days + 1));
}

TEST_CASE_METHOD(contracts_test, "toactivetkns_test", "[contracts][staking]") {
    auto test_data = R"=====(
    {
      "staker": "jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "validator": "validator",
      "sym_id": 1
    }
    )=====";

    auto var    = fc::json::from_string(test_data);
    auto tatk   = var.as<toactivetkns>();
    tatk.staker = key;
    to_variant(tatk, var);

    auto& tokendb = my_tester->control->token_db();
    stakepool_def stakepool_;
    READ_TOKEN(stakepool, 1, stakepool_);

    int64_t pre_stakepool_amout = stakepool_.total.amount();

    validator_def validator_;
    READ_TOKEN(validator, "validator", validator_);
    int64_t pre_validator_units = validator_.total_units;

    CHECK_NOTHROW(my_tester->push_action(N(toactivetkns), N128(.staking), N128(validator), var.get_object(), key_seeds, payer));

    auto appro_roi      = real_type(0.0994194096);
    int64_t diff_units  = (int64_t)mp::floor(real_type(500000) * appro_roi);
    int64_t diff_amount = 1'00000 * diff_units;

    // check diff units
    READ_TOKEN(validator, "validator", validator_);
    CHECK(validator_.total_units - pre_validator_units == diff_units);

    // check diff amounts
    READ_TOKEN(stakepool, 1, stakepool_);
    CHECK(stakepool_.total.amount() - pre_stakepool_amout == diff_amount);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "unstaketkns_test", "[contracts][staking]") {

    auto test_data = R"=====(
    {
      "staker": "jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "validator": "validator",
      "units" : 200000,
      "sym_id": 1,
      "op": "propose"
    }
    )=====";

    auto var     = fc::json::from_string(test_data);
    auto unstk   = var.as<unstaketkns>();
    unstk.staker = key;
    to_variant(unstk, var);

    auto& conf    = my_tester->control->get_global_properties().staking_configuration;
    auto& tokendb = my_tester->control->token_db();
    auto& tokendb_cache = my_tester->control->token_db_cache();
    CHECK(EXISTS_TOKEN(validator, "validator"));

    stakepool_def stakepool_;
    READ_TOKEN(stakepool, 1, stakepool_);
    auto pre_stakepool_amout = stakepool_.total.amount();

    validator_def validator_;
    READ_TOKEN(validator, "validator", validator_);
    auto pre_validator_units = validator_.total_units;

    auto sum_units = [](auto& prop) {
        int64_t total_units = 0;
        for(auto &stake : prop.stake_shares) {
            total_units += stake.units;
        }
        return total_units;
    };

    auto sum_pending_units = [](auto& prop) {
        int64_t total_units = 0;
        for(auto &stake : prop.pending_shares) {
            total_units += stake.units;
        }
        return total_units;
    };

    auto prop = property_stakes();
    READ_DB_ASSET(unstk.staker, jmzk_sym(), prop);
    auto pre_amount = prop.amount;
    auto pre_units  = sum_units(prop);

    // proposed and check pending_shares' total units
    CHECK_NOTHROW(my_tester->push_action(N(unstaketkns), N128(.staking), N128(validator), var.get_object(), key_seeds, payer));
    my_tester->produce_blocks();

    READ_DB_ASSET(unstk.staker, jmzk_sym(), prop);
    CHECK(sum_pending_units(prop) == 200000);
    CHECK(sum_units(prop) == pre_units - 200000);
    CHECK(prop.pending_shares.size() == 1);
    CHECK(prop.stake_shares.size() == 2);

    // canceled and check pending_shares' total units
    unstk.op = unstake_op::cancel;
    to_variant(unstk, var);
    CHECK_NOTHROW(my_tester->push_action(N(unstaketkns), N128(.staking), N128(validator), var.get_object(), key_seeds, payer));

    my_tester->produce_blocks();
    
    READ_DB_ASSET(unstk.staker, jmzk_sym(), prop);
    CHECK(sum_pending_units(prop) == 0);
    CHECK(sum_units(prop) == pre_units);
    CHECK(prop.pending_shares.size() == 0);
    CHECK(prop.stake_shares.size() == 3);

    // propose again
    unstk.op    = unstake_op::propose;
    unstk.units = 300000;
    to_variant(unstk, var);

    CHECK_NOTHROW(my_tester->push_action(N(unstaketkns), N128(.staking), N128(validator), var.get_object(), key_seeds, payer));

    my_tester->produce_blocks();

    READ_DB_ASSET(unstk.staker, jmzk_sym(), prop);
    CHECK(sum_pending_units(prop) == 300000);
    CHECK(sum_units(prop) == pre_units - 300000);
    CHECK(prop.pending_shares.size() == 1);
    CHECK(prop.stake_shares.size() == 2);

    // double validator's net value
    auto validator = make_empty_cache_ptr<validator_def>();
    READ_DB_TOKEN(token_type::validator, std::nullopt, unstk.validator, validator, unknown_validator_exception,
            "Cannot find validator: {}", unstk.validator);
    validator->current_net_value = asset::from_integer(2, nav_sym());
    UPD_DB_TOKEN(validator,  N128(validator), *validator);
    my_tester->produce_blocks();

    // settled again and check pending_shares' total units
    unstk.op = unstake_op::settle;
    to_variant(unstk, var);
    CHECK_THROWS_AS(my_tester->push_action(N(unstaketkns), N128(.staking), N128(validator), var.get_object(), key_seeds, payer), staking_not_enough_exception);

    // exceed pending days: 8
    my_tester->produce_block(fc::days(conf.unstake_pending_days + 1));

    // ok
    CHECK_NOTHROW(my_tester->push_action(N(unstaketkns), N128(.staking), N128(validator), var.get_object(), key_seeds, payer));

    READ_DB_ASSET(unstk.staker, jmzk_sym(), prop);
    CHECK(sum_pending_units(prop) == 0);
    CHECK(sum_units(prop) == pre_units - 300000);
    CHECK(prop.pending_shares.size() == 0);
    CHECK(prop.stake_shares.size() == 2);
    CHECK(prop.amount - pre_amount == 300000'00000);  // net value doubled

    // check stakepool
    READ_TOKEN(stakepool, 1, stakepool_);
    CHECK(pre_stakepool_amout - stakepool_.total.amount() == 450000'00000); // 300'000 + 150'000(another 150'000 is for validator)

    // check validator
    READ_TOKEN(validator, "validator", validator_);
    CHECK(pre_validator_units - validator_.total_units == 300'000);

    auto vaddr = address(N(.validator), validator_.name, jmzk_SYM_ID);
    auto vprop = property();
    READ_DB_ASSET(vaddr, jmzk_sym(), vprop);
    CHECK(vprop.amount == 150'000'00000);  // 50% commission

    my_tester->produce_blocks();

    READ_DB_ASSET(unstk.staker, jmzk_sym(), prop);
    CHECK(prop.stake_shares.size() == 2);
    auto s = prop.stake_shares[0];
    s.units = 5;
    prop.stake_shares =  {s,s,s,s,s,s};
    prop.stake_shares[0].type = stake_type::fixed;
    prop.stake_shares[2].type = stake_type::fixed;
    prop.stake_shares[4].type = stake_type::fixed;
    PUT_DB_ASSET(unstk.staker, prop);

    unstk.op    = unstake_op::propose;
    unstk.units = 12;
    to_variant(unstk, var);

    CHECK_NOTHROW(my_tester->push_action(N(unstaketkns), N128(.staking), N128(validator), var.get_object(), key_seeds, payer));

    READ_DB_ASSET(unstk.staker, jmzk_sym(), prop);
    CHECK(sum_units(prop) == 18);
    CHECK(prop.stake_shares.size() == 4);
}

TEST_CASE_METHOD(contracts_test, "valiwithdraw_test", "[contracts][staking]") {
    auto test_data = R"=====(
    {
      "name": "validator",
      "addr": "jmzk546WaW3zFAxEEEkYKjDiMvg3CHRjmWX2XdNxEhi69RpdKuQRSK",
      "amount": "10.00000 S#1"
    }
    )=====";

    auto var = fc::json::from_string(test_data);
    auto vwd = var.as<valiwithdraw>();
    vwd.addr = key;
    to_variant(vwd, var);

    auto& tokendb = my_tester->control->token_db();

    asset pre_ast;
    READ_DB_ASSET(vwd.addr, jmzk_sym(), pre_ast);

    CHECK_NOTHROW(my_tester->push_action(N(valiwithdraw), N128(.staking), N128(validator), var.get_object(), key_seeds, payer));

    asset ast;
    READ_DB_ASSET(vwd.addr, jmzk_sym(), ast);

    CHECK(ast.amount() - pre_ast.amount() == 10'00000);

    my_tester->produce_blocks();
}

TEST_CASE_METHOD(contracts_test, "recvstkbonus_test", "[contracts][staking]") {
    auto test_data = R"=====(
    {
      "validator": "validator",
      "sym_id": 1
    }
    )=====";

    auto var = fc::json::from_string(test_data);
    auto rsb = var.as<recvstkbonus>();

    auto& tokendb       = my_tester->control->token_db();
    auto& tokendb_cache = my_tester->control->token_db_cache();

    stakepool_def stakepool_;
    READ_TOKEN(stakepool, 1, stakepool_);
    auto pre_amount = stakepool_.total.amount();

    my_tester->produce_blocks();
    auto validator = make_empty_cache_ptr<validator_def>();
    READ_DB_TOKEN(token_type::validator, std::nullopt, rsb.validator, validator, unknown_validator_exception,
                  "Cannot find validator: {}", rsb.validator);

    CHECK(validator->signer == key);
    
    // reset validator's net value
    validator->current_net_value = asset::from_integer(1, nav_sym());
    UPD_DB_TOKEN(validator,  N128(validator), *validator);
    my_tester->produce_blocks();

    // pump days
    auto& conf = my_tester->control->get_global_properties().staking_configuration;
    my_tester->produce_block(fc::days(51));  // total days: 31 + 8 + 51 = 90 days
    my_tester->produce_blocks((conf.cycles_per_period - 1) * conf.blocks_per_cycle);

    // calculate roi
    auto yroi = 0.175410021;
    auto roi  = std::pow(1 + yroi, (76.8 / (365 * 24 * 60))) - 1;

    INFO(stakepool_.total);
    INFO(yroi);
    INFO(roi);
    INFO((my_tester->control->pending_block_time() - stakepool_.begin_time).to_seconds());

    // update net value
    CHECK_NOTHROW(my_tester->push_action(N(recvstkbonus), N128(.staking), N128(validator), var.get_object(), key_seeds, payer));
    
    READ_DB_TOKEN(token_type::validator, std::nullopt, rsb.validator, validator, unknown_validator_exception,
                  "Cannot find validator: {}", rsb.validator);
    CHECK(validator->current_net_value.to_double() == Approx(1 * (1 + roi)).margin(0.001));

    READ_TOKEN(stakepool, 1, stakepool_);
    CHECK(stakepool_.total.amount() / 1'00000.0 - pre_amount / 1'00000.0 == Approx(validator->total_units * roi).margin(1));

    my_tester->produce_blocks();
}