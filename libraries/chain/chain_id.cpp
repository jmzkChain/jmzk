#include <evt/chain/types.hpp>

namespace evt { namespace chain {
    chain_id_type::chain_id_type( const fc::string& s ) { id = fc::sha256(s); }
    chain_id_type::chain_id_type() { id = fc::sha256("1DAF81D021D658B7A55092411F8A856AF5EE2980C0832691B36FDE79B93F28F8"); }
}} // namespace evt::chain
