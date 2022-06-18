/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once
#include <memory>

#include <appbase/application.hpp>
#include <jmzk/chain_plugin/chain_plugin.hpp>

namespace jmzk {

namespace chain {
class snapshot_writer;
class snapshot_reader;
}  // namespace chain

using jmzk::chain::public_key_type;

class postgres_plugin : public plugin<postgres_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin))

    postgres_plugin();
    virtual ~postgres_plugin();

    virtual void set_program_options(options_description& cli, options_description& cfg) override;

    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

public:
    bool enabled() const;
    const std::string& connstr() const;

public:
    void read_from_snapshot(const std::shared_ptr<chain::snapshot_reader>& snapshot);
    void write_snapshot(const std::shared_ptr<chain::snapshot_writer>& snapshot) const;

private:
    std::unique_ptr<class postgres_plugin_impl> my_;
};

}  // namespace jmzk
