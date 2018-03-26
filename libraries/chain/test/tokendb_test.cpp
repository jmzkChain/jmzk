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
    evt::chain::token_db db;
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

    auto r2 = db.add_new_domain(domain);
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

    db.read_domain("test", [&domain](auto& v) {
        BOOST_CHECK_EQUAL(true, "test" == v.name);
        BOOST_CHECK_EQUAL(true, domain.issuer == v.issuer);
        BOOST_CHECK_EQUAL(true, domain.issue_time == v.issue_time);
        BOOST_CHECK_EQUAL(domain.issue.threshold, v.issue.threshold);
        for(int i = 0; i < domain.issue.groups.size(); i++) {
            auto& lgroup = domain.issue.groups[i];
            auto& rgroup = v.issue.groups[i];
            BOOST_CHECK_EQUAL(true, lgroup.id == rgroup.id);
            BOOST_CHECK_EQUAL(lgroup.weight, rgroup.weight);
        }
    });


}

BOOST_AUTO_TEST_SUITE_END()
