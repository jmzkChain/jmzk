#include <fc/retainable.hpp>
#include <assert.h>
#include <atomic>

namespace fc {
  retainable::retainable()
  :_ref_count(1) { 
     static_assert( sizeof(_ref_count) == sizeof(std::atomic<int32_t>), "failed to reserve enough space" );
  }

  retainable::~retainable() { 
    assert( _ref_count <= 0 );
    assert( _ref_count == 0 );
  }
  void retainable::retain() {
    ((std::atomic<int32_t>*)&_ref_count)->fetch_add(1, std::memory_order_relaxed );
  }

  void retainable::release() {
        std::atomic_thread_fence(std::memory_order_acquire);
    if( 1 == ((std::atomic<int32_t>*)&_ref_count)->fetch_sub(1, std::memory_order_release ) ) {
        delete this;
    }
  }

  int32_t retainable::retain_count()const {
    return _ref_count;
  }
}
