#pragma once
#include <iosfwd>
#include <boost/test/unit_test.hpp>
#include <fc/io/json.hpp>
#include <evt/chain/asset.hpp>
#include <evt/chain/controller.hpp>
#include <evt/chain/snapshot.hpp>
#include <evt/chain/contracts/evt_contract_abi.hpp>
#include <evt/chain/contracts/abi_serializer.hpp>

#define REQUIRE_EQUAL_OBJECTS(left, right)                                                                                           \
    {                                                                                                                                \
        auto a = fc::variant(left);                                                                                                  \
        auto b = fc::variant(right);                                                                                                 \
        BOOST_REQUIRE_EQUAL(true, a.is_object());                                                                                    \
        BOOST_REQUIRE_EQUAL(true, b.is_object());                                                                                    \
        BOOST_REQUIRE_EQUAL_COLLECTIONS(a.get_object().begin(), a.get_object().end(), b.get_object().begin(), b.get_object().end()); \
    }

#define REQUIRE_MATCHING_OBJECT(left, right)                                                                             \
    {                                                                                                                    \
        auto a = fc::variant(left);                                                                                      \
        auto b = fc::variant(right);                                                                                     \
        BOOST_REQUIRE_EQUAL(true, a.is_object());                                                                        \
        BOOST_REQUIRE_EQUAL(true, b.is_object());                                                                        \
        auto filtered = ::evt::testing::filter_fields(a.get_object(), b.get_object());                                 \
        BOOST_REQUIRE_EQUAL_COLLECTIONS(a.get_object().begin(), a.get_object().end(), filtered.begin(), filtered.end()); \
    }

std::ostream& operator<<(std::ostream& osm, const fc::variant& v);

std::ostream& operator<<(std::ostream& osm, const fc::variant_object& v);

std::ostream& operator<<(std::ostream& osm, const fc::variant_object::entry& e);

namespace boost { namespace test_tools { namespace tt_detail {

template <>
struct print_log_value<fc::variant> {
    void
    operator()(std::ostream& osm, const fc::variant& v) {
        ::operator<<(osm, v);
    }
};

template <>
struct print_log_value<fc::variant_object> {
    void
    operator()(std::ostream& osm, const fc::variant_object& v) {
        ::operator<<(osm, v);
    }
};

template <>
struct print_log_value<fc::variant_object::entry> {
    void
    operator()(std::ostream& osm, const fc::variant_object::entry& e) {
        ::operator<<(osm, e);
    }
};

}}}  // namespace boost::test_tools::tt_detail

namespace evt { namespace testing {

using namespace evt::chain;

fc::variant_object filter_fields(const fc::variant_object& filter, const fc::variant_object& value);

bool expect_assert_message(const fc::exception& ex, string expected);

/**
 *  @class tester
 *  @brief provides utility function to simplify the creation of unit tests
 */
class base_tester {
public:
    typedef string action_result;

    static const uint32_t DEFAULT_EXPIRATION_DELTA = 6;

    static const uint32_t DEFAULT_BILLED_CPU_TIME_US = 2000;

public:
    base_tester();
    virtual ~base_tester(){};

    void init(bool push_genesis = true);
    void init(controller::config config, const snapshot_reader_ptr& snapshot = nullptr);

    void close();
    void open(const snapshot_reader_ptr& snapshot);
    bool is_same_chain(base_tester& other);

    virtual signed_block_ptr produce_block(fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms), uint32_t skip_flag = 0 /*skip_missed_block_penalty*/)       = 0;
    virtual signed_block_ptr produce_empty_block(fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms), uint32_t skip_flag = 0 /*skip_missed_block_penalty*/) = 0;
    void                     produce_blocks(uint32_t n = 1, bool empty = false);
    void                     produce_blocks_until_end_of_round();
    void                     produce_blocks_for_n_rounds(const uint32_t num_of_rounds = 1);
    // Produce minimal number of blocks as possible to spend the given time without having any producer become inactive
    void             produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(const fc::microseconds target_elapsed_time = fc::microseconds());
    signed_block_ptr push_block(signed_block_ptr b);

    transaction_trace_ptr push_transaction(packed_transaction& trx, fc::time_point deadline = fc::time_point::maximum());
    transaction_trace_ptr push_transaction(signed_transaction& trx, fc::time_point deadline = fc::time_point::maximum());

    transaction_trace_ptr push_action(action&& act, std::vector<name>& auths, const address& payer, uint32_t max_charge = 1'000'000);

    transaction_trace_ptr push_action(const action_name&       acttype,
                                      const domain_name&       domain,
                                      const domain_key&        key,
                                      const variant_object&    data,
                                      const std::vector<name>& auths,
                                      const address&           payer,
                                      uint32_t                 max_charge = 1'000'000,
                                      uint32_t                 expiration = DEFAULT_EXPIRATION_DELTA);

    action get_action(action_name acttype, const domain_name& domain, const domain_key& key, const variant_object& data) const;

    void set_transaction_headers(signed_transaction& trx,
                                 const address&      payer,
                                 uint32_t            max_charge = 1'000'000,
                                 uint32_t            expiration = DEFAULT_EXPIRATION_DELTA) const;


    void push_genesis_block();

    void add_money(const address& addr, const asset& number);

    template <typename KeyType = fc::ecc::private_key_shim>
    static private_key_type
    get_private_key(name keyname, name salt = "none") {
        return private_key_type::regenerate<KeyType>(fc::sha256::hash(string(keyname) + string(salt)));
    }

    template <typename KeyType = fc::ecc::private_key_shim>
    static public_key_type
    get_public_key(name keyname, name salt = "none") {
        return get_private_key<KeyType>(keyname, salt).get_public_key();
    }

    bool                       chain_has_transaction(const transaction_id_type& txid) const;
    const transaction_receipt& get_transaction_receipt(const transaction_id_type& txid) const;

    static vector<uint8_t> to_uint8_vector(const string& s);

    static vector<uint8_t> to_uint8_vector(uint64_t x);

    static uint64_t to_uint64(fc::variant x);

    static string to_string(fc::variant x);

    static action_result
    success() { return string(); }

    static action_result
    error(const string& msg) { return msg; }

    void sync_with(base_tester& other);

    const controller::config&
    get_config() const {
        return cfg;
    }

protected:
    signed_block_ptr _produce_block(fc::microseconds skip_time, bool skip_pending_trxs = false, uint32_t skip_flag = 0);
    void             _start_block(fc::time_point block_time);

    // Fields:
protected:
    // tempdir field must come before control so that during destruction the tempdir is deleted only after controller finishes
    fc::temp_directory tempdir;

public:
    unique_ptr<controller>                                    control;
    std::map<chain::public_key_type, chain::private_key_type> block_signing_private_keys;

protected:
    controller::config                            cfg;
    map<transaction_id_type, transaction_receipt> chain_transactions;
    map<account_name, block_id_type>              last_produced_block;
};

class tester : public base_tester {
public:
    tester(bool push_genesis) {
        init(push_genesis);
    }

    tester() {
        init(true);
    }

    tester(controller::config config) {
        init(config);
    }

    signed_block_ptr
    produce_block(fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms), uint32_t skip_flag = 0 /*skip_missed_block_penalty*/) override {
        return _produce_block(skip_time, false, skip_flag);
    }

    signed_block_ptr
    produce_empty_block(fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms), uint32_t skip_flag = 0 /*skip_missed_block_penalty*/) override {
        control->abort_block();
        return _produce_block(skip_time, true, skip_flag);
    }

    bool
    validate() { return true; }
};

class validating_tester : public base_tester {
public:
    virtual ~validating_tester() {
        try {
            if(num_blocks_to_producer_before_shutdown > 0)
                produce_blocks(num_blocks_to_producer_before_shutdown);
            BOOST_REQUIRE_EQUAL(validate(), true);
        }
        catch(const fc::exception& e) {
            wdump((e.to_detail_string()));
        }
    }
    controller::config vcfg;

    validating_tester() {
        vcfg.blocks_dir            = tempdir.path() / std::string("v_").append(config::default_blocks_dir_name);
        vcfg.state_dir             = tempdir.path() / std::string("v_").append(config::default_state_dir_name);
        vcfg.db_config.db_path     = tempdir.path() / std::string("v_").append(config::default_token_database_dir_name);
        vcfg.state_size            = 1024 * 1024 * 8;
        vcfg.reversible_cache_size = 1024 * 1024 * 8;
        vcfg.contracts_console     = false;
        
        vcfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
        vcfg.genesis.initial_key       = get_public_key("evt");

        validating_node = std::make_unique<controller>(vcfg);
        validating_node->add_indices();
        validating_node->startup();

        init(true);
    }

    validating_tester(controller::config config) {
        FC_ASSERT(config.blocks_dir.filename().generic_string() != "."
                      && config.state_dir.filename().generic_string() != ".",
                  "invalid path names in controller::config");

        vcfg            = config;
        vcfg.blocks_dir = vcfg.blocks_dir.parent_path() / std::string("v_").append(vcfg.blocks_dir.filename().generic_string());
        vcfg.state_dir  = vcfg.state_dir.parent_path() / std::string("v_").append(vcfg.state_dir.filename().generic_string());

        validating_node = std::make_unique<controller>(vcfg);
        validating_node->add_indices();
        validating_node->startup();

        init(config);
    }

    signed_block_ptr
    produce_block(fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms), uint32_t skip_flag = 0 /*skip_missed_block_penalty*/) override {
        auto sb = _produce_block(skip_time, false, skip_flag | 2);
        validating_node->push_block(sb);

        return sb;
    }

    signed_block_ptr
    produce_empty_block(fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms), uint32_t skip_flag = 0 /*skip_missed_block_penalty*/) override {
        control->abort_block();
        auto sb = _produce_block(skip_time, true, skip_flag | 2);
        validating_node->push_block(sb);

        return sb;
    }

    bool
    validate() {
        auto hbh    = control->head_block_state()->header;
        auto vn_hbh = validating_node->head_block_state()->header;
        bool ok     = control->head_block_id() == validating_node->head_block_id() 
            && hbh.previous == vn_hbh.previous 
            && hbh.timestamp == vn_hbh.timestamp 
            && hbh.transaction_mroot == vn_hbh.transaction_mroot 
            && hbh.action_mroot == vn_hbh.action_mroot 
            && hbh.producer == vn_hbh.producer;

        validating_node.reset();
        validating_node = std::make_unique<controller>(vcfg);
        validating_node->add_indices();
        validating_node->startup();

        return ok;
    }

    unique_ptr<controller> validating_node;
    uint32_t               num_blocks_to_producer_before_shutdown = 0;
};

/**
    * Utility predicate to check whether an fc::exception message is equivalent to a given string
    */
struct fc_exception_message_is {
    fc_exception_message_is(const string& msg)
        : expected(msg) {}

    bool operator()(const fc::exception& ex);

    string expected;
};

/**
   * Utility predicate to check whether an fc::exception message starts with a given string
   */
struct fc_exception_message_starts_with {
    fc_exception_message_starts_with(const string& msg)
        : expected(msg) {}

    bool operator()(const fc::exception& ex);

    string expected;
};

/**
   * Utility predicate to check whether an fc::assert_exception message is equivalent to a given string
   */
struct fc_assert_exception_message_is {
    fc_assert_exception_message_is(const string& msg)
        : expected(msg) {}

    bool operator()(const fc::assert_exception& ex);

    string expected;
};

/**
   * Utility predicate to check whether an fc::assert_exception message starts with a given string
   */
struct fc_assert_exception_message_starts_with {
    fc_assert_exception_message_starts_with(const string& msg)
        : expected(msg) {}

    bool operator()(const fc::assert_exception& ex);

    string expected;
};

}}  // namespace evt::testing
