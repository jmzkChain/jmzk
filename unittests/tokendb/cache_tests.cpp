#include "tokendb_tests.hpp"
#include <evt/chain/token_database_cache.hpp>

TEST_CASE_METHOD(tokendb_test, "cache_test", "[tokendb]") {
    auto& tokendb = my_tester->control->token_db();
    auto  cache = token_database_cache(tokendb, 1024 * 1024);

    auto CHECK_EQUAL = [](auto& lhs, auto& rhs) {
        auto b1 = fc::raw::pack(lhs);
        auto b2 = fc::raw::pack(rhs);

        CHECK(b1.size() == b2.size());
        CHECK(memcmp(b1.data(), b2.data(), b1.size()) == 0);
    };

    SECTION("read_test") {
        auto dom = domain_def();
        auto tk  = token_def();
        
        READ_TOKEN(domain, "dm-tkdb-test", dom);
        READ_TOKEN2(token, "dm-tkdb-test", "t1", tk);

        auto dom2 = cache.read_token<domain_def>(token_type::domain, std::nullopt, "dm-tkdb-test");
        auto tk2  = cache.read_token<token_def>(token_type::token, "dm-tkdb-test", "t1");

        CHECK(dom2 != nullptr);
        CHECK(tk2 != nullptr);

        CHECK_EQUAL(dom, *dom2);
        CHECK_EQUAL(tk, *tk2);

        CHECK_THROWS_AS(cache.read_token<domain_def>(token_type::domain, std::nullopt, "dm-tkdb-test123"), unknown_token_database_key);
        CHECK(cache.read_token<domain_def>(token_type::domain, std::nullopt, "dm-tkdb-test123", true) == nullptr);

        CHECK_THROWS_AS(cache.read_token<token_def>(token_type::domain, std::nullopt, "dm-tkdb-test"), token_database_cache_exception);
    }

    SECTION("write_test") {
        auto s = tokendb.new_savepoint_session();

        auto var = fc::json::from_string(domain_data);
        auto dom = var.as<domain_def>();

        cache.put_token(token_type::domain, action_op::put, std::nullopt, "dm-tkdb-cache", dom);
        CHECK(EXISTS_TOKEN(domain, "dm-tkdb-cache"));

        auto dom2 = cache.read_token<domain_def>(token_type::domain, std::nullopt, "dm-tkdb-cache");
        CHECK_EQUAL(dom, *dom2);

        s.accept();
    }

    SECTION("rollback_test") {
        auto s = tokendb.new_savepoint_session();

        auto var = fc::json::from_string(domain_data);
        auto dom = var.as<domain_def>();

        cache.put_token(token_type::domain, action_op::put, std::nullopt, "dm-tkdb-cache-2", dom);
        CHECK(cache.lookup_token<domain_def>(token_type::domain, std::nullopt, "dm-tkdb-cache-2") != nullptr);
        CHECK(EXISTS_TOKEN(domain, "dm-tkdb-cache-2"));

        {
            auto s2 = tokendb.new_savepoint_session();

            // update dom2
            auto dom2 = cache.lookup_token<domain_def>(token_type::domain, std::nullopt, "dm-tkdb-cache-2");
            CHECK(dom2 != nullptr);
            dom2->creator = public_key_type();

            // dom3 is not the same instance as dom2, cannot put again
            auto dom3 = var.as<domain_def>();
            CHECK_THROWS_AS(cache.put_token(token_type::domain, action_op::put, std::nullopt, "dm-tkdb-cache-2", dom3), token_database_cache_exception);
        
            // correct, can update dom2
            cache.put_token(token_type::domain, action_op::put, std::nullopt, "dm-tkdb-cache-2", *dom2);
            CHECK(cache.lookup_token<domain_def>(token_type::domain, std::nullopt, "dm-tkdb-cache-2")->creator == public_key_type());
        }
        // has rollback, cannot read cache
        CHECK(cache.lookup_token<domain_def>(token_type::domain, std::nullopt, "dm-tkdb-cache-2") == nullptr);

        // can read token from internal db with new value
        CHECK(cache.read_token<domain_def>(token_type::domain, std::nullopt, "dm-tkdb-cache-2") != nullptr);
        
        // undo the insert operation, cannot read in both cache and internal db
        s.undo();
        CHECK(cache.lookup_token<domain_def>(token_type::domain, std::nullopt, "dm-tkdb-cache-2") == nullptr);
        CHECK_THROWS_AS(cache.read_token<domain_def>(token_type::domain, std::nullopt, "dm-tkdb-cache-2") == nullptr, unknown_token_database_key);
    }
}
