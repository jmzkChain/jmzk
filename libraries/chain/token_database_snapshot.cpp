#include <evt/chain/token_database_snapshot.hpp>

#include <string.h>
#include <vector>
#include <fmt/format.h>
#include <rocksdb/db.h>
#include <evt/chain/token_database.hpp>

namespace evt { namespace chain {

namespace internal {

// TODO: Replace with values provided by token database class directly
const char* section_names[] = {
    ".asset",
    ".domain",
    ".token",
    ".group",
    ".suspend",
    ".lock",
    ".fungible",
    ".prodvote",
    ".evtlink",
    ".psvbonus",
    ".psvbonus-dist",
    ".validator",
    ".stakepool",
    ".blackaddrs"
};

void
add_reserved_tokens(snapshot_writer_ptr          writer, 
                    const token_database&        db, 
                    std::vector<domain_name>&    domains,
                    std::vector<symbol_id_type>& symbol_ids) {
    static_assert(sizeof(section_names) / sizeof(char*) == (int)token_type::max_value + 1);

    for(auto i = (int)token_type::domain; i <= (int)token_type::max_value; i++) {
        if(i == (int)token_type::asset || i == (int)token_type::token) {
            continue;
        }
        writer->write_section(section_names[i], [&](auto& w) {
            db.read_tokens_range((token_type)i, std::nullopt, 0, [&](auto& key, auto&& v) {
                assert(key.size() == sizeof(name128));

                w.add_row(key.data(), key.size());
                w.add_row(v);

                // we should use memcpy here
                // it's UB when interpret char* as name128*
                // because it may not be aligened
                auto n = name128();
                memcpy(&n, key.data(), sizeof(name128));

                if(i == (int)token_type::domain) {
                    domains.push_back(n);
                }
                else if(i == (int)token_type::fungible) {
                    symbol_ids.push_back((symbol_id_type)n.value);
                }

                return true;
            });
        });
    }
}

void
add_tokens(snapshot_writer_ptr writer, const token_database& db, const std::vector<domain_name> domains) {
    for(auto& d : domains) {
        writer->write_section(d.to_string(), [&](auto& w) {
            db.read_tokens_range(token_type::token, d, 0, [&w](auto& key, auto&& v) {
                w.add_row(key.data(), key.size());
                w.add_row(v);

                return true;
            });
        });
    }
}

void
add_assets(snapshot_writer_ptr writer, const token_database& db, const std::vector<symbol_id_type>& symbol_ids) {
    for(auto& id : symbol_ids) {
        auto sn = fmt::format(".asset-{}", id);
        writer->write_section(sn, [&](auto& w) {
            db.read_assets_range(id, 0, [&w](auto& key, auto&& v) {
                assert(key.size() == sizeof(fc::ecc::public_key_shim));
                w.add_row(key.data(), key.size());
                w.add_row(v);

                return true;
            });
        });
    }
}

void
read_reserved_tokens(snapshot_reader_ptr          reader,
                     token_database&              db,
                     std::vector<domain_name>&    domains,
                     std::vector<symbol_id_type>& symbol_ids) {
    for(auto i = (int)token_type::domain; i <= (int)token_type::max_value; i++) {
        if(i == (int)token_type::asset || i == (int)token_type::token) {
            continue;
        }

        reader->read_section(section_names[i], [&](auto& r) {
            while(!r.eof()) {
                auto k = uint128_t(0);
                auto v = std::string();

                r.read_row((char*)&k, sizeof(k));
                r.read_row(v);

                db.put_token((token_type)i, action_op::put, std::nullopt, k, std::string_view(v.data(), v.size()));

                if(i == (int)token_type::domain) {
                    domains.emplace_back(k);
                }
                else if(i == (int)token_type::fungible) {
                    symbol_ids.emplace_back((symbol_id_type)k);
                }
            }
        });
    }
}

void
read_tokens(snapshot_reader_ptr reader, token_database& db, const std::vector<domain_name>& domains) {
    for(auto& d : domains) {
        reader->read_section(d.to_string(), [&](auto& r) {
            while(!r.eof()) {
                auto k = name128();
                auto v = std::string();

                r.read_row((char*)&k, sizeof(k));
                r.read_row(v);

                db.put_token(token_type::token, action_op::put, d, k, std::string_view(v.data(), v.size()));
            }
        });
    }
}

void
read_assets(snapshot_reader_ptr reader, token_database& db, const std::vector<symbol_id_type>& symbol_ids) {
    for(auto& id : symbol_ids) {
        auto sn = fmt::format(".asset-{}", id);
        reader->read_section(sn, [&](auto& r) {
            while(!r.eof()) {
                auto k = fc::ecc::public_key_shim();
                auto v = std::string();

                r.read_row((char*)&k, sizeof(k));
                r.read_row(v);

                auto addr = address(public_key_type(k));
                db.put_asset(addr, id, std::string_view(v.data(), v.size()));
            }
        });
    }
}

}  // namespace internal

void
token_database_snapshot::add_to_snapshot(snapshot_writer_ptr writer, const token_database& db) {
    using namespace internal;

    try {
        auto domains    = std::vector<domain_name>();
        auto symbol_ids = std::vector<symbol_id_type>();

        add_reserved_tokens(writer, db, domains, symbol_ids);
        add_tokens(writer, db, domains);
        add_assets(writer, db, symbol_ids);
    }
    EVT_CAPTURE_AND_RETHROW(token_database_snapshot_exception);
}

void
token_database_snapshot::read_from_snapshot(snapshot_reader_ptr reader, token_database& db) {
    using namespace internal;

    try {
        // clear all the savepoints
        db.close(false);
        db.open(false);

        FC_ASSERT(db.savepoints_size() == 0);

        auto domains    = std::vector<domain_name>();
        auto symbol_ids = std::vector<symbol_id_type>();

        read_reserved_tokens(reader, db, domains, symbol_ids);
        read_tokens(reader, db, domains);
        read_assets(reader, db, symbol_ids);        
    }
    EVT_CAPTURE_AND_RETHROW(token_database_snapshot_exception);
}

}}  // namespace evt::chain
