/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */

#pragma once

#include <jmzk/chain_plugin/chain_plugin.hpp>
#include <jmzk/http_client_plugin/http_client_plugin.hpp>
#include <appbase/application.hpp>

namespace jmzk {

using boost::signals2::signal;

class producer_plugin : public appbase::plugin<producer_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin)(http_client_plugin))

    struct runtime_options {
        optional<int32_t> max_transaction_time;
        optional<int32_t> max_irreversible_block_age;
        optional<int32_t> produce_time_offset_us;
        optional<int32_t> last_block_time_offset_us;
    };

    struct integrity_hash_information {
        uint32_t             head_block_num;
        chain::block_id_type head_block_id;
        fc::time_point       head_block_time;
        chain::digest_type   integrity_hash;
    };

    struct snapshot_information {
        uint32_t             head_block_num;
        chain::block_id_type head_block_id;
        fc::time_point       head_block_time;
        std::string          snapshot_name;
        size_t               snapshot_size;
        bool                 postgres;
    };

    struct create_snapshot_options {
        bool postgres = false;
    };

    producer_plugin();
    virtual ~producer_plugin();

    virtual void set_program_options(boost::program_options::options_description& command_line_options,
                                     boost::program_options::options_description& config_file_options) override;

    bool                  is_producer_key(const chain::public_key_type& key) const;
    chain::signature_type sign_compact(const chain::public_key_type& key, const fc::sha256& digest) const;

    virtual void plugin_initialize(const boost::program_options::variables_map& options);
    virtual void plugin_startup();
    virtual void plugin_shutdown();
    void handle_sighup() override;
 
    void pause();
    void resume();
    bool paused() const;
    void update_runtime_options(const runtime_options& options);
    runtime_options get_runtime_options() const;

    integrity_hash_information get_integrity_hash() const;
    snapshot_information create_snapshot(const create_snapshot_options& options) const;

    signal<void(const chain::producer_confirmation&)> confirmed_block;

private:
    std::shared_ptr<class producer_plugin_impl> my;
};

}  // namespace jmzk

FC_REFLECT(jmzk::producer_plugin::runtime_options, (max_transaction_time)(max_irreversible_block_age)(produce_time_offset_us)(last_block_time_offset_us));
FC_REFLECT(jmzk::producer_plugin::integrity_hash_information, (head_block_num)(head_block_id)(head_block_time)(integrity_hash));
FC_REFLECT(jmzk::producer_plugin::snapshot_information, (head_block_num)(head_block_id)(head_block_time)(snapshot_name)(snapshot_size)(postgres));
FC_REFLECT(jmzk::producer_plugin::create_snapshot_options, (postgres));
