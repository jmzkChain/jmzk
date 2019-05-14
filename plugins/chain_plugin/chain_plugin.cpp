/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <evt/chain_plugin/chain_plugin.hpp>

#include <signal.h>
#include <stdlib.h>

#include <boost/signals2/connection.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <evt/chain/block_log.hpp>
#include <evt/chain/config.hpp>
#include <evt/chain/exceptions.hpp>
#include <evt/chain/fork_database.hpp>
#include <evt/chain/reversible_block_object.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/genesis_state.hpp>
#include <evt/chain/snapshot.hpp>
#include <evt/chain/contracts/evt_contract_abi.hpp>
#include <evt/chain/contracts/evt_link.hpp>
#include <evt/chain/contracts/evt_link_object.hpp>

#include <evt/utilities/key_conversion.hpp>

namespace evt {

namespace chain {

std::ostream&
operator<<(std::ostream& osm, evt::chain::db_read_mode m) {
    if(m == evt::chain::db_read_mode::SPECULATIVE) {
        osm << "speculative";
    }
    else if(m == evt::chain::db_read_mode::HEAD) {
        osm << "head";
    }
    else if(m == evt::chain::db_read_mode::READ_ONLY) {
        osm << "read-only";
    }
    else if(m == evt::chain::db_read_mode::IRREVERSIBLE) {
        osm << "irreversible";
    }

    return osm;
}

void
validate(boost::any&                     v,
         const std::vector<std::string>& values,
         evt::chain::db_read_mode* /* target_type */,
         int) {
    using namespace boost::program_options;

    // Make sure no previous assignment to 'v' was made.
    validators::check_first_occurrence(v);

    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    std::string const& s = validators::get_single_string(values);

    if(s == "speculative") {
        v = boost::any(evt::chain::db_read_mode::SPECULATIVE);
    }
    else if(s == "head") {
        v = boost::any(evt::chain::db_read_mode::HEAD);
    }
    else if(s == "read-only") {
        v = boost::any(evt::chain::db_read_mode::READ_ONLY);
    }
    else if(s == "irreversible") {
        v = boost::any(evt::chain::db_read_mode::IRREVERSIBLE);
    }
    else {
        throw validation_error(validation_error::invalid_option_value);
    }
}

std::ostream&
operator<<(std::ostream& osm, evt::chain::validation_mode m) {
    if(m == evt::chain::validation_mode::FULL) {
        osm << "full";
    }
    else if(m == evt::chain::validation_mode::LIGHT) {
        osm << "light";
    }

    return osm;
}

void
validate(boost::any&                     v,
         const std::vector<std::string>& values,
         evt::chain::validation_mode* /* target_type */,
         int) {
    using namespace boost::program_options;

    // Make sure no previous assignment to 'v' was made.
    validators::check_first_occurrence(v);

    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    std::string const& s = validators::get_single_string(values);

    if(s == "full") {
        v = boost::any(evt::chain::validation_mode::FULL);
    }
    else if(s == "light") {
        v = boost::any(evt::chain::validation_mode::LIGHT);
    }
    else {
        throw validation_error(validation_error::invalid_option_value);
    }
}

std::ostream&
operator<<(std::ostream& osm, evt::chain::storage_profile m) {
    if(m == evt::chain::storage_profile::disk) {
        osm << "disk";
    }
    else if(m == evt::chain::storage_profile::memory) {
        osm << "memory";
    }

    return osm;
}

void
validate(boost::any&                     v,
         const std::vector<std::string>& values,
         evt::chain::storage_profile* /* target_type */,
         int) {
    using namespace boost::program_options;

    // Make sure no previous assignment to 'v' was made.
    validators::check_first_occurrence(v);

    // Extract the first string from 'values'. If there is more than
    // one string, it's an error, and exception will be thrown.
    std::string const& s = validators::get_single_string(values);

    if(s == "disk") {
        v = boost::any(evt::chain::storage_profile::disk);
    }
    else if(s == "memory") {
        v = boost::any(evt::chain::storage_profile::memory);
    }
    else {
        throw validation_error(validation_error::invalid_option_value);
    }
}

}  // namespace chain

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
        : pre_accepted_block_channel(app().get_channel<channels::pre_accepted_block>())
        , accepted_block_header_channel(app().get_channel<channels::accepted_block_header>())
        , accepted_block_channel(app().get_channel<channels::accepted_block>())
        , irreversible_block_channel(app().get_channel<channels::irreversible_block>())
        , accepted_transaction_channel(app().get_channel<channels::accepted_transaction>())
        , applied_transaction_channel(app().get_channel<channels::applied_transaction>())
        , incoming_block_channel(app().get_channel<incoming::channels::block>())
        , incoming_block_sync_method(app().get_method<incoming::methods::block_sync>())
        , incoming_transaction_async_method(app().get_method<incoming::methods::transaction_async>()) {}

    bfs::path                         blocks_dir;
    bfs::path                         tokendb_dir;
    bool                              readonly = false;
    flat_map<uint32_t, block_id_type> loaded_checkpoints;

    std::optional<fork_database>      fork_db;
    std::optional<block_log>          block_logger;
    std::optional<controller::config> chain_config;
    std::optional<controller>         chain;
    std::optional<chain_id_type>      chain_id;
    std::optional<bfs::path>          snapshot_path;

    // retained references to channels for easy publication
    channels::pre_accepted_block::channel_type&    pre_accepted_block_channel;
    channels::accepted_block_header::channel_type& accepted_block_header_channel;
    channels::accepted_block::channel_type&        accepted_block_channel;
    channels::irreversible_block::channel_type&    irreversible_block_channel;
    channels::accepted_transaction::channel_type&  accepted_transaction_channel;
    channels::applied_transaction::channel_type&   applied_transaction_channel;
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
    std::optional<scoped_connection> pre_accepted_block_connection;
    std::optional<scoped_connection> accepted_block_header_connection;
    std::optional<scoped_connection> accepted_block_connection;
    std::optional<scoped_connection> irreversible_block_connection;
    std::optional<scoped_connection> accepted_transaction_connection;
    std::optional<scoped_connection> applied_transaction_connection;
};

chain_plugin::chain_plugin()
    :my(new chain_plugin_impl()) {
    app().register_config_type<evt::chain::db_read_mode>();
    app().register_config_type<evt::chain::validation_mode>();
    app().register_config_type<evt::chain::storage_profile>();
}

chain_plugin::~chain_plugin() {}

void
chain_plugin::set_program_options(options_description& cli, options_description& cfg) {
    cfg.add_options()
        ("blocks-dir", bpo::value<bfs::path>()->default_value("blocks"), "the location of the blocks directory (absolute path or relative to application data dir)")
        ("token-db-dir", bpo::value<bfs::path>()->default_value("tokendb"), "the location of the token database directory (absolute path or relative to application data dir)")
        ("token-db-cache-size-mb", bpo::value<uint32_t>()->default_value(512), "the cache size of token database in MBytes")
        ("token-db-profile", boost::program_options::value<evt::chain::storage_profile>()->default_value(evt::chain::storage_profile::disk),
            "Token database profile (\"disk\", or \"memory\").\n"
            "In \"disk\" profile database is optimized for the standard storage devices.\n"
            "In \"memory\" mode database is optimized for the usage in ultra-low latency devices like memory\n"
        )
        ("checkpoint", bpo::value<vector<string>>()->composing(), "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
        ("abi-serializer-max-time-ms", bpo::value<uint32_t>()->default_value(config::default_abi_serializer_max_time_ms), "Override default maximum ABI serialization time allowed in ms")
        ("chain-state-db-size-mb", bpo::value<uint64_t>()->default_value(config::default_state_size / (1024 * 1024)), "Maximum size (in MiB) of the chain state database")
        ("chain-state-db-guard-size-mb", bpo::value<uint64_t>()->default_value(config::default_state_guard_size / (1024 * 1024)), "Safely shut down node when free space remaining in the chain state database drops below this size (in MiB).")
        ("reversible-blocks-db-size-mb", bpo::value<uint64_t>()->default_value(config::default_reversible_cache_size / (1024 * 1024)), "Maximum size (in MiB) of the reversible blocks database")
        ("reversible-blocks-db-guard-size-mb", bpo::value<uint64_t>()->default_value(config::default_reversible_guard_size / (1024 * 1024)), "Safely shut down node when free space remaining in the reverseible blocks database drops below this size (in MiB).")
        ("contracts-console", bpo::bool_switch()->default_value(false), "print contract's output to console")
        ("read-mode", boost::program_options::value<evt::chain::db_read_mode>()->default_value(evt::chain::db_read_mode::SPECULATIVE),
            "Database read mode (\"speculative\", \"head\", or \"read-only\").\n"// or \"irreversible\").\n"
            "In \"speculative\" mode database contains changes done up to the head block plus changes made by transactions not yet included to the blockchain.\n"
            "In \"head\" mode database contains changes done up to the current head block.\n"
            "In \"read-only\" mode database contains incoming block changes but no speculative transaction processing.\n"
        )
        //"In \"irreversible\" mode database contains changes done up the current irreversible block.\n")
        ("validation-mode", boost::program_options::value<evt::chain::validation_mode>()->default_value(evt::chain::validation_mode::FULL),
            "Chain validation mode (\"full\" or \"light\").\n"
            "In \"full\" mode all incoming blocks will be fully validated.\n"
            "In \"light\" mode all incoming blocks headers will be fully validated; transactions in those validated blocks will be trusted \n")
        ("trusted-producer", bpo::value<vector<string>>()->composing(), "Indicate a producer whose blocks headers signed by it will be fully validated, but transactions in those validated blocks will be trusted.")
        ;

    cli.add_options()
        ("genesis-json", bpo::value<bfs::path>(), "File to read Genesis State from")
        ("genesis-timestamp", bpo::value<string>(), "override the initial timestamp in the Genesis State file")
        ("print-genesis-json", bpo::bool_switch()->default_value(false), "extract genesis_state from blocks.log as JSON, print to console, and exit")
        ("extract-genesis-json", bpo::value<bfs::path>(), "extract genesis_state from blocks.log as JSON, write into specified file, and exit")
        ("fix-reversible-blocks", bpo::bool_switch()->default_value(false), "recovers reversible block database if that database is in a bad state")
        ("force-all-checks", bpo::bool_switch()->default_value(false), "do not skip any checks that can be skipped while replaying irreversible blocks")
        ("disable-replay-opts", bpo::bool_switch()->default_value(false), "disable optimizations that specifically target replay")
        ("loadtest-mode", bpo::bool_switch()->default_value(false), "special for load-testing, skip expiration and reference block checks")
        ("charge-free-mode", bpo::bool_switch()->default_value(false), "do not charge any fees for transactions")
        ("replay-blockchain", bpo::bool_switch()->default_value(false), "clear chain state database and token database and replay all blocks")
        ("hard-replay-blockchain", bpo::bool_switch()->default_value(false), "clear chain state database and token database, recover as many blocks as possible from the block log, and then replay those blocks")
        ("delete-all-blocks", bpo::bool_switch()->default_value(false), "clear chain state database, token database and block log")
        ("truncate-at-block", bpo::value<uint32_t>()->default_value(0), "stop hard replay / block log recovery at this block number (if set to non-zero number)")
        ("import-reversible-blocks", bpo::value<bfs::path>(), "replace reversible block database with blocks imported from specified file and then exit")
        ("export-reversible-blocks", bpo::value<bfs::path>(), "export reversible block database in portable format into specified file and then exit")
        ("trusted-producer", bpo::value<vector<string>>()->composing(), "Indicate a producer whose blocks headers signed by it will be fully validated, but transactions in those validated blocks will be trusted.")
        ("snapshot", bpo::value<bfs::path>(), "File to read Snapshot State from")
        ;
}

#define LOAD_VALUE_SET(options, name, container) \
    if(options.count(name)) { \
        const std::vector<std::string>& ops = options[name].as<std::vector<std::string>>(); \
        std::copy(ops.begin(), ops.end(), std::inserter(container, container.end())); \
    }


fc::time_point
calculate_genesis_timestamp(string tstr) {
    fc::time_point genesis_timestamp;
    if(strcasecmp(tstr.c_str(), "now") == 0) {
        genesis_timestamp = fc::time_point::now();
    }
    else {
        genesis_timestamp = time_point::from_iso_string(tstr);
    }

    auto epoch_us = genesis_timestamp.time_since_epoch().count();
    auto diff_us  = epoch_us % config::block_interval_us;
    if(diff_us > 0) {
        auto delay_us = (config::block_interval_us - diff_us);
        genesis_timestamp += fc::microseconds(delay_us);
        dlog("pausing ${us} microseconds to the next interval", ("us", delay_us));
    }

    ilog("Adjusting genesis timestamp to ${timestamp}", ("timestamp", genesis_timestamp));
    return genesis_timestamp;
}

void
clear_directory_contents(const fc::path& p) {
    using boost::filesystem::directory_iterator;

    if(!fc::is_directory(p)) {
        return;
    }

    for(directory_iterator enditr, itr{p}; itr != enditr; ++itr) {
        fc::remove_all(itr->path());
    }
}

void
chain_plugin::plugin_initialize(const variables_map& options) {
    ilog("initializing chain plugin");

    try {
        try {
            genesis_state gs;  // Check if EVT_ROOT_KEY is bad
        }
        catch(const fc::exception&) {
            elog("EVT_ROOT_KEY ('${root_key}') is invalid. Recompile with a valid public key.", ("root_key", genesis_state::evt_root_key));
            throw;
        }

        my->chain_config = controller::config();

        LOAD_VALUE_SET(options, "trusted-producer", my->chain_config->trusted_producers);

        if(options.count("blocks-dir")) {
            auto bld = options.at("blocks-dir").as<bfs::path>();
            if(bld.is_relative()) {
                my->blocks_dir = app().data_dir() / bld;
            }
            else {
                my->blocks_dir = bld;
            }
        }

        if(options.count("token-db-dir")) {
            auto tod = options.at("token-db-dir").as<bfs::path>();
            if(tod.is_relative()) {
                my->tokendb_dir = app().data_dir() / tod;
            }
            else {
                my->tokendb_dir = tod;
            }
        }

        if(options.count("checkpoint")) {
            auto cps = options.at("checkpoint").as<vector<string>>();
            my->loaded_checkpoints.reserve(cps.size());
            for(const auto& cp : cps) {
                auto item = fc::json::from_string(cp).as<std::pair<uint32_t, block_id_type>>();
                auto itr  = my->loaded_checkpoints.find(item.first);
                if(itr != my->loaded_checkpoints.end()) {
                    EVT_ASSERT(itr->second == item.second, plugin_config_exception,
                        "redefining existing checkpoint at block number ${num}: original: ${orig} new: ${new}",
                        ("num", item.first)("orig", itr->second)("new", item.second));
                }
                else {
                    my->loaded_checkpoints[item.first] = item.second;
                }
            }
        }

        if(options.count("abi-serializer-max-time-ms")) {
            my->chain_config->max_serialization_time = std::chrono::milliseconds(options.at("abi-serializer-max-time-ms").as<uint32_t>());
        }

        my->chain_config->blocks_dir = my->blocks_dir;
        my->chain_config->state_dir  = app().data_dir() / config::default_state_dir_name;
        my->chain_config->read_only  = my->readonly;

        my->chain_config->db_config.db_path = my->tokendb_dir;
        
        if(options.count("token-db-cache-size-mb")) {
            // simply alloc block cache and object cache 50% and 50%.
            auto sz = options.at("token-db-cache-size-mb").as<uint32_t>() / 2 * 1024 * 1024;
            my->chain_config->db_config.block_cache_size = sz;
            my->chain_config->db_config.object_cache_size = sz;
        }

        if(options.count("token-db-profile")) {
            my->chain_config->db_config.profile = options.at("token-db-profile").as<storage_profile>();
        }

        if(options.count("chain-state-db-size-mb")) {
            my->chain_config->state_size = options.at("chain-state-db-size-mb").as<uint64_t>() * 1024 * 1024;
        }

        if(options.count("chain-state-db-guard-size-mb")) {
            my->chain_config->state_guard_size = options.at("chain-state-db-guard-size-mb").as<uint64_t>() * 1024 * 1024;
        }

        if(options.count("reversible-blocks-db-size-mb")) {
            my->chain_config->reversible_cache_size = options.at("reversible-blocks-db-size-mb").as<uint64_t>() * 1024 * 1024;
        }

        if(options.count("reversible-blocks-db-guard-size-mb")) {
            my->chain_config->reversible_guard_size = options.at("reversible-blocks-db-guard-size-mb").as<uint64_t>() * 1024 * 1024;
        }

        my->chain_config->force_all_checks    = options.at("force-all-checks").as<bool>();
        my->chain_config->disable_replay_opts = options.at("disable-replay-opts").as<bool>();
        my->chain_config->loadtest_mode       = options.at("loadtest-mode").as<bool>();
        my->chain_config->charge_free_mode    = options.at("charge-free-mode").as<bool>();
        my->chain_config->contracts_console   = options.at("contracts-console").as<bool>();

        if(options.count("extract-genesis-json") || options.at("print-genesis-json").as<bool>()) {
            genesis_state gs;

            if(fc::exists(my->blocks_dir / "blocks.log")) {
                gs = block_log::extract_genesis_state(my->blocks_dir);
            }
            else {
                wlog("No blocks.log found at '${p}'. Using default genesis state.",
                     ("p", (my->blocks_dir / "blocks.log").generic_string()));
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

        if(options.count("export-reversible-blocks")) {
            auto p = options.at("export-reversible-blocks").as<bfs::path>();

            if(p.is_relative()) {
                p = bfs::current_path() / p;
            }

            if(export_reversible_blocks(my->chain_config->blocks_dir / config::reversible_blocks_dir_name, p))
                ilog("Saved all blocks from reversible block database into '${path}'", ("path", p.generic_string()));
            else
                ilog("Saved recovered blocks from reversible block database into '${path}'", ("path", p.generic_string()));

            EVT_THROW(node_management_success, "exported reversible blocks");
        }

        if(options.at("delete-all-blocks").as<bool>()) {
            ilog("Deleting state database and blocks");
            if(options.at("truncate-at-block").as<uint32_t>() > 0)
                wlog("The --truncate-at-block option does not make sense when deleting all blocks.");
            clear_directory_contents(my->chain_config->state_dir);
            fc::remove_all(my->tokendb_dir);
            fc::remove_all(my->blocks_dir);
        }
        else if(options.at("hard-replay-blockchain").as<bool>()) {
            ilog("Hard replay requested: deleting state database");
            clear_directory_contents(my->chain_config->state_dir);
            fc::remove_all(my->tokendb_dir);
            auto backup_dir = block_log::repair_log(my->blocks_dir, options.at("truncate-at-block").as<uint32_t>());
            if(fc::exists(backup_dir / config::reversible_blocks_dir_name) || options.at("fix-reversible-blocks").as<bool>()) {
                // Do not try to recover reversible blocks if the directory does not exist, unless the option was explicitly provided.
                if(!recover_reversible_blocks(backup_dir / config::reversible_blocks_dir_name,
                                              my->chain_config->reversible_cache_size,
                                              my->chain_config->blocks_dir / config::reversible_blocks_dir_name,
                                              options.at("truncate-at-block").as<uint32_t>())) {
                    ilog("Reversible blocks database was not corrupted. Copying from backup to blocks directory.");
                    fc::copy(backup_dir / config::reversible_blocks_dir_name,
                             my->chain_config->blocks_dir / config::reversible_blocks_dir_name);
                    fc::copy(backup_dir / config::reversible_blocks_dir_name / "shared_memory.bin",
                             my->chain_config->blocks_dir / config::reversible_blocks_dir_name / "shared_memory.bin");
                    fc::copy(backup_dir / config::reversible_blocks_dir_name / "shared_memory.meta",
                             my->chain_config->blocks_dir / config::reversible_blocks_dir_name / "shared_memory.meta");
                }
            }
        }
        else if(options.at("replay-blockchain").as<bool>()) {
            ilog("Replay requested: deleting state database");
            if(options.at("truncate-at-block").as<uint32_t>() > 0)
                wlog("The --truncate-at-block option does not work for a regular replay of the blockchain.");
            clear_directory_contents(my->chain_config->state_dir);
            fc::remove_all(my->tokendb_dir);
            if(options.at("fix-reversible-blocks").as<bool>()) {
                if(!recover_reversible_blocks(my->chain_config->blocks_dir / config::reversible_blocks_dir_name,
                                              my->chain_config->reversible_cache_size)) {
                    ilog("Reversible blocks database was not corrupted.");
                }
            }
        }
        else if(options.at("fix-reversible-blocks").as<bool>()) {
            if(!recover_reversible_blocks(my->chain_config->blocks_dir / config::reversible_blocks_dir_name,
                                          my->chain_config->reversible_cache_size,
                                          optional<fc::path>(),
                                          options.at("truncate-at-block").as<uint32_t>())) {
                ilog("Reversible blocks database verified to not be corrupted. Now exiting...");
            }
            else {
                ilog("Exiting after fixing reversible blocks database...");
            }
            EVT_THROW(fixed_reversible_db_exception, "fixed corrupted reversible blocks database");
        }
        else if(options.at("truncate-at-block").as<uint32_t>() > 0) {
            wlog("The --truncate-at-block option can only be used with --fix-reversible-blocks without a replay or with --hard-replay-blockchain.");
        }
        else if(options.count("import-reversible-blocks")) {
            auto reversible_blocks_file = options.at("import-reversible-blocks").as<bfs::path>();
            ilog("Importing reversible blocks from '${file}'", ("file", reversible_blocks_file.generic_string()));
            clear_directory_contents(my->chain_config->blocks_dir / config::reversible_blocks_dir_name);

            import_reversible_blocks(my->chain_config->blocks_dir / config::reversible_blocks_dir_name,
                                     my->chain_config->reversible_cache_size, reversible_blocks_file);

            EVT_THROW(node_management_success, "imported reversible blocks");
        }

        if(options.count("import-reversible-blocks")) {
            wlog("The --import-reversible-blocks option should be used by itself.");
        }

        if(options.count("snapshot")) {
            my->snapshot_path = options.at("snapshot").as<bfs::path>();
            EVT_ASSERT(fc::exists(*my->snapshot_path), plugin_config_exception,
                       "Cannot load snapshot, ${name} does not exist", ("name", my->snapshot_path->generic_string()));

            // recover genesis information from the snapshot
            auto infile = std::ifstream(my->snapshot_path->generic_string(), (std::ios::in | std::ios::binary));
            auto reader = std::make_shared<istream_snapshot_reader>(infile);
            reader->validate();
            reader->read_section<genesis_state>([this](auto& section) {
                section.read_row(my->chain_config->genesis);
            });
            infile.close();

            EVT_ASSERT(options.count("genesis-json") == 0 && options.count("genesis-timestamp") == 0,
                       plugin_config_exception,
                       "--snapshot is incompatible with --genesis-json and --genesis-timestamp as the snapshot contains genesis information");

            auto shared_mem_path = my->chain_config->state_dir / "shared_memory.bin";
            EVT_ASSERT(!fc::exists(shared_mem_path),
                       plugin_config_exception,
                       "Snapshot can only be used to initialize an empty database.");

            if(fc::is_regular_file(my->blocks_dir / "blocks.log")) {
                auto log_genesis = block_log::extract_genesis_state(my->blocks_dir);
                EVT_ASSERT(log_genesis.compute_chain_id() == my->chain_config->genesis.compute_chain_id(),
                           plugin_config_exception,
                           "Genesis information in blocks.log does not match genesis information in the snapshot");
            }
        }
        else {
            auto genesis_file                = bfs::path();
            bool genesis_timestamp_specified = false;
            auto existing_genesis            = std::optional<genesis_state>();

            if(fc::exists(my->blocks_dir / "blocks.log" )) {
                my->chain_config->genesis = block_log::extract_genesis_state( my->blocks_dir );
                existing_genesis = my->chain_config->genesis;
            }

            if(options.count("genesis-json")) {
                genesis_file = options.at( "genesis-json" ).as<bfs::path>();
                EVT_ASSERT(!fc::exists(my->blocks_dir / "blocks.log"),
                           plugin_config_exception,
                           "Genesis state can only be set on a fresh blockchain.");

                auto genesis_file = options.at("genesis-json").as<bfs::path>();
                if(genesis_file.is_relative()) {
                    genesis_file = bfs::current_path() / genesis_file;
                }

                EVT_ASSERT(fc::is_regular_file(genesis_file),
                           plugin_config_exception,
                           "Specified genesis file '${genesis}' does not exist.",
                           ("genesis", genesis_file.generic_string()));

                my->chain_config->genesis = fc::json::from_file(genesis_file).as<genesis_state>();
            }

            if(options.count("genesis-timestamp")) {
                my->chain_config->genesis.initial_timestamp = calculate_genesis_timestamp(
                    options.at("genesis-timestamp").as<string>());
                genesis_timestamp_specified = true;
            }

            if(!existing_genesis) {
                if(!genesis_file.empty()) {
                    if(genesis_timestamp_specified) {
                        ilog("Using genesis state provided in '${genesis}' but with adjusted genesis timestamp",
                            ("genesis", genesis_file.generic_string()));
                    }
                    else {
                        ilog("Using genesis state provided in '${genesis}'", ("genesis", genesis_file.generic_string()));
                    }
                    wlog("Starting up fresh blockchain with provided genesis state.");
                }
                else if(genesis_timestamp_specified) {
                    wlog("Starting up fresh blockchain with default genesis state but with adjusted genesis timestamp.");
                }
                else {
                    wlog("Starting up fresh blockchain with default genesis state.");
                }
            }
            else {
                EVT_ASSERT(my->chain_config->genesis == *existing_genesis, plugin_config_exception,
                           "Genesis state provided via command line arguments does not match the existing genesis state in blocks.log. "
                           "It is not necessary to provide genesis state arguments when a blocks.log file already exists."
                           );
            }

        }

        if(options.count("read-mode")) {
            my->chain_config->read_mode = options.at("read-mode").as<db_read_mode>();
            EVT_ASSERT(my->chain_config->read_mode != db_read_mode::IRREVERSIBLE, plugin_config_exception, "irreversible mode not currently supported.");
        }

        if(options.count("validation-mode")) {
            my->chain_config->block_validation_mode = options.at("validation-mode").as<validation_mode>();
        }

        my->chain.emplace(*my->chain_config);
        my->chain_id.emplace(my->chain->get_chain_id());

        // set up method providers
        my->get_block_by_number_provider = app().get_method<methods::get_block_by_number>().register_provider(
            [this](uint32_t block_num) -> signed_block_ptr {
                return my->chain->fetch_block_by_number(block_num);
            });

        my->get_block_by_id_provider = app().get_method<methods::get_block_by_id>().register_provider(
            [this](block_id_type id) -> signed_block_ptr {
                return my->chain->fetch_block_by_id(id);
            });

        my->get_head_block_id_provider = app().get_method<methods::get_head_block_id>().register_provider([this]() {
            return my->chain->head_block_id();
        });

        my->get_last_irreversible_block_number_provider = app().get_method<methods::get_last_irreversible_block_number>().register_provider(
            [this]() {
                return my->chain->last_irreversible_block_num();
            });

        // relay signals to channels
        my->pre_accepted_block_connection = my->chain->pre_accepted_block.connect([this](const signed_block_ptr& blk) {
            auto itr = my->loaded_checkpoints.find(blk->block_num());
            if(itr != my->loaded_checkpoints.end()) {
                auto id = blk->id();
                EVT_ASSERT(itr->second == id, checkpoint_exception,
                           "Checkpoint does not match for block number ${num}: expected: ${expected} actual: ${actual}",
                           ("num", blk->block_num())("expected", itr->second)("actual", id));
            }

            my->pre_accepted_block_channel.publish(priority::medium, blk);
        });

        my->accepted_block_header_connection = my->chain->accepted_block_header.connect(
            [this](const block_state_ptr& blk) {
                my->accepted_block_header_channel.publish(priority::medium, blk);
            });

        my->accepted_block_connection = my->chain->accepted_block.connect([this](const block_state_ptr& blk) {
            my->accepted_block_channel.publish(priority::high, blk);
        });

        my->irreversible_block_connection = my->chain->irreversible_block.connect([this](const block_state_ptr& blk) {
            my->irreversible_block_channel.publish(priority::low, blk);
        });

        my->accepted_transaction_connection = my->chain->accepted_transaction.connect(
            [this](const transaction_metadata_ptr& meta) {
                my->accepted_transaction_channel.publish(priority::low, meta);
            });

        my->applied_transaction_connection = my->chain->applied_transaction.connect(
            [this](const transaction_trace_ptr& trace) {
                my->applied_transaction_channel.publish(priority::low, trace);
            });

        my->chain->add_indices();
    }
    FC_LOG_AND_RETHROW()
}

void
chain_plugin::plugin_startup() {
    try {
        try {
            if(my->snapshot_path) {
                auto infile = std::ifstream(my->snapshot_path->generic_string(), (std::ios::in | std::ios::binary));
                auto reader = std::make_shared<istream_snapshot_reader>(infile);
                my->chain->startup(reader);
                infile.close();
            }
            else {
                my->chain->startup();
            }
        }
        catch(const database_guard_exception& e) {
            log_guard_exception(e);
            // make sure to properly close the db
            my->chain.reset();
            throw;
        }

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
    my->pre_accepted_block_connection.reset();
    my->accepted_block_header_connection.reset();
    my->accepted_block_connection.reset();
    my->irreversible_block_connection.reset();
    my->accepted_transaction_connection.reset();
    my->applied_transaction_connection.reset();
    my->chain.reset();
}

chain_apis::read_only
chain_plugin::get_read_only_api() const {
    return chain_apis::read_only(chain());
}

chain_apis::read_write
chain_plugin::get_read_write_api() {
    return chain_apis::read_write(chain());
}

void
chain_plugin::accept_block(const signed_block_ptr& block) {
    my->incoming_block_sync_method(block);
}

void
chain_plugin::accept_transaction(const chain::packed_transaction& trx, next_function<chain::transaction_trace_ptr> next) {
    my->incoming_transaction_async_method(std::make_shared<transaction_metadata>(std::make_shared<packed_transaction>(trx)), false, std::forward<decltype(next)>(next));
}

void
chain_plugin::accept_transaction(const chain::transaction_metadata_ptr& trx, next_function<chain::transaction_trace_ptr> next) {
    my->incoming_transaction_async_method(trx, false, std::forward<decltype(next)>(next));
}

bool
chain_plugin::block_is_on_preferred_chain(const block_id_type& block_id) {
    auto b = chain().fetch_block_by_number(block_header::num_from_id(block_id));
    return b && b->id() == block_id;
}

bool
chain_plugin::recover_reversible_blocks(const fc::path& db_dir, uint32_t cache_size,
                                        optional<fc::path> new_db_dir, uint32_t truncate_at_block) {
    try {
        chainbase::database reversible(db_dir, database::read_only);  // Test if dirty
        // If it reaches here, then the reversible database is not dirty

        if(truncate_at_block == 0)
            return false;

        reversible.add_index<reversible_block_index>();
        const auto& ubi = reversible.get_index<reversible_block_index, by_num>();

        auto itr = ubi.rbegin();
        if(itr != ubi.rend() && itr->blocknum <= truncate_at_block)
            return false;  // Because we are not going to be truncating the reversible database at all.
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

    auto now = fc::time_point::now();

    if(new_db_dir) {
        backup_dir     = reversible_dir;
        reversible_dir = *new_db_dir;
    }
    else {
        auto reversible_dir_name = reversible_dir.filename().generic_string();
        EVT_ASSERT(reversible_dir_name != ".", invalid_reversible_blocks_dir, "Invalid path to reversible directory");
        backup_dir = reversible_dir.parent_path() / reversible_dir_name.append("-").append(now);

        EVT_ASSERT(!fc::exists(backup_dir),
                   reversible_blocks_backup_dir_exist,
                   "Cannot move existing reversible directory to already existing directory '${backup_dir}'",
                   ("backup_dir", backup_dir));

        fc::rename(reversible_dir, backup_dir);
        ilog("Moved existing reversible directory to backup location: '${new_db_dir}'", ("new_db_dir", backup_dir));
    }

    fc::create_directories(reversible_dir);

    ilog("Reconstructing '${reversible_dir}' from backed up reversible directory", ("reversible_dir", reversible_dir));

    chainbase::database old_reversible(backup_dir, database::read_only, 0, true);
    chainbase::database new_reversible(reversible_dir, database::read_write, cache_size);
    std::fstream        reversible_blocks;
    reversible_blocks.open((reversible_dir.parent_path() / std::string("portable-reversible-blocks-").append(now)).generic_string().c_str(),
                           std::ios::out | std::ios::binary);

    uint32_t num   = 0;
    uint32_t start = 0;
    uint32_t end   = 0;
    old_reversible.add_index<reversible_block_index>();
    new_reversible.add_index<reversible_block_index>();
    const auto& ubi = old_reversible.get_index<reversible_block_index, by_num>();
    auto        itr = ubi.begin();
    if(itr != ubi.end()) {
        start = itr->blocknum;
        end   = start - 1;
    }
    if(truncate_at_block > 0 && start > truncate_at_block) {
        ilog("Did not recover any reversible blocks since the specified block number to stop at (${stop}) is less than first block in the reversible database (${start}).",
            ("stop", truncate_at_block)("start", start));
        return true;
    }
    try {
        for(; itr != ubi.end(); ++itr) {
            EVT_ASSERT(itr->blocknum == end + 1, gap_in_reversible_blocks_db,
                       "gap in reversible block database between ${end} and ${blocknum}",
                       ("end", end)("blocknum", itr->blocknum));
            reversible_blocks.write(itr->packedblock.data(), itr->packedblock.size());
            new_reversible.create<reversible_block_object>([&](auto& ubo) {
                ubo.blocknum = itr->blocknum;
                ubo.set_block(itr->get_block());  // get_block and set_block rather than copying the packed data acts as additional validation
            });
            end = itr->blocknum;
            ++num;
            if(end == truncate_at_block)
                break;
        }
    }
    catch(const gap_in_reversible_blocks_db& e) {
        wlog("${details}", ("details", e.to_detail_string()));
    }
    catch(...) {
    }

    if(end == truncate_at_block)
        ilog("Stopped recovery of reversible blocks early at specified block number: ${stop}", ("stop", truncate_at_block));

    if(num == 0)
        ilog("There were no recoverable blocks in the reversible block database");
    else if(num == 1)
        ilog("Recovered 1 block from reversible block database: block ${start}", ("start", start));
    else
        ilog("Recovered ${num} blocks from reversible block database: blocks ${start} to ${end}",
             ("num", num)("start", start)("end", end));

    return true;
}

bool
chain_plugin::import_reversible_blocks(const fc::path& reversible_dir,
                                       uint32_t        cache_size,
                                       const fc::path& reversible_blocks_file) {
    std::fstream        reversible_blocks;
    chainbase::database new_reversible(reversible_dir, database::read_write, cache_size);
    reversible_blocks.open(reversible_blocks_file.generic_string().c_str(), std::ios::in | std::ios::binary);

    reversible_blocks.seekg(0, std::ios::end);
    auto end_pos = reversible_blocks.tellg();
    reversible_blocks.seekg(0);

    uint32_t num   = 0;
    uint32_t start = 0;
    uint32_t end   = 0;
    new_reversible.add_index<reversible_block_index>();
    try {
        while(reversible_blocks.tellg() < end_pos) {
            signed_block tmp;
            fc::raw::unpack(reversible_blocks, tmp);
            num = tmp.block_num();

            if(start == 0) {
                start = num;
            }
            else {
                EVT_ASSERT(num == end + 1, gap_in_reversible_blocks_db,
                           "gap in reversible block database between ${end} and ${num}",
                           ("end", end)("num", num));
            }

            new_reversible.create<reversible_block_object>([&](auto& ubo) {
                ubo.blocknum = num;
                ubo.set_block(std::make_shared<signed_block>(std::move(tmp)));
            });
            end = num;
        }
    }
    catch(gap_in_reversible_blocks_db& e) {
        wlog("${details}", ("details", e.to_detail_string()));
        FC_RETHROW_EXCEPTION(e, warn, "rethrow");
    }
    catch(...) {
    }

    ilog("Imported blocks ${start} to ${end}", ("start", start)("end", end));

    if(num == 0 || end != num)
        return false;

    return true;
}

bool
chain_plugin::export_reversible_blocks(const fc::path& reversible_dir,
                                       const fc::path& reversible_blocks_file) {
    chainbase::database reversible(reversible_dir, database::read_only, 0, true);
    std::fstream        reversible_blocks;
    reversible_blocks.open(reversible_blocks_file.generic_string().c_str(), std::ios::out | std::ios::binary);

    uint32_t num   = 0;
    uint32_t start = 0;
    uint32_t end   = 0;
    reversible.add_index<reversible_block_index>();
    const auto& ubi = reversible.get_index<reversible_block_index, by_num>();
    auto        itr = ubi.begin();
    if(itr != ubi.end()) {
        start = itr->blocknum;
        end   = start - 1;
    }
    try {
        for(; itr != ubi.end(); ++itr) {
            EVT_ASSERT(itr->blocknum == end + 1, gap_in_reversible_blocks_db,
                       "gap in reversible block database between ${end} and ${blocknum}",
                       ("end", end)("blocknum", itr->blocknum));
            signed_block                tmp;
            fc::datastream<const char*> ds(itr->packedblock.data(), itr->packedblock.size());
            fc::raw::unpack(ds, tmp);  // Verify that packed block has not been corrupted.
            reversible_blocks.write(itr->packedblock.data(), itr->packedblock.size());
            end = itr->blocknum;
            ++num;
        }
    }
    catch(const gap_in_reversible_blocks_db& e) {
        wlog("${details}", ("details", e.to_detail_string()));
    }
    catch(...) {
    }

    if(num == 0) {
        ilog("There were no recoverable blocks in the reversible block database");
        return false;
    }
    else if(num == 1)
        ilog("Exported 1 block from reversible block database: block ${start}", ("start", start));
    else
        ilog("Exported ${num} blocks from reversible block database: blocks ${start} to ${end}",
             ("num", num)("start", start)("end", end));

    return (end >= start) && ((end - start + 1) == num);
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
    EVT_ASSERT(my->chain_id.has_value(), chain_id_type_exception, "Chain ID has not been initialized yet");
    return *my->chain_id;
}

void
chain_plugin::log_guard_exception(const chain::guard_exception& e) const {
    if(e.code() == chain::database_guard_exception::code_value) {
        elog("Database has reached an unsafe level of usage, shutting down to avoid corrupting the database.  "
             "Please increase the value set for \"chain-state-db-size-mb\" and restart the process!");
    }
    else if(e.code() == chain::reversible_guard_exception::code_value) {
        elog("Reversible block database has reached an unsafe level of usage, shutting down to avoid corrupting the database.  "
             "Please increase the value set for \"reversible-blocks-db-size-mb\" and restart the process!");
    }

    dlog("Details: ${details}", ("details", e.to_detail_string()));
}

void
chain_plugin::handle_guard_exception(const chain::guard_exception& e) const {
    log_guard_exception(e);

    // quit the app
    app().quit();
}

void
chain_plugin::handle_db_exhaustion() {
   elog("database memory exhausted: increase chain-state-db-size-mb and/or reversible-blocks-db-size-mb");
   //return 1 -- it's what programs/nodeos/main.cpp considers "BAD_ALLOC"
   std::_Exit(1);
}

namespace chain_apis {

std::vector<string>
get_enabled_plugins() {
    auto plugins = std::vector<string>();
    for(auto& it : app().get_plugins()) {
        if(it.second->get_state() == appbase::abstract_plugin::started) {
            plugins.emplace_back(it.first);
        }
    }
    return plugins;
}

read_only::get_info_results
read_only::get_info(const read_only::get_info_params&) const {
    auto itoh = [](uint32_t n, size_t hlen = sizeof(uint32_t) << 1) {
        static const char* digits = "0123456789abcdef";
        
        auto r = std::string(hlen, '0');
        for(size_t i = 0, j = (hlen - 1) * 4; i < hlen; ++i, j -= 4) {
            r[i] = digits[(n >> j) & 0x0f];
        }
        return r;
    };

    return {
        itoh(static_cast<uint32_t>(app().version())),
        db.get_chain_id(),
        contracts::evt_contract_abi_version(),
        db.fork_db_head_block_num(),
        db.last_irreversible_block_num(),
        db.last_irreversible_block_id(),
        db.fork_db_head_block_id(),
        db.fork_db_head_block_time(),
        db.fork_db_head_block_producer(),
        get_enabled_plugins(),
        app().version_string()
    };
}

fc::variant
read_only::get_block(const read_only::get_block_params& params) const {
    auto block = signed_block_ptr();
    EVT_ASSERT(!params.block_num_or_id.empty() && params.block_num_or_id.size() <= 64,
        chain::block_id_type_exception, "Invalid Block number or ID, must be greater than 0 and less than 64 characters");
    try {
        if(params.block_num_or_id.size() == 64) {
            block = db.fetch_block_by_id(fc::variant(params.block_num_or_id).as<block_id_type>());
        }
        else {
            block = db.fetch_block_by_number(fc::to_uint64(params.block_num_or_id));
        }
    }
    EVT_RETHROW_EXCEPTIONS(chain::block_id_type_exception, "Invalid block ID: ${block_num_or_id}", ("block_num_or_id", params.block_num_or_id))

    EVT_ASSERT(block, unknown_block_exception, "Could not find block: ${block}", ("block", params.block_num_or_id));

    auto pretty_output = fc::variant();
    db.get_abi_serializer().to_variant(*block, pretty_output, db.get_execution_context());

    uint32_t ref_block_prefix = block->id()._hash[1];

    return fc::mutable_variant_object(pretty_output.get_object())("id", block->id())("block_num", block->block_num())("ref_block_prefix", ref_block_prefix);
}

fc::variant
read_only::get_block_header_state(const get_block_header_state_params& params) const {
    block_state_ptr    b;
    optional<uint64_t> block_num;
    std::exception_ptr e;

    try {
        block_num = fc::to_uint64(params.block_num_or_id);
    }
    catch(...) {
    }

    if(block_num.has_value()) {
        b = db.fetch_block_state_by_number(*block_num);
    }
    else {
        try {
            b = db.fetch_block_state_by_id(fc::variant(params.block_num_or_id).as<block_id_type>());
        }
        EVT_RETHROW_EXCEPTIONS(chain::block_id_type_exception, "Invalid block ID: ${block_num_or_id}", ("block_num_or_id", params.block_num_or_id))
    }

    EVT_ASSERT(b, unknown_block_exception, "Could not find reversible block: ${block}", ("block", params.block_num_or_id));

    fc::variant vo;
    fc::to_variant(static_cast<const block_header_state&>(*b), vo);
    return vo;
}

fc::variant
read_only::get_head_block_header_state(const get_head_block_header_state_params& params) const {
    auto b = db.head_block_state();
    EVT_ASSERT(b, unknown_block_exception, "Could not find head block");

    fc::variant vo;
    fc::to_variant(static_cast<const block_header_state&>(*b), vo);
    return vo;
}

fc::variant
read_only::get_transaction(const get_transaction_params& params) {
    auto block_num = 0;
    if(!params.block_num.has_value()) {
        block_num = db.get_block_num_for_trx_id(params.id);
    }
    else {
        block_num = *params.block_num;
    }
    auto block = db.fetch_block_by_number(block_num);
    EVT_ASSERT(block, unknown_block_exception, "Could not find head block");

    for(auto& tx : block->transactions) {
        if(tx.trx.id() == params.id) {
            auto var = fc::variant();
            if(params.raw.has_value() && *params.raw) {
                fc::to_variant(tx.trx, var);
            }
            else {
                db.get_abi_serializer().to_variant(tx.trx, var, db.get_execution_context());
            }

            auto mv = fc::mutable_variant_object(var);
            mv["block_num"] = block_num;
            mv["block_id"]  = block->id();

            return mv;
        }
    }
    EVT_THROW(unknown_transaction_exception, "Cannot find transaction");
}

fc::variant
read_only::get_trx_id_for_link_id(const get_trx_id_for_link_id_params& params) const {
    if(params.link_id.size() != sizeof(link_id_type)) {
        EVT_THROW(evt_link_id_exception, "EVT-Link id is not in proper length");
    }

    auto link_id = link_id_type();
    memcpy(&link_id, params.link_id.data(), sizeof(link_id));

    auto obj        = db.get_link_obj_for_link_id(link_id);
    auto vo         = fc::mutable_variant_object();
    vo["block_num"] = obj.block_num;
    vo["trx_id"]    = obj.trx_id;

    return vo;
}

void
read_write::push_block(read_write::push_block_params&& params, next_function<read_write::push_block_results> next) {
    try {
        app().get_method<incoming::methods::block_sync>()(std::make_shared<signed_block>(std::move(params)));
        next(read_write::push_block_results{});
    }
    catch(boost::interprocess::bad_alloc&) {
        chain_plugin::handle_db_exhaustion();
    }
    catch(fc::unrecoverable_exception&) {
        raise(SIGUSR1);
    }
    CATCH_AND_CALL(next);
}

void
read_write::push_transaction(const read_write::push_transaction_params& params, next_function<read_write::push_transaction_results> next) {
    try {
        auto  ptrx     = std::make_shared<packed_transaction>();
        auto& exec_ctx = db.get_execution_context();
        auto  trx_meta = transaction_metadata_ptr();
        try {
            db.get_abi_serializer().from_variant(params, *ptrx, exec_ctx);
            trx_meta = std::make_shared<transaction_metadata>(ptrx);
        }
        EVT_RETHROW_EXCEPTIONS(chain::packed_transaction_type_exception, "Invalid packed transaction")

        app().get_method<incoming::methods::transaction_async>()(trx_meta, true, [this, next, &exec_ctx](const fc::static_variant<fc::exception_ptr, transaction_trace_ptr>& result) -> void {
            if(result.contains<fc::exception_ptr>()) {
                next(result.get<fc::exception_ptr>());
            }
            else {
                auto trx_trace_ptr = result.get<transaction_trace_ptr>();

                try {
                    auto pretty_output = fc::variant();
                    db.get_abi_serializer().to_variant(*trx_trace_ptr, pretty_output, exec_ctx);

                    auto& id = trx_trace_ptr->id;
                    next(read_write::push_transaction_results{id, pretty_output});
                }
                CATCH_AND_CALL(next);
            }
        });
    }
    catch(boost::interprocess::bad_alloc&) {
        chain_plugin::handle_db_exhaustion();
    }
    catch(fc::unrecoverable_exception&) {
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

        auto next_index = index + 1;
        if((size_t)next_index < params->size()) {
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
action_abi_to_variant(const abi_serializer& abi, contracts::type_name action_type) {
    auto v = fc::variant();
    if(abi.is_struct(action_type)) {
        to_variant(abi.get_struct(action_type), v);
    }
    return v;
};

read_only::abi_json_to_bin_result
read_only::abi_json_to_bin(const read_only::abi_json_to_bin_params& params) const try {
    auto& abi      = db.get_abi_serializer();
    auto& exec_ctx = db.get_execution_context();

    auto result      = abi_json_to_bin_result();
    auto action_type = exec_ctx.get_acttype_name(params.action);

    try {
        result.binargs = abi.variant_to_binary(action_type, params.args, exec_ctx, shorten_abi_errors);
    }
    EVT_RETHROW_EXCEPTIONS(chain::action_args_exception,
                           "'${args}' is invalid args for action '${action}'. expected '${proto}'",
                           ("args", params.args)("action", params.action)("proto", action_abi_to_variant(abi, action_type)))
    return result;
}
FC_CAPTURE_AND_RETHROW((params.action)(params.args))

read_only::abi_bin_to_json_result
read_only::abi_bin_to_json(const read_only::abi_bin_to_json_params& params) const {
    auto& abi      = db.get_abi_serializer();
    auto& exec_ctx = db.get_execution_context();

    auto result      = abi_bin_to_json_result();
    auto action_type = exec_ctx.get_acttype_name(params.action);

    result.args = abi.binary_to_variant(action_type, params.binargs, exec_ctx, shorten_abi_errors);
    return result;
}

read_only::trx_json_to_digest_result
read_only::trx_json_to_digest(const trx_json_to_digest_params& params) const {
    auto result = trx_json_to_digest_result();
    try {
        auto trx = std::make_shared<transaction>();
        try {
            db.get_abi_serializer().from_variant(params, *trx, db.get_execution_context());
        }
        EVT_RETHROW_EXCEPTIONS(chain::packed_transaction_type_exception, "Invalid transaction");

        result.digest = trx->sig_digest(db.get_chain_id());
        result.id     = trx->id();
    }
    catch(boost::interprocess::bad_alloc&) {
        chain_plugin::handle_db_exhaustion();
    }
    catch(fc::unrecoverable_exception&) {
        raise(SIGUSR1);
    }
    catch(...) {
        throw;
    }
    return result;
}

read_only::get_required_keys_result
read_only::get_required_keys(const get_required_keys_params& params) const {
    auto trx = transaction();
    try {
        db.get_abi_serializer().from_variant(params.transaction, trx, db.get_execution_context());
    }
    EVT_RETHROW_EXCEPTIONS(chain::transaction_type_exception, "Invalid transaction");

    auto result          = get_required_keys_result();
    result.required_keys = db.get_required_keys(trx, params.available_keys);

    return result;
}

read_only::get_suspend_required_keys_result
read_only::get_suspend_required_keys(const get_suspend_required_keys_params& params) const {
    auto result          = get_suspend_required_keys_result();
    result.required_keys = db.get_suspend_required_keys(params.name, params.available_keys);
    return result;
}

read_only::get_charge_result
read_only::get_charge(const get_charge_params& params) const {
    auto trx = transaction();
    try {
        db.get_abi_serializer().from_variant(params.transaction, trx, db.get_execution_context());
    }
    EVT_RETHROW_EXCEPTIONS(chain::transaction_type_exception, "Invalid transaction");

    auto result   = get_charge_result();
    result.charge = db.get_charge(std::move(trx), params.sigs_num);

    return result;
}

fc::variant
read_only::get_transaction_ids_for_block(const get_transaction_ids_for_block_params& params) const {
    auto block = signed_block_ptr();
    try {
        block = db.fetch_block_by_id(params.block_id);
    }
    EVT_RETHROW_EXCEPTIONS(chain::block_id_type_exception, "Invalid block ID: ${id}", ("id", params.block_id))

    EVT_ASSERT(block, unknown_block_exception, "Could not find block: ${block}", ("id", params.block_id));

    auto arr = fc::variants();
    for(auto& trx : block->transactions) {
        arr.emplace_back(trx.trx.id());
    }

    return arr;
}

const std::string&
read_only::get_abi(const get_abi_params&) const {
    static std::string _abi_json;
    
    if(_abi_json.empty()) {
        auto abi = evt::chain::contracts::evt_contract_abi();
        auto ver = evt::chain::contracts::evt_contract_abi_version();

        auto var = fc::variant();
        fc::to_variant(abi, var);

        auto varobj = fc::mutable_variant_object(var);
        varobj["version"] = ver;

        _abi_json = fc::json::to_string(varobj);
    }

    return _abi_json;
}

const std::string&
read_only::get_actions(const get_actions_params&) const {
    static std::string                  _actions_json;
    static std::vector<action_ver_type> _actions;

    auto acts = db.get_execution_context().get_current_actions();
    if(_actions.empty()) {
        _actions      = std::move(acts);
        _actions_json = fc::json::to_string(_actions);

        return _actions_json;
    }
    else {
        FC_ASSERT(_actions.size() == acts.size());
        
        for(auto i = 0u; i < acts.size(); i++) {
            if(acts[i].ver != _actions[i].ver) {
                _actions      = std::move(acts);
                _actions_json = fc::json::to_string(_actions);

                break;
            }
        }

        return _actions_json;
    }
}

std::string
read_only::get_db_info(const get_db_info_params&) const {
    return db.token_db().stats();
}

}  // namespace chain_apis
}  // namespace evt
