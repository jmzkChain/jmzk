#include <catch/catch.hpp>

#include <evt/chain/address.hpp>
#include <evt/chain/contracts/authorizer_ref.hpp>
#include <evt/chain/contracts/evt_link.hpp>
#include <evt/chain/contracts/types.hpp>
#include <evt/chain/types.hpp>

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
    auto pkey   = public_key_type(keystr);

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

TEST_CASE("test_link_1", "[types]") {
    auto str = "03XBY4E/KTS:PNHVA3JP9QG258F08JHYOYR5SLJGN0EA-C3J6S:2G:T1SX7WA1"
               "4KH9ETLZ97TUX9R9JJA6+06$E/_PYNX-/152P4CTC:WKXLK$/7G-K:89+::2K4"
               "C-KZ2**HI-P8CYJ**XGFO1K5:$E*SOY8MFYWMNHP*BHX2U8$$FTFI81YDP1HT";
    auto link = evt_link::parse_from_evtli(str);

    CHECK(link.get_header() == 3);
    CHECK(*link.get_segment(evt_link::timestamp).intv == 1532465234);
    CHECK(link.get_segment(evt_link::domain).intv.valid() == false);
    CHECK(*link.get_segment(evt_link::domain).strv == "nd1532465232490");
    CHECK(*link.get_segment(evt_link::token).strv == "tk3064930465.8381");

    auto uid = std::string();
    uid.push_back(249);
    uid.push_back(136);
    uid.push_back(100);
    uid.push_back(134);
    uid.push_back(20);
    uid.push_back(86);
    uid.push_back(38);
    uid.push_back(125);
    uid.push_back(124);
    uid.push_back(173);
    uid.push_back(243);
    uid.push_back(124);
    uid.push_back(140);
    uid.push_back(182);
    uid.push_back(117);
    uid.push_back(147);

    CHECK(link.get_segment(evt_link::link_id).strv == uid);

    auto& sigs = link.get_signatures();
    CHECK(sigs.size() == 1);

    CHECK(sigs.find(signature_type(std::string("SIG_K1_JyyaM7x9a4AjaD8yaG6iczgHskUFPvkWEk7X5DPkdZfRGBxYTbpLJ1y7gvmeL4vMqrMmw6QwtErfKUds5L7sxwU2nR7mvu"))) != sigs.end());

    auto pkeys = link.restore_keys();
    CHECK(pkeys.size() == 1);

    CHECK(pkeys.find(public_key_type(std::string("EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"))) != pkeys.end());
}

TEST_CASE("test_link_2", "[types]") {
    auto str = "https://evt.li/04OH4QSYU-9:0ISOMCF2AY*JO/O/7VTMZLC6W*F0NQ831F+60"
               "7/$9/9F/T6HT:FU*W99Q_PWV-SEQQOBAI6AXPY-32ZV:DTQ8BNCA$Z15-OHQ7*9O"
               "+CGUBIMTB261AT$6:*I+UKBHSQP3D84/JEZDG7BEJ5OUD$ZINCC24";

    auto link = evt_link::parse_from_evtli(str);

    CHECK(link.get_header() == 11);
    CHECK(*link.get_segment(evt_link::timestamp).intv == 1532465608);
    CHECK(link.get_segment(evt_link::domain).intv.valid() == false);
    CHECK(*link.get_segment(evt_link::domain).strv == "testdomain");
    CHECK(*link.get_segment(evt_link::token).strv == "testtoken");

    auto uid = std::string();
    uid.push_back(102);
    uid.push_back(24);
    uid.push_back(21);
    uid.push_back(81);
    uid.push_back(221);
    uid.push_back(147);
    uid.push_back(189);
    uid.push_back(88);
    uid.push_back(93);
    uid.push_back(215);
    uid.push_back(204);
    uid.push_back(121);
    uid.push_back(77);
    uid.push_back(131);
    uid.push_back(249);
    uid.push_back(168);

    CHECK(link.get_segment(evt_link::link_id).strv == uid);

    auto& sigs = link.get_signatures();
    CHECK(sigs.size() == 1);

    CHECK(sigs.find(signature_type(std::string("SIG_K1_JxEoFdWMsK9dsmqWKrouhkE7qdj1Up8Er4R1jrUiL3zqEm3pmiPGpriJeVP4Fx2TceYX7iVaNjAsjBdiaJSgqSiwJKatNz"))) != sigs.end());

    auto pkeys = link.restore_keys();
    CHECK(pkeys.size() == 1);

    CHECK(pkeys.find(public_key_type(std::string("EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"))) != pkeys.end());
}

TEST_CASE("test_link_3", "[types]") {
    auto str = "04OH4QS:OERU*WONPUIU+Z3BQE6C4QG7ONJ16GUBT2HE5XCN87ZG651*OV-VLBH69RNB0_FWFIIX04G6X-28M"
               "HW*EO/$JB2+GM-OK8N52EKZP471H4Q96T*3CD:*ITVNM7$WWAWZTPQKN4LUSBH+*9KXEYAJ9R$5R32LFISP0W"
               "J*KXXMX8$C8*005AX-VCA60JJFBZ6+T$7CLHKPH2W-4I93I+I5ZPUYR1O6X:8A/+TYKIWG88UE$M74URQ:TEJ"
               "SK+N5*WJZ:6H3I$RLQZ*Y7-OO8G1060NLL5+RRVJTJXF0Y0:0MYM0/EF+/KJUY79G9WD8R0IVA2TA$2/1JLAS"
               "Y6$3M9-RP-6/YPM7:3P";

    auto link = evt_link::parse_from_evtli(str);

    CHECK(link.get_header() == 11);
    CHECK(*link.get_segment(evt_link::timestamp).intv == 1532468461);
    CHECK(link.get_segment(evt_link::domain).intv.valid() == false);
    CHECK(*link.get_segment(evt_link::domain).strv == "testdomain");
    CHECK(*link.get_segment(evt_link::token).strv == "testtoken");

    auto uid = std::string();
    uid.push_back(249);
    uid.push_back(31);
    uid.push_back(135);
    uid.push_back(246);
    uid.push_back(101);
    uid.push_back(32);
    uid.push_back(91);
    uid.push_back(177);
    uid.push_back(24);
    uid.push_back(132);
    uid.push_back(216);
    uid.push_back(242);
    uid.push_back(244);
    uid.push_back(141);
    uid.push_back(118);
    uid.push_back(82);

    CHECK(link.get_segment(evt_link::link_id).strv == uid);

    auto& sigs = link.get_signatures();
    CHECK(sigs.size() == 3);

    CHECK(sigs.find(signature_type(std::string("SIG_K1_K4DGNFAAV4B8M1KmcE7Xy8o75God5eZMfWRXyCxbjwjDH8KEfnMQtzGoKMqR8xbLm3rdLJb3ZyXgHweVaX89Lq1LdxFEg8"))) != sigs.end());
    CHECK(sigs.find(signature_type(std::string("SIG_K1_KVFibY5eWJ3UTCmxKiWpd2PigYGynQ9NjwxENg6GD41VfKDxhG9wdoaUEfXMpc3ANYQ2pSyb9t9ZkmV7AgawpShDfVVf7H"))) != sigs.end());
    CHECK(sigs.find(signature_type(std::string("SIG_K1_K1FnY42qkeu9Pa1NAGGu9baehuyZNTWtYAsYddRgMQoXVqdZ4Zp9dngY9xbsoAHjvQvuNp5a9Fr89NE4YtMZkWTZcNfHFc"))) != sigs.end());

    auto pkeys = link.restore_keys();
    CHECK(pkeys.size() == 3);

    CHECK(pkeys.find(public_key_type(std::string("EVT8HdQYD1xfKyD7Hyu2fpBUneamLMBXmP3qsYX6HoTw7yonpjWyC"))) != pkeys.end());
    CHECK(pkeys.find(public_key_type(std::string("EVT6MYSkiBHNDLxE6JfTmSA1FxwZCgBnBYvCo7snSQEQ2ySBtpC6s"))) != pkeys.end());
    CHECK(pkeys.find(public_key_type(std::string("EVT7bUYEdpHiKcKT9Yi794MiwKzx5tGY3cHSh4DoCrL4B2LRjRgnt"))) != pkeys.end());
}

TEST_CASE("test_name128", "[types]") {
    auto get_raw = [](auto& n) {
        auto bytes = fc::raw::pack(n);
        return bytes;
    };

    auto CHECK_RAW = [](auto& raw, auto& n) {
        auto n2 = fc::raw::unpack<name128>(raw);
        CHECK(n == n2);
        CHECK(n.value == n2.value);
        auto raw2 = fc::raw::pack(n2);
        CHECK(memcmp((char*)&raw[0], (char*)&raw2[0], raw.size()) == 0);
    };

    auto CHECK_N128 = [&](const auto& str, auto size) {
        auto n = name128(str);
        CHECK((std::string)n == str);
        auto r = get_raw(n);
        CHECK(r.size() == size);
        CHECK_RAW(r, n);
    };

    CHECK_N128("", 4);
    CHECK_N128("123", 4);
    CHECK_N128("12345", 4);
    CHECK_N128("123456", 8);
    CHECK_N128("1234567890", 8);
    CHECK_N128("1234567890A", 12);
    CHECK_N128("1234567890ABCDE", 12);
    CHECK_N128("1234567890ABCDEF", 16);
    CHECK_N128("1234567890ABCDEFGHIJK", 16);

    auto n1 = name128(N128(12345.67890));
    CHECK((std::string)n1 == "12345.67890");

    auto n2 = name128(N128(12345...));
    CHECK((std::string)n2 == "12345");

    auto n3 = name128(N128(.12345));
    CHECK((std::string)n3 == ".12345");

    auto n4 = name128(N128(ABCDEFGZZ));
    CHECK((std::string)n4 == "ABCDEFGZZ");
}
