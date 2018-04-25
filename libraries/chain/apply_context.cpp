/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */
#include <algorithm>
#include <evt/chain/apply_context.hpp>
#include <evt/chain/chain_controller.hpp>
#include <evt/chain/generated_transaction_object.hpp>
#include <boost/container/flat_set.hpp>

using boost::container::flat_set;

namespace evt { namespace chain {
void apply_context::exec_one()
{
   auto start = fc::time_point::now();
   try {
      auto func = mutable_controller.find_apply_handler(act.name);
      EVT_ASSERT(func != nullptr, action_validate_exception, "Action is not valid, ${name} doesn't exist", ("name",act.name));
      (*func)(*this);
   } FC_CAPTURE_AND_RETHROW((_pending_console_output.str()));

   results.applied_actions.emplace_back(action_trace { act, _pending_console_output.str() });
   reset_console();
   results.applied_actions.back()._profiling_us = fc::time_point::now() - start;
}

void apply_context::exec()
{
   exec_one();
} /// exec()

bool apply_context::has_authorized(const domain_name& domain, const domain_key& key)const {
    return act.domain == domain && act.key == key;
}

vector<account_name> apply_context::get_active_producers() const {
   const auto& gpo = controller.get_global_properties();
   vector<account_name> accounts;
   for(const auto& producer : gpo.active_producers.producers)
      accounts.push_back(producer.producer_name);

   return accounts;
}

const bytes& apply_context::get_packed_transaction() {
   if( !trx_meta.packed_trx.size() ) {
      if (_cached_trx.empty()) {
         auto size = fc::raw::pack_size(trx_meta.trx());
         _cached_trx.resize(size);
         fc::datastream<char *> ds(_cached_trx.data(), size);
         fc::raw::pack(ds, trx_meta.trx());
      }

      return _cached_trx;
   }

   return trx_meta.packed_trx;
}

int apply_context::get_action( uint32_t type, uint32_t index, char* buffer, size_t buffer_size )const
{
   const transaction& trx = trx_meta.trx();
   const action* act = nullptr;
   if( type == 1 ) {
      if( index >= trx.actions.size() )
         return -1;
      act = &trx.actions[index];
   }

   auto ps = fc::raw::pack_size( *act );
   if( ps <= buffer_size ) {
      fc::datastream<char*> ds(buffer, buffer_size);
      fc::raw::pack( ds, *act );
   }
   return ps;
}

void apply_context::reset_console() {
   _pending_console_output = std::ostringstream();
   _pending_console_output.setf( std::ios::scientific, std::ios::floatfield );
}

} } /// evt::chain
