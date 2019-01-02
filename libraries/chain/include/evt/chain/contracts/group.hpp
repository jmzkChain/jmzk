/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#pragma once

#include <string>
#include <functional>
#include <fc/reflect/reflect.hpp>
#include <evt/chain/types.hpp>
#include <evt/chain/address.hpp>
#include <evt/chain/contracts/metadata.hpp>

namespace evt { namespace chain { namespace contracts {

class group {
public:
    struct node {
    public:
        weight_type weight;     // for root node, its weight is zero
        weight_type threshold;  // for leaf node, its threshold is zero
        uint16_t    index;      // for leaf node, it's the key index, otherwise is the node index for non-leaf or root node
        uint16_t    size;       // for non-leaf node, indicates how many child nodes it has

    public:
        bool is_root() const { return weight == 0; }
        bool is_leaf() const { return threshold == 0; }

    public:
        bool validate() const {
            if(is_root()) {
                return threshold > 0 && index > 0 && size > 0;
            }
            if(is_leaf()) {
                return size == 0;
            }
            return index > 0 && size > 0;
        }
    };

    using visit_func = std::function<bool(const node&)>;

public:
    const group_name& name() const { return name_; }
    const address& key() const { return key_; }
    const node& root() const { FC_ASSERT(nodes_.size() > 0); return nodes_[0]; }
    bool empty() const { return nodes_.empty(); }
    const auto& metas() const { return metas_; }

public:
    void visit_root(const visit_func&) const;
    void visit_node(const node&, const visit_func&) const;

public:
    const public_key_type&
    get_leaf_key(const node& n) const {
        FC_ASSERT(n.is_leaf());
        return keys_[n.index];
    }

    const node&
    get_child_node(const node& n, int i) const {
        FC_ASSERT(!n.is_leaf());
        FC_ASSERT(i < n.size);
        return nodes_[n.index + i];
    }

public:
    group_name                       name_;
    address                          key_;
    small_vector<node, 4>            nodes_;
    small_vector<public_key_type, 4> keys_;
    meta_list                        metas_;
};

}}}  // namespac evt::chain::contracts

namespace fc {

class variant;
void to_variant(const evt::chain::contracts::group& group, fc::variant& v);
void from_variant(const fc::variant& v, evt::chain::contracts::group& group);

}  // namespace fc

FC_REFLECT(evt::chain::contracts::group::node, (weight)(threshold)(index)(size));
FC_REFLECT(evt::chain::contracts::group, (name_)(key_)(nodes_)(keys_)(metas_));
