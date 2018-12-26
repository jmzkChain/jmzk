/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <string>
#include <memory>
#include <optional>
#include <boost/scope_exit.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/bulk_write.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>
#include <appbase/application.hpp>

namespace evt {

using mongocxx::database;
using mongocxx::collection;
using mongocxx::bulk_write;

#define define_collection(n)                                        \
    collection                 n##_collection;                       \
    std::optional<bulk_write>  n##_commits;                          \
                                                                    \
    auto& get_##n() {                                               \
        if(!(n##_commits)) {                                        \
            n##_commits = n##_collection.create_bulk_write(opts_);  \
        }                                                           \
        total_++;                                                   \
        return *n##_commits;                                        \
    }

#define commit_collection(n)            \
    if(n##_commits.has_value()) {       \
        BOOST_SCOPE_EXIT_ALL(&) {       \
            n##_commits.reset();        \
        };                              \
        try {                           \
            n##_commits->execute();     \
        }                               \
        catch(...) {                    \
            handle_mongo_exception(#n); \
        }                               \
    }

class write_context {
public:
    write_context() {
        opts_.ordered(true);
    }

public:
    define_collection(blocks);
    define_collection(trxs);
    define_collection(actions);
    define_collection(domains);
    define_collection(tokens);
    define_collection(groups);
    define_collection(fungibles);

public:
    void
    execute() {
        commit_collection(blocks);
        commit_collection(trxs);
        commit_collection(actions);
        commit_collection(domains);
        commit_collection(tokens);
        commit_collection(groups);
        commit_collection(fungibles);

        total_ = 0;
    }

    size_t
    total() const {
        return total_;
    }

private:
    void
    handle_mongo_exception(std::string desc) {
        bool shutdown = true;
        try {
            try {
                wlog("exception from: ${desc}", ("desc",desc));
                throw;
            }
            catch(mongocxx::logic_error& e) {
                // logic_error on invalid key, do not shutdown
                wlog("mongo logic error, code ${code}, ${what}",
                     ("code", e.code().value())("what", e.what()));
                shutdown = false;
            }
            catch(mongocxx::operation_exception& e) {
                elog("mongo exception, code ${code}, ${details}",
                     ("code", e.code().value())("details", e.code().message()));
                if(e.raw_server_error()) {
                    elog("mongo exception, ${details}",
                         ("details", bsoncxx::to_json(e.raw_server_error()->view())));
                }
            }
            catch(mongocxx::exception& e) {
                elog("mongo exception, code ${code}, ${what}",
                     ("code", e.code().value())("what", e.what()));
            }
            catch(bsoncxx::exception& e) {
                elog("bsoncxx exception, code ${code}, ${what}",
                     ("code", e.code().value())("what", e.what()));
            }
            catch(fc::exception& er) {
                elog("mongo fc exception, ${details}",
                     ("details", er.to_detail_string()));
            }
            catch(const std::exception& e) {
                elog("mongo std exception, ${what}",
                     ("what", e.what()));
            }
            catch(...) {
                elog("mongo unknown exception");
            }
        }
        catch(...) {
            std::cerr << "Exception attempting to handle exception" << std::endl;
        }

        if(shutdown) {
            // shutdown if mongo failed to provide opportunity to fix issue and restart
            appbase::app().quit();
        }
    }

private:
    mongocxx::options::bulk_write opts_;
    size_t                        total_;
};

}  // namespace evt
