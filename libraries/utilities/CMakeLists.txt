#list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/libraries/fc/GitVersionGen" )
#include( GetGitRevisionDescription )
#get_git_head_revision(GIT_REFSPEC jmzk_GIT_REVISION_SHA)
#get_git_unix_timestamp(jmzk_GIT_REVISION_UNIX_TIMESTAMP)
#git_describe(jmzk_GIT_REVISION_DESCRIPTION --tags)
if(NOT jmzk_GIT_REVISION_DESCRIPTION)
    set(jmzk_GIT_REVISION_DESCRIPTION "unknown")
endif(NOT jmzk_GIT_REVISION_DESCRIPTION)

set(sources
    key_conversion.cpp
    string_escape.cpp
    tempdir.cpp
    words.cpp
)

#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp" @ONLY)
#list(APPEND sources "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp")

add_library(jmzk_utilities
    ${sources}
)
target_link_libraries(jmzk_utilities fc)
target_include_directories(jmzk_utilities PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
if (USE_PCH)
    set_target_properties(jmzk_utilities PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
    cotire(jmzk_utilities)
endif(USE_PCH)
