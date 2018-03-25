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

    BOOST_CHECK_EQUAL(false, db.exists_token_type("test"));
    BOOST_CHECK_EQUAL(true, db.exists_token_type("group"));
    
    user_id issuer = {};
    token_type_def ttype;
    ttype.name = "test";
    ttype.issuer = issuer;
    ttype.issue_time = fc::time_point_sec(1024);
    
    permission_def issue_permission;
    issue_permission.name = "issue";
    issue_permission.threshold = 2;
    issue_permission.groups.push_back(group_weight(group_ref_type::owner, 2));
    issue_permission.groups.push_back(group_weight(group_ref_type::key, issuer, 2));
    ttype.issue = issue_permission;

    auto r2 = db.add_new_token_type(ttype);
    BOOST_CHECK_EQUAL(0, r2);
    BOOST_CHECK_EQUAL(true, db.exists_token_type("test"));

    issuetoken issuetoken;
    issuetoken.type = "test";
    issuetoken.owner = { issuer };
    issuetoken.ids = { 1001, 1002, 1003 };
    auto r3 = db.issue_tokens(issuetoken);
    BOOST_CHECK_EQUAL(0, r3);

    BOOST_CHECK_EQUAL(true, db.exists_token("test", 1001));
    BOOST_CHECK_EQUAL(false, db.exists_token("test", 1004));

    auto gkey = group_key();
    group_def group {};
    group.key = gkey;
    group.threshold = 20;
    group.keys.emplace_back(key_weight { .key = gkey, .weight = 10 });
    group.keys.emplace_back(key_weight { .key = gkey, .weight = 20 });

    BOOST_CHECK_EQUAL(false, db.exists_group(gkey));
    auto r4 = db.add_group(group);
    BOOST_CHECK_EQUAL(0, r4);
    BOOST_CHECK_EQUAL(true, db.exists_group(gkey));

    db.read_token_type("test", [&ttype](auto& v) {
        BOOST_CHECK_EQUAL(true, "test" == v.name);
        BOOST_CHECK_EQUAL(true, ttype.issuer == v.issuer);
        BOOST_CHECK_EQUAL(true, ttype.issue_time == v.issue_time);
        BOOST_CHECK_EQUAL(ttype.issue.threshold, v.issue.threshold);
        for(int i = 0; i < ttype.issue.groups.size(); i++) {
            auto& lgroup = ttype.issue.groups[i];
            auto& rgroup = v.issue.groups[i];
            BOOST_CHECK_EQUAL(lgroup.type, rgroup.type);
            BOOST_CHECK_EQUAL(true, lgroup.key == rgroup.key);
            BOOST_CHECK_EQUAL(lgroup.weight, rgroup.weight);
        }
    });


}

BOOST_AUTO_TEST_SUITE_END()
