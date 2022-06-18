/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#include <jmzk/chain/contracts/group.hpp>
#include <jmzk/chain/config.hpp>
#include <limits>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/variant.hpp>
#include <jmzk/chain/exceptions.hpp>

namespace jmzk { namespace chain { namespace contracts {

void
group::visit_root(const visit_func& func) const {
    jmzk_ASSERT(nodes_.size() > 0, group_type_exception, "There's not any node defined in this group");
    return visit_node(nodes_[0], func);
}

void
group::visit_node(const node& node, const visit_func& func) const {
    if(node.is_leaf()) {
        return;
    }
    for(uint i = 0; i < node.size; i++) {
        auto& cnode = nodes_[i + node.index];
        if(!func(cnode)) {
            break;
        }
    }
}

}}}  // namespac jmzk::chain::contracts

namespace fc {
using namespace jmzk::chain;
using namespace jmzk::chain::contracts;

void
to_variant(const group& group, const group::node& node, fc::variant& v) {
    fc::mutable_variant_object mv;
    if(node.is_leaf()) {
        to_variant(group.keys_[node.index], mv["key"]);
        mv["weight"] = node.weight;
        v = mv;
        return;
    }
    
    mv["threshold"] = node.threshold;
    if(node.weight > 0) {
        mv["weight"] = node.weight;
    }

    fc::variants cvs;
    cvs.reserve(node.size);
    for(uint i = 0; i < node.size; i++) {
        auto& cnode = group.nodes_[node.index + i];
        fc::variant cv;
        to_variant(group, cnode, cv);
        cvs.emplace_back(std::move(cv));
    }
    mv["nodes"] = std::move(cvs);
    v = mv;
}

void
to_variant(const jmzk::chain::contracts::group& group, fc::variant& v) {
    fc::mutable_variant_object mv;
    to_variant(group.name_, mv["name"]);
    to_variant(group.key_, mv["key"]);
    to_variant(group, group.nodes_[0], mv["root"]);
    to_variant(group.metas_, mv["metas"]);
    v = mv;
}

void
from_variant(const fc::variant& v, group& group, group::node& node, uint32_t depth) {
    jmzk_ASSERT(depth < config::default_max_auth_depth, group_type_exception, "Exceeds max node depth");

    auto& vo = v.get_object();
    if(vo.find("threshold") == vo.end()) {
        // leaf node
        node.weight = v["weight"].as<weight_type>();
        node.threshold = 0;
        node.size = 0;
        
        public_key_type key;
        from_variant(v["key"], key);
        jmzk_ASSERT(group.keys_.size() < std::numeric_limits<decltype(node.index)>::max(), group_type_exception, "Exceeds max keys limit");
        node.index = group.keys_.size();
        group.keys_.emplace_back(std::move(key));
        return;
    }
    if(depth == 0) {
        // root node
        node.weight = 0;
    }
    else {
        node.weight = v["weight"].as<weight_type>();
    }
    node.threshold = v["threshold"].as<weight_type>();
    
    auto& cvs = v["nodes"].get_array();
    jmzk_ASSERT(cvs.size() < std::numeric_limits<decltype(node.size)>::max(), group_type_exception, "Exceeds max child nodes limit");
    jmzk_ASSERT(group.nodes_.size() + cvs.size() < std::numeric_limits<decltype(node.index)>::max(), group_type_exception, "Exceeds max nodes limit");
    node.index = group.nodes_.size();
    node.size = cvs.size();

    auto index = node.index;
    group.nodes_.resize(group.nodes_.size() + cvs.size());
    for(uint i = 0; i < cvs.size(); i++) {
        auto cv = cvs[i];
        from_variant(cv, group, group.nodes_[index + i], depth + 1);
    }
}

void
from_variant(const fc::variant& v, jmzk::chain::contracts::group& group) {
    auto& vo = v.get_object();
    if(vo.find("name") != vo.end()) {
        from_variant(vo["name"], group.name_);
    }
    jmzk_ASSERT(!group.key_.is_generated(), group_type_exception, "Generated group key is not allowed here");
    from_variant(vo["key"], group.key_);
    group.nodes_.resize(1);
    from_variant(vo["root"], group, group.nodes_[0], 0);
}

}  // namespace fc