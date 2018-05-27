/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <appbase/application.hpp>

#include <evt/chain_api_plugin/chain_api_plugin.hpp>
#include <evt/chain_plugin/chain_plugin.hpp>
#include <evt/evt_api_plugin/evt_api_plugin.hpp>
#include <evt/evt_plugin/evt_plugin.hpp>
#include <evt/http_plugin/http_plugin.hpp>
#include <evt/net_plugin/net_plugin.hpp>
#include <evt/producer_plugin/producer_plugin.hpp>
#include <evt/utilities/common.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/appender.hpp>
#include <fc/log/logger_config.hpp>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "config.hpp"

using namespace appbase;
using namespace evt;

namespace fc {
std::unordered_map<std::string, appender::ptr>& get_appender_map();
}

namespace detail {

void
configure_logging(const bfs::path& config_path) {
    try {
        try {
            fc::configure_logging(config_path);
        }
        catch(...) {
            elog("Error reloading logging.json");
            throw;
        }
    }
    catch(const fc::exception& e) {
        elog("${e}", ("e", e.to_detail_string()));
    }
    catch(const boost::exception& e) {
        elog("${e}", ("e", boost::diagnostic_information(e)));
    }
    catch(const std::exception& e) {
        elog("${e}", ("e", e.what()));
    }
    catch(...) {
        // empty
    }
}

}  // namespace detail

void
logging_conf_loop() {
    std::shared_ptr<boost::asio::signal_set> sighup_set(new boost::asio::signal_set(app().get_io_service(), SIGHUP));
    sighup_set->async_wait([sighup_set](const boost::system::error_code& err, int /*num*/) {
        if(!err) {
            ilog("Received HUP.  Reloading logging configuration.");
            auto config_path = app().get_logging_conf();
            if(fc::exists(config_path))
                ::detail::configure_logging(config_path);
            for(auto iter : fc::get_appender_map())
                iter.second->initialize(app().get_io_service());
            logging_conf_loop();
        }
    });
}

void
initialize_logging() {
    auto config_path = app().get_logging_conf();
    if(fc::exists(config_path))
        fc::configure_logging(config_path);  // intentionally allowing exceptions to escape
    for(auto iter : fc::get_appender_map())
        iter.second->initialize(app().get_io_service());

    logging_conf_loop();
}

enum return_codes {
    OTHER_FAIL        = -2,
    INITIALIZE_FAIL   = -1,
    SUCCESS           = 0,
    BAD_ALLOC         = 1,
    DATABASE_DIRTY    = 2,
    FIXED_REVERSIBLE  = 3,
    EXTRACTED_GENESIS = 4
};

int
main(int argc, char** argv) {
    try {
        app().set_version(evt::evtd::config::version);

        auto root = fc::app_path();
        app().set_default_data_dir(root / "evt/evtd/data");
        app().set_default_config_dir(root / "evt/evtd/config");
        if(!app().initialize<chain_plugin, chain_api_plugin, http_plugin, net_plugin, producer_plugin, evt_plugin, evt_api_plugin>(argc, argv))
            return INITIALIZE_FAIL;
        initialize_logging();
        ilog("evtd version ${ver}", ("ver", evt::utilities::common::itoh(static_cast<uint32_t>(app().version()))));
        ilog("evt root is ${root}", ("root", root.string()));
        app().startup();
        app().exec();
    }
    catch( const extract_genesis_state_exception& e ) {
        return EXTRACTED_GENESIS;
    }
    catch(const fixed_reversible_db_exception& e) {
        return FIXED_REVERSIBLE;
    }
    catch(const fc::exception& e) {
        elog("${e}", ("e", e.to_detail_string()));
        return OTHER_FAIL;
    }
    catch(const boost::interprocess::bad_alloc& e) {
        elog("bad alloc");
        return BAD_ALLOC;
    }
    catch(const boost::exception& e) {
        elog("${e}", ("e", boost::diagnostic_information(e)));
        return OTHER_FAIL;
    }
    catch(const std::runtime_error& e) {
        if(std::string(e.what()) == "database dirty flag set") {
            elog("database dirty flag set (likely due to unclean shutdown): replay required");
            return DATABASE_DIRTY;
        }
        else if(std::string(e.what()) == "database metadata dirty flag set") {
            elog("database metadata dirty flag set (likely due to unclean shutdown): replay required");
            return DATABASE_DIRTY;
        }
        else {
            elog("${e}", ("e", e.what()));
        }
        return OTHER_FAIL;
    }
    catch(const std::exception& e) {
        elog("${e}", ("e", e.what()));
        return OTHER_FAIL;
    }
    catch(...) {
        elog("unknown exception");
        return OTHER_FAIL;
    }

    return SUCCESS;
}
