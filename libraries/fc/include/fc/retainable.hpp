#pragma once
#include <fc/utility.hpp>

namespace fc {

  /**
   *  @brief used to create reference counted types.
   *
   *  Must be a virtual base class that is initialized with the
   *
   */
  class retainable {
    public:
      retainable();
      void    retain();
      void    release();
      int32_t retain_count()const;

    protected:
      virtual ~retainable();
    private:
      volatile int32_t _ref_count;
  };
}
