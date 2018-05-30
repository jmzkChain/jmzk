/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/chain/block_log.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/fork_database.hpp>
#include <evt/chain/reversible_block_object.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/genesis_state.hpp>
#include <evt/chain/contracts/evt_contract.hpp>

#include <evt/utilities/key_conversion.hpp>

#include <boost/signals2/connection.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <signal.h>

namespace evt {

using namespace evt;
using namespace evt::chain;
using namespace evt::chain::config;
using namespace evt::chain::plugin_interface;
using boost::signals2::scoped_connection;
using fc::flat_map;
using fc::json;

#define CATCH_AND_CALL(NEXT)                                               \
    catch(const fc::exception& err) {                                      \
        NEXT(err.dynamic_copy_exception());                                \
    }                                                                      \
    catch(const std::exception& e) {                                       \
        fc::exception fce(                                                 \
            FC_LOG_MESSAGE(warn, "rethrow ${what}: ", ("what", e.what())), \
            fc::std_exception_code,                                        \
            BOOST_CORE_TYPEID(e).name(),                                   \
            e.what());                                                     \
        NEXT(fce.dynamic_copy_exception());                                \
    }                                                                      \
    catch(...) {                                                           \
        fc::unhandled_exception e(                                         \
            FC_LOG_MESSAGE(warn, "rethrow"),                               \
            std::current_exception());                                     \
        NEXT(e.dynamic_copy_exception());                                  \
    }

class chain_plugin_impl {
public:
    chain_plugin_impl()
        : system_api(chain::contracts::evt_contract_abi())
        , accepted_block_header_channel(app().get_channel<channels::accepted_block_header>())
        , accepted_block_channel(app().get_channel<channels::accepted_block>())
        , irreversible_block_channel(app().get_channel<channels::irreversible_block>())
        , accepted_transaction_channel(app().get_channel<channels::accepted_transaction>())
        , applied_transaction_channel(app().get_channel<channels::applied_transaction>())
        , accepted_confirmation_channel(app().get_channel<channels::accepted_confirmation>())
        , incoming_block_channel(app().get_channel<incoming::channels::block>())
        , incoming_block_sync_method(app().get_method<incoming::methods::block_sync>())
        , incoming_transaction_async_method(app().get_method<incoming::methods::transaction_async>()) {}

    bfs::path                         blocks_dir;
    bfs::path                         tokendb_dir;
    bool                              readonly = false;
    uint64_t                          shared_memory_size;
    flat_map<uint32_t, block_id_type> loaded_checkpoints;

    fc::optional<fork_database>      fork_db;
    fc::optional<block_log>          block_logger;
    fc::optional<controller::config> chain_config;
    fc::optional<controller>         chain;
    fc::optional<chain_id_type>      chain_id;
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
    incoming::methods::block_sync::method_type&        incoming_block_sync_method;
    incoming::methods::transaction_async::method_type& incoming_transaction_async_method;

    // method provider handles
    methods::get_block_by_number::method_type::handle                get_block_by_number_provider;
    methods::get_block_by_id::method_type::handle                    get_block_by_id_provider;
    methods::get_head_block_id::method_type::handle                  get_head_block_id_provider;
    methods::get_last_irreversible_block_number::method_type::handle get_last_irreversible_block_number_provider;

    // scoped connections for chain controller
    fc::optional<scoped_connection> accepted_block_header_connection;
    fc::optional<scoped_connection> accepted_block_connection;
    fc::optional<scoped_connection> irreversible_block_connection;
    fc::optional<scoped_connection> accepted_transaction_connection;
    fc::optional<scoped_connection> applied_transaction_connection;
    fc::optional<scoped_connection> accepted_confirmation_connection;
};

chain_plugin::chain_plugin()
    : my(new chain_plugin_impl()) {
}

chain_plugin::~chain_plugin() {}

void
chain_plugin::set_program_options(options_description& cli, options_description& cfg) {
    cfg.add_options()
        ("blocks-dir", bpo::value<bfs::path>()->default_value("blocks"), "the location of the blocks directory (absolute path or relative to application data dir)")
        ("tokendb-dir", bpo::value<bfs::path>()->default_value("tokendb"), "the location of the token database directory (absolute path or relative to application data dir)")
        ("checkpoint", bpo::value<vector<string>>()->composing(), "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
        ("chain-state-db-size-mb", bpo::value<uint64_t>()->default_value(config::default_state_size / (1024 * 1024)), "Maximum size (in MB) of the chain state database")
        ("reversible-blocks-db-size-mb", bpo::value<uint64_t>()->default_value(config::default_reversible_cache_size / (1024 * 1024)), "Maximum size (in MB) of the reversible blocks database")
        ("contracts-console", bpo::bool_switch()->default_value(false), "print contract's output to console");

    cli.add_options()
        ("genesis-json", bpo::value<bfs::path>(), "File to read Genesis State from")
        ("genesis-timestamp", bpo::value<string>(), "override the initial timestamp in the Genesis State file")
        ("print-genesis-json", bpo::bool_switch()->default_value(false), "extract genesis_state from blocks.log as JSON, print to console, and exit")
        ("extract-genesis-json", bpo::value<bfs::path>(), "extract genesis_state from blocks.log as JSON, write into specified file, and exit")
        ("fix-reversible-blocks", bpo::bool_switch()->default_value(false), "recovers reversible block database if that database is in a bad state")
        ("force-all-checks", bpo::bool_switch()->default_value(false), "do not skip any checks that can be skipped while replaying irreversible blocks")
        ("replay-blockchain", bpo::bool_switch()->default_value(false), "clear chain state database and replay all blocks")
        ("hard-replay-blockchain", bpo::bool_switch()->default_value(false), "clear chain state database, recover as many blocks as possible from the block log, and then replay those blocks")
        ("delete-all-blocks", bpo::bool_switch()->default_value(false), "clear chain state database and block log");
}

fc::time_point
calculate_genesis_timestamp( string tstr ) {
fc::time_point genesis_timestamp;
    if(strcasecmp (tstr.c_str(), "now") == 0) {
        genesis_timestamp = fc::time_point::now();
    }
    else {
        genesis_timestamp = time_point::from_iso_string(tstr);
    }

    auto epoch_us = genesis_timestamp.time_since_epoch().count();
    auto diff_us = epoch_us % config::block_interval_us;
    if(diff_us > 0) {
        auto delay_us = (config::block_interval_us - diff_us);
        genesis_timestamp += fc::microseconds(delay_us);
        dlog("pausing ${us} microseconds to the next interval",("us",delay_us));
    }

    ilog("Adjusting genesis timestamp to ${timestamp}", ("timestamp", genesis_timestamp));
    return genesis_timestamp;
}


void
chain_plugin::plugin_initialize(const variables_map& options) {
    ilog("initializing chain plugin");

    try {
        genesis_state gs;  // Check if EVT_ROOT_KEY is bad
    }
    catch(const fc::exception&) {
        elog("EVT_ROOT_KEY ('${root_key}') is invalid. Recompile with a valid public key.", ("root_key", genesis_state::evt_root_key));
        throw;
    }

    my->chain_config = controller::config();

    if(options.count("blocks-dir")) {
        auto bld = options.at("blocks-dir").as<bfs::path>();
        if(bld.is_relative())
            my->blocks_dir = app().data_dir() / bld;
        else
            my->blocks_dir = bld;
    }

    if(options.count("tokendb-dir")) {
        auto bld = options.at("tokendb-dir").as<bfs::path>();
        if(bld.is_relative())
            my->tokendb_dir = app().data_dir() / bld;
        else
            my->tokendb_dir = bld;
    }

    if(options.count("checkpoint")) {
        auto cps = options.at("checkpoint").as<vector<string>>();
        my->loaded_checkpoints.reserve(cps.size());
        for(auto cp : cps) {
            auto item                          = fc::json::from_string(cp).as<std::pair<uint32_t, block_id_type>>();
            my->loaded_checkpoints[item.first] = item.second;
        }
    }

    my->chain_config->blocks_dir = my->blocks_dir;
    my->chain_config->state_dir  = app().data_dir() / config::default_state_dir_name;
    my->chain_config->read_only  = my->readonly;

    if(options.count("chain-state-db-size-mb"))
        my->chain_config->state_size = options.at("chain-state-db-size-mb").as<uint64_t>() * 1024 * 1024;

    if(options.count("reversible-blocks-db-size-mb"))
        my->chain_config->reversible_cache_size = options.at("reversible-blocks-db-size-mb").as<uint64_t>() * 1024 * 1024;

    my->chain_config->force_all_checks  = options.at("force-all-checks").as<bool>();
    my->chain_config->contracts_console = options.at("contracts-console").as<bool>();

    if(options.count("extract-genesis-json") || options.at("print-genesis-json").as<bool>()) {
        genesis_state gs;

        if(fc::exists(my->blocks_dir / "blocks.log")) {
            gs = block_log::extract_genesis_state(my->blocks_dir);
        }
        else {
            wlog("No blocks.log found at '${p}'. Using default genesis state.", ("p", (my->blocks_dir / "blocks.log").generic_string()));
        }

        if(options.at("print-genesis-json").as<bool>()) {
            ilog("Genesis JSON:\n${genesis}", ("genesis", json::to_pretty_string(gs)));
        }

        if(options.count("extract-genesis-json")) {
            auto p = options.at("extract-genesis-json").as<bfs::path>();

            if(p.is_relative()) {
                p = bfs::current_path() / p;
            }

            fc::json::save_to_file(gs, p, true);
            ilog("Saved genesis JSON to '${path}'", ("path", p.generic_string()));
        }

        EVT_THROW(extract_genesis_state_exception, "extracted genesis state from blocks.log");
    }

    if(options.at("delete-all-blocks").as<bool>()) {
        ilog("Deleting state database and blocks");
        fc::remove_all(my->chain_config->state_dir);
        fc::remove_all(my->blocks_dir);
    }
    else if(options.at("hard-replay-blockchain").as<bool>()) {
        ilog("Hard replay requested: deleting state database");
        fc::remove_all(my->chain_config->state_dir);
        auto backup_dir = block_log::repair_log(my->blocks_dir);
        if(fc::exists(backup_dir / config::reversible_blocks_dir_name) || options.at("fix-reversible-blocks").as<bool>()) {
            // Do not try to recover reversible blocks if the directory does not exist, unless the option was explicitly provided.
            if(!recover_reversible_blocks(backup_dir / config::reversible_blocks_dir_name,
                                          my->chain_config->reversible_cache_size,
                                          my->chain_config->blocks_dir / config::reversible_blocks_dir_name)) {
                ilog("Reversible blocks database was not corrupted. Copying from backup to blocks directory.");
                fc::copy(backup_dir / config::reversible_blocks_dir_name, my->chain_config->blocks_dir / config::reversible_blocks_dir_name);
                fc::copy(backup_dir / "reversible/shared_memory.bin", my->chain_config->blocks_dir / "reversible/shared_memory.bin");
                fc::copy(backup_dir / "reversible/shared_memory.meta", my->chain_config->blocks_dir / "reversible/shared_memory.meta");
            }
        }
    }
    else if(options.at("replay-blockchain").as<bool>()) {
        ilog("Replay requested: deleting state database");
        fc::remove_all(my->chain_config->state_dir);
        if(options.at("fix-reversible-blocks").as<bool>()) {
            if(!recover_reversible_blocks(my->chain_config->blocks_dir / config::reversible_blocks_dir_name,
                                          my->chain_config->reversible_cache_size)) {
                ilog("Reversible blocks database was not corrupted.");
            }
        }
    }
    else if(options.at("fix-reversible-blocks").as<bool>()) {
        if(!recover_reversible_blocks(my->chain_config->blocks_dir / config::reversible_blocks_dir_name,
                                      my->chain_config->reversible_cache_size)) {
            ilog("Reversible blocks database verified to not be corrupted. Now exiting...");
        }
        else {
            ilog("Exiting after fixing reversible blocks database...");
        }
        EVT_THROW(fixed_reversible_db_exception, "fixed corrupted reversible blocks database");
    }

    if(options.count("genesis-json")) {
        FC_ASSERT(!fc::exists(my->blocks_dir / "blocks.log"), "Genesis state can only be set on a fresh blockchain.");

        auto genesis_file = options.at("genesis-json").as<bfs::path>();
        if(genesis_file.is_relative()) {
            genesis_file = bfs::current_path() / genesis_file;
        }

        FC_ASSERT(fc::is_regular_file(genesis_file), "Specified genesis file '${genesis}' does not exist.", ("genesis", genesis_file.generic_string()));

        my->chain_config->genesis = fc::json::from_file(genesis_file).as<genesis_state>();

        ilog("Using genesis state provided in '${genesis}'", ("genesis", genesis_file.generic_string()));

        if(options.count("genesis-timestamp")) {
            my->chain_config->genesis.initial_timestamp = calculate_genesis_timestamp(options.at("genesis-timestamp").as<string>());
        }

        wlog("Starting up fresh blockchain with provided genesis state.");
    }
    else if(options.count("genesis-timestamp")) {
        FC_ASSERT(!fc::exists( my->blocks_dir / "blocks.log" ), "Genesis state can only be set on a fresh blockchain.");

        my->chain_config->genesis.initial_timestamp = calculate_genesis_timestamp(options.at("genesis-timestamp").as<string>());

        wlog("Starting up fresh blockchain with default genesis state but with adjusted genesis timestamp.");
    }
    else if(fc::is_regular_file(my->blocks_dir / "blocks.log")) {
        my->chain_config->genesis = block_log::extract_genesis_state(my->blocks_dir);
    }
    else {
        wlog("Starting up fresh blockchain with default genesis state.");
    }

    my->chain.emplace(*my->chain_config);
    my->chain_id.emplace(my->chain->get_chain_id());

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
    my->accepted_block_header_connection = my->chain->accepted_block_header.connect([this](const block_state_ptr& blk) {
        my->accepted_block_header_channel.publish(blk);
    });

    my->accepted_block_connection = my->chain->accepted_block.connect([this](const block_state_ptr& blk) {
        my->accepted_block_channel.publish(blk);
    });

    my->irreversible_block_connection = my->chain->irreversible_block.connect([this](const block_state_ptr& blk) {
        my->irreversible_block_channel.publish(blk);
    });

    my->accepted_transaction_connection = my->chain->accepted_transaction.connect([this](const transaction_metadata_ptr& meta) {
        my->accepted_transaction_channel.publish(meta);
    });

    my->applied_transaction_connection = my->chain->applied_transaction.connect([this](const transaction_trace_ptr& trace) {
        my->applied_transaction_channel.publish(trace);
    });

    my->accepted_confirmation_connection = my->chain->accepted_confirmation.connect([this](const header_confirmation& conf) {
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
    FC_CAPTURE_AND_RETHROW()
}

void
chain_plugin::plugin_shutdown() {
    my->accepted_block_header_connection.reset();
    my->accepted_block_connection.reset();
    my->irreversible_block_connection.reset();
    my->accepted_transaction_connection.reset();
    my->applied_transaction_connection.reset();
    my->accepted_confirmation_connection.reset();
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
chain_plugin::accept_transaction(const chain::packed_transaction& trx, next_function<chain::transaction_trace_ptr> next) {
    my->incoming_transaction_async_method(std::make_shared<packed_transaction>(trx), false, std::forward<decltype(next)>(next));
}

bool
chain_plugin::block_is_on_preferred_chain(const block_id_type& block_id) {
    auto b = chain().fetch_block_by_number(block_header::num_from_id(block_id));
    return b && b->id() == block_id;
}

bool
chain_plugin::recover_reversible_blocks(const fc::path& db_dir, uint32_t cache_size, optional<fc::path> new_db_dir) const {
    try {
        chainbase::database reversible(db_dir, database::read_only);  // Test if dirty
        return false;                                                 // If it reaches here, then the reversible database is not dirty
    }
    catch(const std::runtime_error&) {
    }
    catch(...) {
        throw;
    }
    // Reversible block database is dirty. So back it up (unless already moved) and then create a new one.

    auto reversible_dir = fc::canonical(db_dir);
    if(reversible_dir.filename().generic_string() == ".") {
        reversible_dir = reversible_dir.parent_path();
    }
    fc::path backup_dir;

    if(new_db_dir) {
        backup_dir     = reversible_dir;
        reversible_dir = *new_db_dir;
    }
    else {
        auto now = fc::time_point::now();

        auto reversible_dir_name = reversible_dir.filename().generic_string();
        FC_ASSERT(reversible_dir_name != ".", "Invalid path to reversible directory");
        backup_dir = reversible_dir.parent_path() / reversible_dir_name.append("-").append(now);

        FC_ASSERT(!fc::exists(backup_dir),
                  "Cannot move existing reversible directory to already existing directory '${backup_dir}'",
                  ("backup_dir", backup_dir));

        fc::rename(reversible_dir, backup_dir);
        ilog("Moved existing reversible directory to backup location: '${new_db_dir}'", ("new_db_dir", backup_dir));
    }

    fc::create_directories(reversible_dir);

    ilog("Reconstructing '${reversible_dir}' from backed up reversible directory", ("reversible_dir", reversible_dir));

    chainbase::database old_reversible(backup_dir, database::read_only, 0, true);
    chainbase::database new_reversible(reversible_dir, database::read_write, cache_size);

    uint32_t num   = 0;
    uint32_t start = 0;
    uint32_t end   = 0;
    try {
        old_reversible.add_index<reversible_block_index>();
        new_reversible.add_index<reversible_block_index>();
        const auto& ubi = old_reversible.get_index<reversible_block_index, by_num>();
        auto        itr = ubi.begin();
        if(itr != ubi.end()) {
            start = itr->blocknum;
            end   = start - 1;
        }
        for(; itr != ubi.end(); ++itr) {
            FC_ASSERT(itr->blocknum == end + 1, "gap in reversible block database");
            new_reversible.create<reversible_block_object>([&](auto& ubo) {
                ubo.blocknum = itr->blocknum;
                ubo.set_block(itr->get_block());  // get_block and set_block rather than copying the packed data acts as additional validation
            });
            end = itr->blocknum;
            ++num;
        }
    }
    catch(...) {
    }

    if(num == 0)
        ilog("There were no recoverable blocks in the reversible block database");
    else if(num == 1)
        ilog("Recovered 1 block from reversible block database: block ${start}", ("start", start));
    else
        ilog("Recovered ${num} blocks from reversible block database: blocks ${start} to ${end}",
             ("num", num)("start", start)("end", end));

    return true;
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

chain::chain_id_type
chain_plugin::get_chain_id() const {
    FC_ASSERT(my->chain_id.valid(), "chain ID has not been initialized yet");
    return *my->chain_id;
}

namespace chain_apis {

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
        db.get_chain_id(),
        contracts::evt_contract_abi_version(),
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
    }
    EVT_RETHROW_EXCEPTIONS(chain::block_id_type_exception, "Invalid block ID: ${block_num_or_id}", ("block_num_or_id", params.block_num_or_id))

    EVT_ASSERT(block, unknown_block_exception, "Could not find block: ${block}", ("block", params.block_num_or_id));

    fc::variant pretty_output;
    abi_serializer::to_variant(*block, pretty_output, make_resolver(this));

    uint32_t ref_block_prefix = block->id()._hash[1];

    return fc::mutable_variant_object(pretty_output.get_object())("id", block->id())("block_num", block->block_num())("ref_block_prefix", ref_block_prefix);
}

void
read_write::push_block(const read_write::push_block_params& params, next_function<read_write::push_block_results> next) {
    try {
        app().get_method<incoming::methods::block_sync>()(std::make_shared<signed_block>(params));
        next(read_write::push_block_results{});
    }
    catch(boost::interprocess::bad_alloc&) {
        raise(SIGUSR1);
    }
    CATCH_AND_CALL(next);
}

void
read_write::push_transaction(const read_write::push_transaction_params& params, next_function<read_write::push_transaction_results> next) {
    try {
        auto pretty_input = std::make_shared<packed_transaction>();
        auto resolver     = make_resolver(this);
        try {
            abi_serializer::from_variant(params, *pretty_input, resolver);
        }
        EVT_RETHROW_EXCEPTIONS(chain::packed_transaction_type_exception, "Invalid packed transaction")

        app().get_method<incoming::methods::transaction_async>()(pretty_input, true, [this, next](const fc::static_variant<fc::exception_ptr, transaction_trace_ptr>& result) -> void {
            if(result.contains<fc::exception_ptr>()) {
                next(result.get<fc::exception_ptr>());
            }
            else {
                auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                try {
                    fc::variant pretty_output;
                    pretty_output = db.to_variant_with_abi(*trx_trace_ptr);
                    //abi_serializer::to_variant(*trx_trace_ptr, pretty_output, resolver);

                    chain::transaction_id_type id = trx_trace_ptr->id;
                    next(read_write::push_transaction_results{id, pretty_output});
                }
                CATCH_AND_CALL(next);
            }
        });
    }
    catch(boost::interprocess::bad_alloc&) {
        raise(SIGUSR1);
    }
    CATCH_AND_CALL(next);
}

static void
push_recurse(read_write* rw, int index, const std::shared_ptr<read_write::push_transactions_params>& params, const std::shared_ptr<read_write::push_transactions_results>& results, const next_function<read_write::push_transactions_results>& next) {
    auto wrapped_next = [=](const fc::static_variant<fc::exception_ptr, read_write::push_transaction_results>& result) {
        if(result.contains<fc::exception_ptr>()) {
            const auto& e = result.get<fc::exception_ptr>();
            results->emplace_back(read_write::push_transaction_results{transaction_id_type(), fc::mutable_variant_object("error", e->to_detail_string())});
        }
        else {
            const auto& r = result.get<read_write::push_transaction_results>();
            results->emplace_back(r);
        }

        int next_index = index + 1;
        if(next_index < params->size()) {
            push_recurse(rw, next_index, params, results, next);
        }
        else {
            next(*results);
        }
    };

    rw->push_transaction(params->at(index), wrapped_next);
}

void
read_write::push_transactions(const read_write::push_transactions_params& params, next_function<read_write::push_transactions_results> next) {
    try {
        FC_ASSERT(params.size() <= 1000, "Attempt to push too many transactions at once");
        auto params_copy = std::make_shared<read_write::push_transactions_params>(params.begin(), params.end());
        auto result      = std::make_shared<read_write::push_transactions_results>();
        result->reserve(params.size());

        push_recurse(this, 0, params_copy, result, next);
    }
    CATCH_AND_CALL(next);
}

static variant
action_abi_to_variant(const abi_serializer& api, contracts::type_name action_type) {
    variant v;
    auto    it = api.structs.find(action_type);
    if(it != api.structs.end()) {
        to_variant(it->second.fields, v);
    }
    return v;
};

read_only::abi_json_to_bin_result
read_only::abi_json_to_bin(const read_only::abi_json_to_bin_params& params) const try {
    auto  result      = abi_json_to_bin_result();
    auto& api         = system_api;
    auto  action_type = api.get_action_type(params.action);
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
    auto  result = abi_bin_to_json_result();
    auto& api    = system_api;
    result.args  = api.binary_to_variant(api.get_action_type(params.action), params.binargs);
    return result;
}

read_only::trx_json_to_digest_result
read_only::trx_json_to_digest(const trx_json_to_digest_params& params) const {
    auto result = trx_json_to_digest_result();
    try {
        auto trx      = std::make_shared<transaction>();
        auto resolver = make_resolver(this);
        try {
            abi_serializer::from_variant(params, *trx, resolver);
        }
        EVT_RETHROW_EXCEPTIONS(chain::packed_transaction_type_exception, "Invalid transaction")
        result.digest = trx->sig_digest(db.get_chain_id());
    }
    catch(boost::interprocess::bad_alloc&) {
        raise(SIGUSR1);
    }
    catch(...) {
        throw;
    }
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
