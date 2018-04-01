/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
*/

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include "eosio/chain/tokendb.hpp"

using namespace eosio::chain;
using namespace eosio::chain::contracts;

BOOST_AUTO_TEST_SUITE(tokendb_test)

BOOST_AUTO_TEST_CASE(tokendb_default_test)
{
    evt::chain::tokendb db;
    auto path = "/var/tmp/tokendb";
    if(boost::filesystem::exists(path)) {
        boost::filesystem::remove_all(path);
    }
    boost::filesystem::create_directory(path);
    auto r = db.initialize("/var/tmp/tokendb");
    BOOST_CHECK_EQUAL(0, r);

    BOOST_CHECK_EQUAL(false, db.exists_domain("test"));
    BOOST_CHECK_EQUAL(true, db.exists_domain("group"));
    
    user_id issuer = {};
    domain_def domain;
    domain.name = "test";
    domain.issuer = issuer;
    domain.issue_time = fc::time_point_sec(1024);
    
    permission_def issue_permission;
    issue_permission.name = "issue";
    issue_permission.threshold = 2;
    issue_permission.groups.push_back(group_weight(0, 2));
    issue_permission.groups.push_back(group_weight(100001, 2));
    domain.issue = issue_permission;

    auto r2 = db.add_domain(domain);
    BOOST_CHECK_EQUAL(0, r2);
    BOOST_CHECK_EQUAL(true, db.exists_domain("test"));

    issuetoken issuetoken;
    issuetoken.domain = "test";
    issuetoken.owner = { issuer };
    issuetoken.names = { "TEST-A", "TEST-B", "TEST-C" };
    auto r3 = db.issue_tokens(issuetoken);
    BOOST_CHECK_EQUAL(0, r3);

    BOOST_CHECK_EQUAL(true, db.exists_token("test", "TEST-A"));
    BOOST_CHECK_EQUAL(false, db.exists_token("test", "TEST-D"));

    auto gkey = group_key();
    group_def group {};
    group.id = 100001;
    group.key = gkey;
    group.threshold = 20;
    group.keys.emplace_back(key_weight { .key = gkey, .weight = 10 });
    group.keys.emplace_back(key_weight { .key = gkey, .weight = 20 });

    BOOST_CHECK_EQUAL(false, db.exists_group(group.id));
    auto r4 = db.add_group(group);
    BOOST_CHECK_EQUAL(0, r4);
    BOOST_CHECK_EQUAL(true, db.exists_group(group.id));

    db.read_domain("test", [&domain](const auto& v) {
        BOOST_CHECK_EQUAL(true, "test" == v.name);
        BOOST_CHECK_EQUAL(true, domain.issuer == v.issuer);
        BOOST_CHECK_EQUAL(true, domain.issue_time == v.issue_time);
        BOOST_CHECK_EQUAL(domain.issue.threshold, v.issue.threshold);
        for(uint i = 0; i < domain.issue.groups.size(); i++) {
            auto& lgroup = domain.issue.groups[i];
            auto& rgroup = v.issue.groups[i];
            BOOST_CHECK_EQUAL(true, lgroup.id == rgroup.id);
            BOOST_CHECK_EQUAL(lgroup.weight, rgroup.weight);
        }
    });

    db.read_group(100001, [](const auto& g) {
        BOOST_CHECK_EQUAL(20, g.threshold);
        BOOST_CHECK_EQUAL(2, g.keys.size());
    });

    auto ug = updategroup();
    ug.id = 100001;
    ug.threshold = 40;
    ug.keys = {};
    db.update_group(ug);

    db.read_group(100001, [](const auto& g) {
        BOOST_CHECK_EQUAL(40, g.threshold);
        BOOST_CHECK_EQUAL(0, g.keys.size());
    });

    db.read_token("test", "TEST-A", [&](const auto& t) {
        BOOST_CHECK_EQUAL(true, t.owner[0] == issuer);
    });

    auto nuser = user_id(std::string("EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"));
    auto tt = transfertoken();
    tt.domain = "test";
    tt.name = "TEST-A";
    tt.to.push_back(nuser);

    db.transfer_token(tt);
    db.read_token("test", "TEST-A", [&](const auto& t) {
        BOOST_CHECK_EQUAL(true, t.owner[0] == nuser);
    });
}

BOOST_AUTO_TEST_CASE(tokendb_savepoint_test)
{
    evt::chain::tokendb db;
    auto path = "/var/tmp/tokendb";
    if(boost::filesystem::exists(path)) {
        boost::filesystem::remove_all(path);
    }
    boost::filesystem::create_directory(path);
    auto r = db.initialize("/var/tmp/tokendb");
    BOOST_CHECK_EQUAL(0, r);

    // initial
    user_id issuer = {};
    domain_def domain;
    domain.name = "test";
    domain.issuer = issuer;
    domain.issue_time = fc::time_point_sec(1024);
    
    permission_def issue_permission;
    issue_permission.name = "issue";
    issue_permission.threshold = 2;
    issue_permission.groups.push_back(group_weight(0, 2));
    issue_permission.groups.push_back(group_weight(100001, 2));
    domain.issue = issue_permission;

    auto r2 = db.add_domain(domain);
    BOOST_CHECK_EQUAL(0, r2);
    BOOST_CHECK_EQUAL(true, db.exists_domain("test"));

    issuetoken issuetoken;
    issuetoken.domain = "test";
    issuetoken.owner = { issuer };
    issuetoken.names = { "TEST-A", "TEST-B", "TEST-C" };
    auto r3 = db.issue_tokens(issuetoken);
    BOOST_CHECK_EQUAL(0, r3);

    BOOST_CHECK_EQUAL(true, db.exists_token("test", "TEST-A"));
    BOOST_CHECK_EQUAL(true, db.exists_token("test", "TEST-B"));
    BOOST_CHECK_EQUAL(true, db.exists_token("test", "TEST-C"));

    // set savepoint 1
    auto r4 = db.add_savepoint(1);
    BOOST_CHECK_EQUAL(0, r4);

    domain.name = "test2";
    auto r5 = db.add_domain(domain);
    BOOST_CHECK_EQUAL(0, r5);

    issuetoken.domain = "test2";
    auto r6 = db.issue_tokens(issuetoken);
    BOOST_CHECK_EQUAL(0, r6);

    // set savepoint 2
    auto r7 = db.add_savepoint(2);
    BOOST_CHECK_EQUAL(0, r7);

    auto nuser = user_id(std::string("EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV"));
    auto tt = transfertoken();
    tt.domain = "test2";
    tt.name = "TEST-A";
    tt.to.push_back(nuser);

    auto r8 = db.transfer_token(tt);
    BOOST_CHECK_EQUAL(0, r8);

    db.read_token("test2", "TEST-A", [&](const auto& t) {
        BOOST_CHECK_EQUAL(true, t.owner[0] == nuser);
    });

    domain.name = "test3";
    auto r9 = db.add_domain(domain);
    BOOST_CHECK_EQUAL(0, r9);
    BOOST_CHECK_EQUAL(true, db.exists_domain("test3"));

    db.read_domain("test3", [](const auto& d) {
        BOOST_CHECK_EQUAL(2, d.issue.threshold);
    });

    updatedomain ud;
    ud.name = "test3";
    ud.issue = issue_permission;
    ud.issue.threshold = 20;
    auto r13 = db.update_domain(ud);
    BOOST_CHECK_EQUAL(0, r13);
    db.read_domain("test3", [](const auto& d) {
        BOOST_CHECK_EQUAL(20, d.issue.threshold);
    });

    issuetoken.domain = "test";
    issuetoken.names = { "TEST-D", "TEST-E" };
    auto r14 = db.issue_tokens(issuetoken);
    BOOST_CHECK_EQUAL(0, r14);
    BOOST_CHECK_EQUAL(true, db.exists_token("test", "TEST-D"));
    BOOST_CHECK_EQUAL(true, db.exists_token("test", "TEST-E"));

    // revert to savepoint 2
    auto r10 = db.rollback_to_latest_savepoint();
    BOOST_CHECK_EQUAL(0, r10);

    BOOST_CHECK_EQUAL(false, db.exists_domain("test3"));
    db.read_token("test2", "TEST-A", [&](const auto& t) {
        BOOST_CHECK_EQUAL(true, t.owner[0] == issuer);
        BOOST_CHECK_EQUAL(false, t.owner[0] == nuser);
    });
    
    BOOST_CHECK_EQUAL(false, db.exists_token("test", "TEST-D"));
    BOOST_CHECK_EQUAL(false, db.exists_token("test", "TEST-E"));

    // pop all before savepoint 2
    auto r11 = db.pop_savepoints(2);
    BOOST_CHECK_EQUAL(0, r11);
    auto r12 = db.rollback_to_latest_savepoint();
    BOOST_CHECK_NE(0, r12);
}

BOOST_AUTO_TEST_SUITE_END()
