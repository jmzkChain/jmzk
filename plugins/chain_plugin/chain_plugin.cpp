/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/chain/block_log.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/fork_database.hpp>
#include <evt/chain/producer_object.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/plugin_interface.hpp>

#include <evt/chain/contracts/evt_contract.hpp>
#include <evt/chain/contracts/genesis_state.hpp>

#include <evt/utilities/key_conversion.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

namespace evt {

using namespace evt;
using namespace evt::chain;
using namespace evt::chain::config;
using namespace evt::chain::plugin_interface;
using fc::flat_map;

//using txn_msg_rate_limits = controller::txn_msg_rate_limits;

class chain_plugin_impl {
public:
    chain_plugin_impl()
        : accepted_block_header_channel(app().get_channel<channels::accepted_block_header>())
        , accepted_block_channel(app().get_channel<channels::accepted_block>())
        , irreversible_block_channel(app().get_channel<channels::irreversible_block>())
        , accepted_transaction_channel(app().get_channel<channels::accepted_transaction>())
        , applied_transaction_channel(app().get_channel<channels::applied_transaction>())
        , accepted_confirmation_channel(app().get_channel<channels::accepted_confirmation>())
        , incoming_block_channel(app().get_channel<incoming::channels::block>())
        , incoming_block_sync_method(app().get_method<incoming::methods::block_sync>())
        , incoming_transaction_sync_method(app().get_method<incoming::methods::transaction_sync>()) {}

    bfs::path                         block_log_dir;
    bfs::path                         tokendb_dir;
    bfs::path                         genesis_file;
    time_point                        genesis_timestamp;
    bool                              readonly = false;
    uint64_t                          shared_memory_size;
    flat_map<uint32_t, block_id_type> loaded_checkpoints;

    fc::optional<fork_database>      fork_db;
    fc::optional<block_log>          block_logger;
    fc::optional<controller::config> chain_config = controller::config();
    fc::optional<controller>         chain;
    chain_id_type                    chain_id;
    abi_serializer                   system_api;
    int32_t                          max_reversible_block_time_ms;
    int32_t                          max_pending_transaction_time_ms;

    // retained references to channels for easy publication
    channels::accepted_block_header::channel_type& accepted_block_header_channel;
    channels::accepted_block::channel_type&        accepted_block_channel;
    channels::irreversible_block::channel_type&    irreversible_block_channel;
    channels::accepted_transaction::channel_type&  accepted_transaction_channel;
    channels::applied_transaction::channel_type&   applied_transaction_channel;
    channels::accepted_confirmation::channel_type& accepted_confirmation_channel;
    incoming::channels::block::channel_type&       incoming_block_channel;

    // retained references to methods for easy calling
    incoming::methods::block_sync::method_type&       incoming_block_sync_method;
    incoming::methods::transaction_sync::method_type& incoming_transaction_sync_method;

    // method provider handles
    methods::get_block_by_number::method_type::handle                get_block_by_number_provider;
    methods::get_block_by_id::method_type::handle                    get_block_by_id_provider;
    methods::get_head_block_id::method_type::handle                  get_head_block_id_provider;
    methods::get_last_irreversible_block_number::method_type::handle get_last_irreversible_block_number_provider;
};

chain_plugin::chain_plugin()
    : my(new chain_plugin_impl()) {
}

chain_plugin::~chain_plugin() {}

void
chain_plugin::set_program_options(options_description& cli, options_description& cfg) {
    cfg.add_options()
        ("genesis-json", bpo::value<bfs::path>()->default_value("genesis.json"), "File to read Genesis State from")
        ("genesis-timestamp", bpo::value<string>(), "override the initial timestamp in the Genesis State file")
        ("block-log-dir", bpo::value<bfs::path>()->default_value("blocks"), "the location of the block log (absolute path or relative to application data dir)")
        ("tokendb-dir", bpo::value<bfs::path>()->default_value("tokendb"), "the location of the tokendb (absolute path or relative to application data dir)")
        ("checkpoint,c", bpo::value<vector<string>>()->composing(), "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
        ("shared-memory-size-mb", bpo::value<uint64_t>()->default_value(config::default_shared_memory_size / (1024 * 1024)), "Maximum size MB of database shared memory file");

    cli.add_options()
        ("replay-blockchain", bpo::bool_switch()->default_value(false), "clear chain database and replay all blocks")
        ("resync-blockchain", bpo::bool_switch()->default_value(false), "clear chain database and block log");
}

void
chain_plugin::plugin_initialize(const variables_map& options) {
    ilog("initializing chain plugin");

    if(options.count("genesis-json")) {
        auto genesis = options.at("genesis-json").as<bfs::path>();
        if(genesis.is_relative())
            my->genesis_file = app().config_dir() / genesis;
        else
            my->genesis_file = genesis;
    }
    if(options.count("genesis-timestamp")) {
        string tstr = options.at("genesis-timestamp").as<string>();
        if(strcasecmp(tstr.c_str(), "now") == 0) {
            my->genesis_timestamp = fc::time_point::now();
            auto epoch_ms         = my->genesis_timestamp.time_since_epoch().count() / 1000;
            auto diff_ms          = epoch_ms % block_interval_ms;
            if(diff_ms > 0) {
                auto delay_ms = (block_interval_ms - diff_ms);
                my->genesis_timestamp += fc::microseconds(delay_ms * 10000);
                dlog("pausing ${ms} milliseconds to the next interval", ("ms", delay_ms));
            }
        }
        else {
            my->genesis_timestamp = time_point::from_iso_string(tstr);
        }
    }
    if(options.count("block-log-dir")) {
        auto bld = options.at("block-log-dir").as<bfs::path>();
        if(bld.is_relative())
            my->block_log_dir = app().data_dir() / bld;
        else
            my->block_log_dir = bld;
    }
    if(options.count("shared-memory-size-mb")) {
        my->shared_memory_size = options.at("shared-memory-size-mb").as<uint64_t>() * 1024 * 1024;
    }
    if(options.count("tokendb-dir")) {
        auto bld = options.at("tokendb-dir").as<bfs::path>();
        if(bld.is_relative())
            my->tokendb_dir = app().data_dir() / bld;
        else
            my->tokendb_dir = bld;
    }

    if(options.at("replay-blockchain").as<bool>()) {
        ilog("Replay requested: wiping database");
        fc::remove_all(app().data_dir() / default_shared_memory_dir);
    }
    if(options.at("resync-blockchain").as<bool>()) {
        ilog("Resync requested: wiping database and blocks");
        fc::remove_all(app().data_dir() / default_shared_memory_dir);
        fc::remove_all(my->block_log_dir);
    }

    if(options.count("checkpoint")) {
        auto cps = options.at("checkpoint").as<vector<string>>();
        my->loaded_checkpoints.reserve(cps.size());
        for(auto cp : cps) {
            auto item                          = fc::json::from_string(cp).as<std::pair<uint32_t, block_id_type>>();
            my->loaded_checkpoints[item.first] = item.second;
        }
    }

    if(!fc::exists(my->genesis_file)) {
        wlog("\n generating default genesis file ${f}", ("f", my->genesis_file.generic_string()));
        contracts::genesis_state default_genesis;
        fc::json::save_to_file(default_genesis, my->genesis_file, true);
    }
    my->chain_config->block_log_dir      = my->block_log_dir;
    my->chain_config->tokendb_dir        = my->tokendb_dir;
    my->chain_config->shared_memory_dir  = app().data_dir() / default_shared_memory_dir;
    my->chain_config->read_only          = my->readonly;
    my->chain_config->shared_memory_size = my->shared_memory_size;
    my->chain_config->genesis            = fc::json::from_file(my->genesis_file).as<contracts::genesis_state>();
    if(my->genesis_timestamp.sec_since_epoch() > 0) {
        my->chain_config->genesis.initial_timestamp = my->genesis_timestamp;
    }

    my->chain.emplace(*my->chain_config);
    my->system_api = abi_serializer(contracts::evt_contract_abi());

    // set up method providers
    my->get_block_by_number_provider = app().get_method<methods::get_block_by_number>().register_provider([this](uint32_t block_num) -> signed_block_ptr {
        return my->chain->fetch_block_by_number(block_num);
    });

    my->get_block_by_id_provider = app().get_method<methods::get_block_by_id>().register_provider([this](block_id_type id) -> signed_block_ptr {
        return my->chain->fetch_block_by_id(id);
    });

    my->get_head_block_id_provider = app().get_method<methods::get_head_block_id>().register_provider([this]() {
        return my->chain->head_block_id();
    });

    my->get_last_irreversible_block_number_provider = app().get_method<methods::get_last_irreversible_block_number>().register_provider([this]() {
        return my->chain->last_irreversible_block_num();
    });

    // relay signals to channels
    my->chain->accepted_block_header.connect([this](const block_state_ptr& blk) {
        my->accepted_block_header_channel.publish(blk);
    });

    my->chain->accepted_block.connect([this](const block_state_ptr& blk) {
        my->accepted_block_channel.publish(blk);
    });

    my->chain->irreversible_block.connect([this](const block_state_ptr& blk) {
        my->irreversible_block_channel.publish(blk);
    });

    my->chain->accepted_transaction.connect([this](const transaction_metadata_ptr& meta) {
        my->accepted_transaction_channel.publish(meta);
    });

    my->chain->applied_transaction.connect([this](const transaction_trace_ptr& trace) {
        my->applied_transaction_channel.publish(trace);
    });

    my->chain->accepted_confirmation.connect([this](const header_confirmation& conf) {
        my->accepted_confirmation_channel.publish(conf);
    });
}

void
chain_plugin::plugin_startup() {
    try {
        my->chain->startup();

        if(!my->readonly) {
            ilog("starting chain in read/write mode");
            /// TODO: my->chain->add_checkpoints(my->loaded_checkpoints);
        }

        ilog("Blockchain started; head block is #${num}, genesis timestamp is ${ts}",
             ("num", my->chain->head_block_num())("ts", (std::string)my->chain_config->genesis.initial_timestamp));

        my->chain_config.reset();
    }
    FC_CAPTURE_LOG_AND_RETHROW((my->genesis_file.generic_string()))
}

void
chain_plugin::plugin_shutdown() {
    my->chain.reset();
}

chain_apis::read_only
chain_plugin::get_read_only_api() const {
    return chain_apis::read_only(chain(), my->system_api);
}

chain_apis::read_write
chain_plugin::get_read_write_api() {
    return chain_apis::read_write(chain(), my->system_api);
}

void
chain_plugin::accept_block(const signed_block_ptr& block) {
    my->incoming_block_sync_method(block);
}

void
chain_plugin::accept_transaction(const packed_transaction& trx) {
    my->incoming_transaction_sync_method(std::make_shared<packed_transaction>(trx));
}

bool
chain_plugin::block_is_on_preferred_chain(const block_id_type& block_id) {
    auto b = chain().fetch_block_by_number(block_header::num_from_id(block_id));
    return b && b->id() == block_id;
}

controller::config&
chain_plugin::chain_config() {
    // will trigger optional assert if called before/after plugin_initialize()
    return *my->chain_config;
}

controller&
chain_plugin::chain() {
    return *my->chain;
}
const controller&
chain_plugin::chain() const {
    return *my->chain;
}

void
chain_plugin::get_chain_id(chain_id_type& cid) const {
    memcpy(cid.data(), my->chain_id.data(), cid.data_size());
}

namespace chain_apis {

const string read_only::KEYi64       = "i64";
const string read_only::KEYstr       = "str";
const string read_only::KEYi128i128  = "i128i128";
const string read_only::KEYi64i64i64 = "i64i64i64";
const string read_only::PRIMARY      = "primary";
const string read_only::SECONDARY    = "secondary";
const string read_only::TERTIARY     = "tertiary";

read_only::get_info_results
read_only::get_info(const read_only::get_info_params&) const {
    auto itoh = [](uint32_t n, size_t hlen = sizeof(uint32_t) << 1) {
        static const char* digits = "0123456789abcdef";
        std::string        r(hlen, '0');
        for(size_t i = 0, j = (hlen - 1) * 4; i < hlen; ++i, j -= 4)
            r[i] = digits[(n >> j) & 0x0f];
        return r;
    };
    return {
        itoh(static_cast<uint32_t>(app().version())),
        db.head_block_num(),
        db.last_irreversible_block_num(),
        db.last_irreversible_block_id(),
        db.head_block_id(),
        db.head_block_time(),
        db.head_block_producer()};
}

template <typename Api>
auto
make_resolver(const Api* api) {
    return [api] {
        return api->system_api;
    };
}

fc::variant
read_only::get_block(const read_only::get_block_params& params) const {
    signed_block_ptr block;
    try {
        block = db.fetch_block_by_id(fc::json::from_string(params.block_num_or_id).as<block_id_type>());
        if(!block) {
            block = db.fetch_block_by_number(fc::to_uint64(params.block_num_or_id));
        }
    } EVT_RETHROW_EXCEPTIONS(chain::block_id_type_exception, "Invalid block ID: ${block_num_or_id}", ("block_num_or_id", params.block_num_or_id))

    EVT_ASSERT( block, unknown_block_exception, "Could not find block: ${block}", ("block", params.block_num_or_id));

    fc::variant pretty_output;
    abi_serializer::to_variant(*block, pretty_output, make_resolver(this));

    uint32_t ref_block_prefix = block->id()._hash[1];

    return fc::mutable_variant_object(pretty_output.get_object())
        ("id", block->id())
        ("block_num", block->block_num())
        ("ref_block_prefix", ref_block_prefix);
}

read_write::push_block_results
read_write::push_block(const read_write::push_block_params& params) {
    db.push_block(std::make_shared<signed_block>(params));
    return read_write::push_block_results();
}

read_write::push_transaction_results
read_write::push_transaction(const read_write::push_transaction_params& params) {
    auto pretty_input = std::make_shared<packed_transaction>();
    auto resolver     = make_resolver(this);
    abi_serializer::from_variant(params, *pretty_input, resolver);
    auto trx_trace_ptr = app().get_method<incoming::methods::transaction_sync>()(pretty_input);

    fc::variant pretty_output = db.to_variant_with_abi(*trx_trace_ptr);;
    return read_write::push_transaction_results{ trx_trace_ptr->id, pretty_output };
}

read_write::push_transactions_results
read_write::push_transactions(const read_write::push_transactions_params& params) {
    FC_ASSERT(params.size() <= 1000, "Attempt to push too many transactions at once");

    push_transactions_results result;
    result.reserve(params.size());
    for(const auto& item : params) {
        try {
            result.emplace_back(push_transaction(item));
        }
        catch(const fc::exception& e) {
            result.emplace_back(read_write::push_transaction_results{transaction_id_type(),
                                                                     fc::mutable_variant_object("error", e.to_detail_string())});
        }
    }
    return result;
}

static variant
action_abi_to_variant(const abi_serializer& api, contracts::type_name action_type) {
    variant v;
    auto it = api.structs.find(action_type);
    if(it != api.structs.end()) {
        to_variant(it->second.fields, v);
    }
    return v;
};

read_only::abi_json_to_bin_result
read_only::abi_json_to_bin(const read_only::abi_json_to_bin_params& params) const try {
    abi_json_to_bin_result result;
    auto& api = system_api;
    auto action_type = api.get_action_type(params.action);
    EVT_ASSERT(!action_type.empty(), action_validate_exception, "Unknown action ${action}", ("action", params.action));
    try {
        result.binargs = api.variant_to_binary(action_type, params.args);
    }
    EVT_RETHROW_EXCEPTIONS(chain::invalid_action_args_exception,
        "'${args}' is invalid args for action '${action}'. expected '${proto}'",
        ("args", params.args)("action", params.action)("proto", action_abi_to_variant(api, action_type)))
    return result;
}
FC_CAPTURE_AND_RETHROW((params.action)(params.args))

read_only::abi_bin_to_json_result
read_only::abi_bin_to_json(const read_only::abi_bin_to_json_params& params) const {
    abi_bin_to_json_result result;
    auto& api = system_api;
    result.args = api.binary_to_variant(api.get_action_type(params.action), params.binargs);
    return result;
}

read_only::get_required_keys_result
read_only::get_required_keys(const get_required_keys_params& params) const {
    transaction pretty_input;
    from_variant(params.transaction, pretty_input);
    auto required_keys_set = db.get_required_keys(pretty_input, params.available_keys);

    get_required_keys_result result;
    result.required_keys = required_keys_set;
    return result;
}

}  // namespace chain_apis
}  // namespace evt
