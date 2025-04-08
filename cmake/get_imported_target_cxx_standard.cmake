# ######################################################################################################################
# get_imported_target_cxx_standard.cmake
#
# Define function to obtain the compiler flag for the C++ standard required by an imported target (defined via
# target_compile_features()), querying recursively the dependencies as well. If multiple standards are required by
# different dependencies, the greatest standard will be picked.
#
# Usage:
#
# get_imported_target_cxx_standard(MyImportedTarget NameOfVariableToPutFlagIn)
#
# ######################################################################################################################

# ######################################################################################################################
#
# IMPORTANT NOTE:
#
# DO NOT MODIFY THIS FILE inside a project. Instead update the project-template repository and pull the change from
# there. Make sure to keep the file generic, since it will be used by other projects, too.
#
# If you have modified this file inside a project despite this warning, make sure to cherry-pick all your changes
# into the project-template repository immediately.
#
# ######################################################################################################################

# Internal helper function
function(collect_compile_features_recursive target compile_features)
  get_target_property(features ${target} INTERFACE_COMPILE_FEATURES)

  if(features)
    foreach(f ${features})
      list(APPEND ${compile_features} ${f})
    endforeach()
  endif()

  get_target_property(deps ${target} INTERFACE_LINK_LIBRARIES)

  foreach(dep ${deps})
    if(TARGET ${dep})
      collect_compile_features_recursive(${dep} ${compile_features})
    endif()
  endforeach()

  set(${compile_features} ${${compile_features}} PARENT_SCOPE)
endfunction()

# ######################################################################################################################

# Main function to be called by the user
function(get_imported_target_cxx_standard target required_standard_flag)
  collect_compile_features_recursive(${target} my_compile_features)

  set(required_standard 0)

  foreach(feature ${my_compile_features})
    if(feature MATCHES "cxx_std_([0-9]+)")
      if(CMAKE_MATCH_1 GREATER required_standard)
        set(required_standard ${CMAKE_MATCH_1})
      endif()
    endif()
  endforeach()

  if(required_standard GREATER 0)
    set(flag "-std=c++${required_standard}")
  else()
    set(flag "")
  endif()

  set(${required_standard_flag} ${flag} PARENT_SCOPE)
endfunction()

# ######################################################################################################################
