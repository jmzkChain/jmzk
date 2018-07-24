#include <catch/catch.hpp>

#include <evt/chain/types.hpp>
#include <evt/chain/address.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/contracts/authorizer_ref.hpp>
#include <evt/chain/contracts/evt_link.hpp>

using namespace evt::chain;
using namespace evt::chain::contracts;

TEST_CASE("test_address", "[types]") {
    auto addr = address();

    CHECK(addr.is_reserved());
    auto var1 = fc::variant();
    fc::to_variant(addr, var1);
    CHECK(var1.is_string());
    CHECK(var1.get_string() == "EVT00000000000000000000000000000000000000000000000000");

    address addr2;
    fc::from_variant(var1, addr2);
    CHECK(addr2.is_reserved());

    CHECK(addr == addr2);

    auto keystr = std::string("EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3");
    auto pkey = public_key_type(keystr);

    addr.set_public_key(pkey);
    CHECK(addr.is_public_key());
    auto var2 = fc::variant();
    fc::to_variant(addr, var2);
    CHECK(var2.is_string());
    CHECK(var2.get_string() == "EVT6bMPrzVm77XSjrTfZxEsbAuWPuJ9hCqGRLEhkTjANWuvWTbwe3");

    address addr3;
    fc::from_variant(var2, addr3);
    CHECK(addr3.is_public_key());
    CHECK(addr3.get_public_key() == pkey);

    CHECK(addr == addr3);

    addr.set_generated("xxxxxxxxxxxx", "xxxxxxxxxxxxxxxxxxxxx", 1234);
    CHECK(addr.is_generated());
    auto var3 = fc::variant();
    fc::to_variant(addr, var3);

    address addr4;
    fc::from_variant(var3, addr4);
    CHECK(addr4.is_generated());
    CHECK(addr4.get_prefix() == "xxxxxxxxxxxx");
    CHECK(addr4.get_key() == "xxxxxxxxxxxxxxxxxxxxx");
    CHECK(addr4.get_nonce() == 1234);

    CHECK(addr == addr4);
    INFO(addr4.to_string());
}

TEST_CASE("test_link", "[types]") {
    auto str = "https://evt.li/105872245008276224820591850571406257599055110087309045699"
               "857346357746726272659157993059159349575304490639719329170682417766-50650"
               "888506055079811583602057798359777036080208936513190985560278747067750843"
               "546989807062585779161049048705714969431924010459958553375007765386537646"
               "589480404085653550805144861548482671327759747922103129049559214656483145"
               "652620127881665022904010501332559092813546656391959204942463147561690656"
               "761339314412424841775995984000887229638084203771090463957213545978606258"
               "503004791138876832340228078575546381687000899946944997703639139337718538"
               "94308706279167138159625333661707";
    auto link = evt_link::parse_from_evtli(str);

    CHECK(link.get_segment(41).intv == 11);
    CHECK(link.get_segment(42).intv == 1532413494);
    CHECK(link.get_segment(91).intv == 0);
    CHECK(link.get_segment(91).strv == "testdomain");
    CHECK(link.get_segment(92).strv == "testtoken");

    auto uid = std::string();
    uid.push_back(220);
    uid.push_back(178);
    uid.push_back(159);
    uid.push_back(254);
    uid.push_back(169);
    uid.push_back(136);
    uid.push_back(34);
    uid.push_back(185);
    uid.push_back(65);
    uid.push_back(18);
    uid.push_back(98);
    uid.push_back(248);
    uid.push_back(71);
    uid.push_back(246);
    uid.push_back(18);
    uid.push_back(102);

    CHECK(link.get_segment(156).strv == uid);

    auto& sigs = link.get_signatures();
    CHECK(sigs.size() == 3);

    CHECK(sigs.find(signature_type(std::string("SIG_K1_KWvwzRSLgJQJbvTGEoo5TqKwSiSHJnPfKBct6H1ArfVrQsWUEuy1eK6p6cvCpsEnbZbm89ffqKNu8BbQwkyW4pL8C7s7QW"))) != sigs.end());
    CHECK(sigs.find(signature_type(std::string("SIG_K1_KfkVBKNvDKXhPksdvAMhDViooMt4fRhsSnh5Bx9VqcKoeAf8ZnKo1MPQZMV7rskgTbu36nGjh6jpRf6rLoHEvLrzGgn1ub"))) != sigs.end());
    CHECK(sigs.find(signature_type(std::string("SIG_K1_K36D1VmoWrzEqi8uQ1ysT3wagj1HcrrCUwo582hQRGV39Q7xT3J3BgigypvqJtNQjUzgmrNEaSrS81AAbe2EFeBXTvckSR"))) != sigs.end());

    auto pkeys = link.restore_keys();
    CHECK(pkeys.size() == 3);

    CHECK(pkeys.find(public_key_type(std::string("EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"))) != pkeys.end());
    CHECK(pkeys.find(public_key_type(std::string("EVT6MYSkiBHNDLxE6JfTmSA1FxwZCgBnBYvCo7snSQEQ2ySBtpC6s"))) != pkeys.end());
    CHECK(pkeys.find(public_key_type(std::string("EVT7bUYEdpHiKcKT9Yi794MiwKzx5tGY3cHSh4DoCrL4B2LRjRgnt"))) != pkeys.end());
}
