#pragma once
#include <stdint.h>
#include <algorithm>
#include <new>
#include <vector>
#include <type_traits>
#include <tuple>
#include <fc/container/small_vector_fwd.hpp>

#ifdef _MSC_VER
#pragma warning(disable: 4482) // nonstandard extension used enum Name::Val, standard in C++11
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN __attribute__((noreturn))
#endif

#define MAX_NUM_ARRAY_ELEMENTS (1024*1024)
#define MAX_SIZE_OF_BYTE_ARRAYS (128*1024*1024)

//namespace std {
//  typedef decltype(sizeof(int)) size_t;
//  typedef decltype(nullptr) nullptr_t;
//}

namespace fc {
  using std::size_t;
  typedef decltype(nullptr) nullptr_t;

  template<typename T> struct remove_reference           { typedef T type;       };
  template<typename T> struct remove_reference<T&>       { typedef T type;       };
  template<typename T> struct remove_reference<T&&>      { typedef T type;       };

  template<typename T> struct deduce           { typedef T type; };
  template<typename T> struct deduce<T&>       { typedef T type; };
  template<typename T> struct deduce<const T&> { typedef T type; };
  template<typename T> struct deduce<T&&>      { typedef T type; };
  template<typename T> struct deduce<const T&&>{ typedef T type; };


  using std::move;
  using std::forward;

  struct true_type  { enum _value { value = 1 }; };
  struct false_type { enum _value { value = 0 }; };

   namespace detail {
      template<typename T> fc::true_type is_class_helper(void(T::*)());
      template<typename T> fc::false_type is_class_helper(...);

      template<typename T, typename A>
      struct supports_allocator {
      private:
         template<typename TT, typename AA>
         static auto test( int ) -> decltype( TT(std::declval<AA>()).get_allocator(), std::true_type() ) { return {}; }

         template<typename, typename>
         static std::false_type test( long ) { return {}; }

      public:
         static constexpr bool value = std::is_same<decltype( test<T,A>( 0 ) ), std::true_type>::value;
      };

      template<typename T, typename A>
      auto default_construct_maybe_with_allocator( A&& allocator )
      -> std::enable_if_t<supports_allocator<T, A>::value , T>
      {
         return T( std::forward<A>(allocator) );
      }

      template<typename T, typename A>
      auto default_construct_maybe_with_allocator( A&& )
      -> std::enable_if_t<!supports_allocator<T, A>::value , T>
      {
         return T();
      }

      template<typename T1, typename T2, typename A>
      auto default_construct_pair_maybe_with_allocator( A&& allocator )
      -> std::enable_if_t<
            supports_allocator<T1, A>::value
            && supports_allocator<T2, A>::value
         , std::pair<T1,T2>>
      {
         return std::pair<T1,T2>( std::piecewise_construct,
                                  std::forward_as_tuple( allocator ),
                                  std::forward_as_tuple( allocator )
         );
      }

      template<typename T1, typename T2, typename A>
      auto default_construct_pair_maybe_with_allocator( A&& allocator )
      -> std::enable_if_t<
            supports_allocator<T1, A>::value
            && !supports_allocator<T2, A>::value
         , std::pair<T1,T2>>
      {
         return std::pair<T1,T2>( std::piecewise_construct,
                                  std::forward_as_tuple( std::forward<A>(allocator) ),
                                  std::forward_as_tuple()
         );
      }

      template<typename T1, typename T2, typename A>
      auto default_construct_pair_maybe_with_allocator( A&& allocator )
      -> std::enable_if_t<
            !supports_allocator<T1, A>::value
            && supports_allocator<T2, A>::value
         , std::pair<T1,T2>>
      {
         return std::pair<T1,T2>( std::piecewise_construct,
                                  std::forward_as_tuple(),
                                  std::forward_as_tuple( std::forward<A>(allocator) )
         );
      }

      template<typename T1, typename T2, typename A>
      auto default_construct_pair_maybe_with_allocator( A&& allocator )
      -> std::enable_if_t<
            !supports_allocator<T1, A>::value
            && !supports_allocator<T2, A>::value
         , std::pair<T1,T2>>
      {
         return std::pair<T1,T2>();
      }

   }

  template<typename T>
  struct is_class { typedef decltype(detail::is_class_helper<T>(0)) type; enum value_enum { value = type::value }; };
#ifdef min
#undef min
#endif
  template<typename T>
  const T& min( const T& a, const T& b ) { return a < b ? a: b; }

  constexpr size_t const_strlen(const char* str) {
     int i = 0;
     while(*(str+i) != '\0')
        i++;
     return i;
  }


  template<typename T>
  void move_append(std::vector<T> &dest, std::vector<T>&& src ) {
    if (src.empty()) {
      return;
    } else if (dest.empty()) {
      dest = std::move(src);
    } else {
      dest.insert(std::end(dest), std::make_move_iterator(std::begin(src)), std::make_move_iterator(std::end(src)));
    }
  }

  template<typename T>
  void move_append(small_vector_base<T> &dest, small_vector_base<T>&& src ) {
    if (src.empty()) {
      return;
    } else if (dest.empty()) {
      dest = std::move(src);
    } else {
      dest.insert(std::end(dest), std::make_move_iterator(std::begin(src)), std::make_move_iterator(std::end(src)));
    }
  }

  template<typename T>
  void copy_append(std::vector<T> &dest, const std::vector<T>& src ) {
    if (src.empty()) {
      return;
    } else {
      dest.insert(std::end(dest), std::begin(src), std::end(src));
    }
  }

  template<typename T>
  void deduplicate( std::vector<T>& entries ) {
    if (entries.size() > 1) {
      std::sort( entries.begin(), entries.end() );
      auto itr = std::unique( entries.begin(), entries.end() );
      entries.erase( itr, entries.end() );
    }
  }
}

  // outside of namespace fc becuase of VC++ conflict with std::swap
  template<typename T>
  void fc_swap( T& a, T& b ) {
    T tmp = fc::move(a);
    a = fc::move(b);
    b = fc::move(tmp);
  }

#define LLCONST(constant)   static_cast<int64_t>(constant##ll)
#define ULLCONST(constant)  static_cast<uint64_t>(constant##ull)
