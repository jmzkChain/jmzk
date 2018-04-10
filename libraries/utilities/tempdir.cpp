/**
 *  @file
 *  @copyright defined in evt/LICENSE.txt
 */

#include <evt/utilities/tempdir.hpp>

#include <cstdlib>

namespace evt { namespace utilities {

fc::path temp_directory_path()
{
   const char* eos_tempdir = getenv("EVT_TEMPDIR");
   if( eos_tempdir != nullptr )
      return fc::path( eos_tempdir );
   return fc::temp_directory_path() / "eos-tmp";
}

} } // evt::utilities
