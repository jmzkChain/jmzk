/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */

#include <jmzk/utilities/tempdir.hpp>

#include <cstdlib>

namespace jmzk { namespace utilities {

fc::path temp_directory_path()
{
   const char* eos_tempdir = getenv("jmzk_TEMPDIR");
   if( eos_tempdir != nullptr )
      return fc::path( eos_tempdir );
   return fc::temp_directory_path() / "eos-tmp";
}

} } // jmzk::utilities
