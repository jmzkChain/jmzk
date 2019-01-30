#include <catch/catch.hpp>

#include <evt/chain/address.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/token_database.hpp>
#include <evt/chain/contracts/authorizer_ref.hpp>
#include <evt/chain/contracts/evt_link.hpp>
#include <evt/chain/contracts/types.hpp>

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
    auto str2 = link.to_string();
    
    CHECK(str == str2);

    CHECK(link.get_header() == 3);
    CHECK(*link.get_segment(evt_link::timestamp).intv == 1532465234);
    CHECK(link.get_segment(evt_link::domain).intv.has_value() == false);
    CHECK(*link.get_segment(evt_link::domain).strv == "nd1532465232490");
    CHECK(*link.get_segment(evt_link::token).strv == "tk3064930465.8381");

    auto uid = std::string();
    uid.push_back((char)249);
    uid.push_back((char)136);
    uid.push_back((char)100);
    uid.push_back((char)134);
    uid.push_back((char)20);
    uid.push_back((char)86);
    uid.push_back((char)38);
    uid.push_back((char)125);
    uid.push_back((char)124);
    uid.push_back((char)173);
    uid.push_back((char)243);
    uid.push_back((char)124);
    uid.push_back((char)140);
    uid.push_back((char)182);
    uid.push_back((char)117);
    uid.push_back((char)147);

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
    auto str2 = link.to_string(1);
    
    CHECK(str == str2);

    CHECK(link.get_header() == 11);
    CHECK(*link.get_segment(evt_link::timestamp).intv == 1532465608);
    CHECK(link.get_segment(evt_link::domain).intv.has_value() == false);
    CHECK(*link.get_segment(evt_link::domain).strv == "testdomain");
    CHECK(*link.get_segment(evt_link::token).strv == "testtoken");

    auto uid = std::string();
    uid.push_back((char)102);
    uid.push_back((char)24);
    uid.push_back((char)21);
    uid.push_back((char)81);
    uid.push_back((char)221);
    uid.push_back((char)147);
    uid.push_back((char)189);
    uid.push_back((char)88);
    uid.push_back((char)93);
    uid.push_back((char)215);
    uid.push_back((char)204);
    uid.push_back((char)121);
    uid.push_back((char)77);
    uid.push_back((char)131);
    uid.push_back((char)249);
    uid.push_back((char)168);

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
    auto str2 = link.to_string();
    
    // CHECK(str == str2); because evtjs generated link doesn't guarantee the order of signatures

    CHECK(link.get_header() == 11);
    CHECK(*link.get_segment(evt_link::timestamp).intv == 1532468461);
    CHECK(link.get_segment(evt_link::domain).intv.has_value() == false);
    CHECK(*link.get_segment(evt_link::domain).strv == "testdomain");
    CHECK(*link.get_segment(evt_link::token).strv == "testtoken");

    auto uid = std::string();
    uid.push_back((char)249);
    uid.push_back((char)31);
    uid.push_back((char)135);
    uid.push_back((char)246);
    uid.push_back((char)101);
    uid.push_back((char)32);
    uid.push_back((char)91);
    uid.push_back((char)177);
    uid.push_back((char)24);
    uid.push_back((char)132);
    uid.push_back((char)216);
    uid.push_back((char)242);
    uid.push_back((char)244);
    uid.push_back((char)141);
    uid.push_back((char)118);
    uid.push_back((char)82);

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

TEST_CASE("test_name", "[types]") {
    auto CHECK_RESERVED = [](auto& str) {
        auto n = name(str);
        CHECK(n.reserved());
    };
    auto CHECK_NOT_RESERVED = [](auto& str) {
        auto n = name(str);
        CHECK(!n.reserved());
    };

    CHECK_RESERVED(".1");
    CHECK_RESERVED(".12");
    CHECK_RESERVED(".abc");
    CHECK_RESERVED("..abc");

    CHECK_NOT_RESERVED("1.1");
    CHECK_NOT_RESERVED("123.a");
    CHECK_NOT_RESERVED("abc.1");
    CHECK_NOT_RESERVED("abc..12");
    CHECK_NOT_RESERVED("abc...12");
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
        CHECK(r.size() == (size_t)size);
        CHECK_RAW(r, n);
    };

    CHECK_N128("", 4);
    CHECK_N128("123", 4);
    CHECK_N128("12345", 4);
    CHECK_N128("123456", 8);
    CHECK_N128("1234567890", 8);
    CHECK_N128("1234567890A", 12);
    CHECK_N128("11111111111", 12);
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

    auto CHECK_NUM = [](auto v) {
        auto n1 = name128::from_number(v);
        auto n2 = name128(std::to_string(v));

        INFO(v);
        REQUIRE(n1.value == n2.value);
        REQUIRE(n1.to_string() == n2.to_string());
    };
    for(uint128_t i = 0u; i < std::numeric_limits<uint64_t>::max(); i = i * 10 + 1) {
        CHECK_NUM((uint64_t)i);
    }
}

TEST_CASE("test_symbol", "[types]") {
    auto s = symbol(3, 123);
    CHECK((std::string)s == "3,S#123");
    CHECK(s.to_string() == "3,S#123");

    auto s2 = symbol::from_string("3,S#123");
    CHECK(s == s2);

    CHECK_THROWS_AS(symbol::from_string("21,S#123"), symbol_type_exception);
    CHECK_THROWS_AS(symbol::from_string("21"), symbol_type_exception);
    CHECK_THROWS_AS(symbol::from_string("3"), symbol_type_exception);
    CHECK_THROWS_AS(symbol::from_string("3,S#abc"), symbol_type_exception);
    CHECK_THROWS_AS(symbol::from_string("a,S#2"), symbol_type_exception);
    CHECK_THROWS_AS(symbol::from_string("3,S2"), symbol_type_exception);
    CHECK_THROWS_AS(symbol::from_string("3,#2"), symbol_type_exception);
    CHECK_THROWS_AS(symbol::from_string("3,2"), symbol_type_exception);

    CHECK(evt_sym() == symbol(5, 1));
    CHECK(pevt_sym() == symbol(5, 2));
}

TEST_CASE("test_asset", "[types]") {
    auto CHECK_ASSET = [&](auto amount, auto sym, auto str) {
        auto a = asset(amount, sym);
        CHECK((std::string)a == str);
        CHECK(a.to_string() == str);

        auto a2 = asset::from_string(str);
        INFO(a.sym() << " " << a.amount());
        INFO(a2.sym() << " " << a2.amount());
        CHECK(a == a2);
    };

    auto CHECK_PARSE = [&](auto str, auto amount, auto precision, auto id) {
        auto a = asset::from_string(str);
        CHECK(a.precision() == precision);
        CHECK(a.amount() == amount);
        CHECK(a.symbol_id() == (symbol_id_type)id);
    };

    auto sym = symbol(5, 1);
    CHECK_ASSET(0, sym, "0.00000 S#1");
    CHECK_ASSET(1, sym, "0.00001 S#1");
    CHECK_ASSET(10, sym, "0.00010 S#1");
    CHECK_ASSET(100, sym, "0.00100 S#1");
    CHECK_ASSET(1000, sym, "0.01000 S#1");
    CHECK_ASSET(10000, sym, "0.10000 S#1");
    CHECK_ASSET(100000, sym, "1.00000 S#1");
    CHECK_ASSET(1000000, sym, "10.00000 S#1");

    CHECK_ASSET(-1, sym, "-0.00001 S#1");
    CHECK_ASSET(-10, sym, "-0.00010 S#1");
    CHECK_ASSET(-100, sym, "-0.00100 S#1");
    CHECK_ASSET(-1000, sym, "-0.01000 S#1");
    CHECK_ASSET(-10000, sym, "-0.10000 S#1");
    CHECK_ASSET(-100000, sym, "-1.00000 S#1");
    CHECK_ASSET(-1000000, sym, "-10.00000 S#1");

    CHECK_ASSET(100, symbol(0,1), "100 S#1");
    CHECK_ASSET(0, symbol(0,1), "0 S#1");

    CHECK_PARSE("0.001 S#123", 1, 3, 123);
    CHECK_PARSE("0.0010 S#123", 10, 4, 123);
    CHECK_PARSE("0.199 S#123", 199, 3, 123);
    CHECK_PARSE("0.1990 S#123", 1990, 4, 123);
    CHECK_PARSE("1.0120 S#123", 10120, 4, 123);
    CHECK_PARSE("1003.0120 S#123", 10030120, 4, 123);
    CHECK_PARSE("1003 S#123", 1003, 0, 123);
    CHECK_PARSE("0 S#123", 0, 0, 123);

    CHECK_PARSE("-0.001 S#123", -1, 3, 123);
    CHECK_PARSE("-0.0010 S#123", -10, 4, 123);
    CHECK_PARSE("-0.199 S#123", -199, 3, 123);
    CHECK_PARSE("-0.1990 S#123", -1990, 4, 123);
    CHECK_PARSE("-1.0120 S#123", -10120, 4, 123);
    CHECK_PARSE("-1003.0120 S#123", -10030120, 4, 123);
    CHECK_PARSE("-1003 S#123", -1003, 0, 123);

    CHECK_THROWS_AS(asset::from_string("0.1S#1"), asset_type_exception);
    CHECK_THROWS_AS(asset::from_string("0.1 S#a"), asset_type_exception);
    CHECK_THROWS_AS(asset::from_string("0.1 S1"), asset_type_exception);
    CHECK_THROWS_AS(asset::from_string("0.100a S#1"), asset_type_exception);
}

TEST_CASE("test_make_db_value", "[types]") {
    auto CHECK_MAKE = [](auto sz) {
        auto str = std::string();
        str.resize(sz);
        CHECK_NOTHROW(make_db_value(str));
    };

    for(auto i = 1; i < 16; i++) {
        CHECK_MAKE(i * 512);
        CHECK_MAKE(i * 512 + 1);
        CHECK_MAKE(i * 512 - 1);
    }
}
