/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <algorithm>
#include <fc/bitutil.hpp>
#include <fc/io/raw.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <evt/chain/exceptions.hpp>
#include <evt/chain/transaction.hpp>
#include <evt/chain/config.hpp>

namespace evt { namespace chain {

void
transaction_header::set_reference_block(const block_id_type& reference_block) {
    ref_block_num    = fc::endian_reverse_u32(reference_block._hash[0]);
    ref_block_prefix = reference_block._hash[1];
}

bool
transaction_header::verify_reference_block(const block_id_type& reference_block) const {
    return ref_block_num == (decltype(ref_block_num))fc::endian_reverse_u32(reference_block._hash[0])
           && ref_block_prefix == (decltype(ref_block_prefix))reference_block._hash[1];
}

void
transaction_header::validate() const {}

transaction_id_type
transaction::id() const {
    digest_type::encoder enc;
    fc::raw::pack(enc, *this);
    return enc.result();
}

digest_type
transaction::sig_digest(const chain_id_type& chain_id) const {
    digest_type::encoder enc;
    fc::raw::pack(enc, chain_id);
    fc::raw::pack(enc, *this);
    return enc.result();
}

public_keys_set
transaction::get_signature_keys(const signatures_base_type& signatures, const chain_id_type& chain_id,
                                bool allow_duplicate_keys) const {
    if(signatures.empty()) {
        return public_keys_set();
    }

    try {
        auto digest = sig_digest(chain_id);

        auto recovered_pub_keys = public_keys_set();
        for(auto& sig : signatures) {
            auto successful_insertion                   = false;
            std::tie(std::ignore, successful_insertion) = recovered_pub_keys.emplace(sig, digest);
            EVT_ASSERT(allow_duplicate_keys || successful_insertion, tx_duplicate_sig,
                       "transaction includes more than one signature signed using the same key associated with public "
                       "key: ${key}",
                       ("key", public_key_type(sig, digest)));
        }

        return recovered_pub_keys;
    }
    FC_CAPTURE_AND_RETHROW()
}

const signature_type&
signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id) {
    signatures.push_back(key.sign(sig_digest(chain_id)));
    return signatures.back();
}

signature_type
signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id) const {
    return key.sign(sig_digest(chain_id));
}

public_keys_set
signed_transaction::get_signature_keys(const chain_id_type& chain_id, bool allow_duplicate_keys) const {
    return transaction::get_signature_keys(signatures, chain_id, allow_duplicate_keys);
}

void
packed_transaction::reflector_init() {
   // called after construction, but always on the same thread and before packed_transaction passed to any other threads
   static_assert(fc::raw::has_feature_reflector_init_on_unpacked_reflected_types,
                 "FC unpack needs to call reflector_init otherwise unpacked_trx will not be initialized");
   EVT_ASSERT(unpacked_trx.expiration == time_point_sec(), tx_decompression_error, "packed_transaction already unpacked");
   local_unpack_transaction();
}

uint32_t
packed_transaction::get_unprunable_size() const {
    uint64_t size = config::fixed_net_overhead_of_packed_trx;
    size += packed_trx.size();
    EVT_ASSERT(size <= std::numeric_limits<uint32_t>::max(), tx_too_big, "packed_transaction is too big");
    return static_cast<uint32_t>(size);
}

uint32_t
packed_transaction::get_prunable_size() const {
    uint64_t size = fc::raw::pack_size(signatures);
    EVT_ASSERT(size <= std::numeric_limits<uint32_t>::max(), tx_too_big, "packed_transaction is too big");
    return static_cast<uint32_t>(size);
}

digest_type
packed_transaction::packed_digest() const {
    digest_type::encoder prunable;
    fc::raw::pack(prunable, signatures);

    digest_type::encoder enc;
    fc::raw::pack(enc, compression);
    fc::raw::pack(enc, packed_trx);
    fc::raw::pack(enc, prunable.result());

    return enc.result();
}

namespace bio = boost::iostreams;

template <size_t Limit>
struct read_limiter {
    using char_type = char;
    using category  = bio::multichar_output_filter_tag;

    template <typename Sink>
    size_t
    write(Sink& sink, const char* s, size_t count) {
        EVT_ASSERT(_total + count <= Limit, tx_decompression_error, "Exceeded maximum decompressed transaction size");
        _total += count;
        return bio::write(sink, s, count);
    }

    size_t _total = 0;
};

static transaction
unpack_transaction(const bytes& data) {
    return fc::raw::unpack<transaction>(data);
}

static bytes
zlib_decompress(const bytes& data) {
    try {
        auto out    = bytes();
        auto decomp = bio::filtering_ostream();

        decomp.push(bio::zlib_decompressor());
        decomp.push(read_limiter<1 * 1024 * 1024>());  // limit to 1 meg decompressed for zip bomb protections
        decomp.push(bio::back_inserter(out));
        bio::write(decomp, data.data(), data.size());
        bio::close(decomp);
        return out;
    }
    catch(fc::exception& er) {
        throw;
    }
    catch(...) {
        fc::unhandled_exception er(FC_LOG_MESSAGE(warn, "internal decompression error"), std::current_exception());
        throw er;
    }
}

static transaction
zlib_decompress_transaction(const bytes& data) {
    bytes out = zlib_decompress(data);
    return unpack_transaction(out);
}

static bytes
pack_transaction(const transaction& t) {
    return fc::raw::pack(t);
}

static bytes
zlib_compress_transaction(const transaction& t) {
    auto in   = pack_transaction(t);
    auto out  = bytes();
    auto comp = bio::filtering_ostream();

    comp.push(bio::zlib_compressor(bio::zlib::best_compression));
    comp.push(bio::back_inserter(out));
    bio::write(comp, in.data(), in.size());
    bio::close(comp);
    return out;
}

void
packed_transaction::local_unpack_transaction() {
    try {
        switch(compression) {
        case none:
            unpacked_trx = signed_transaction(unpack_transaction(packed_trx), signatures);
            break;
        case zlib:
            unpacked_trx = signed_transaction(zlib_decompress_transaction(packed_trx), signatures);
            break;
        default:
            EVT_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
        }
    }
    FC_CAPTURE_AND_RETHROW((compression)(packed_trx))
}

void
packed_transaction::local_pack_transaction() {
    try {
        switch(compression) {
        case none:
            packed_trx = pack_transaction(unpacked_trx);
            break;
        case zlib:
            packed_trx = zlib_compress_transaction(unpacked_trx);
            break;
        default:
            EVT_THROW(unknown_transaction_compression, "Unknown transaction compression algorithm");
        }
    }
    FC_CAPTURE_AND_RETHROW((compression))
}

}}  // namespace evt::chain
