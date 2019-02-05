include(CMakeParseArguments)
#
# usage:
# register_tests(SOURCES
#                  ${list_of_source_files}
#                  "test_src.cc"
#                  ${concatenated_string_of_source_files}
#                NAMESPACE
#                  "test_namespace"
#                LINK_LIBRARIES
#                  ${list_of_targets}
#                INCLUDE_DIRECTORIES
#                  ${list_of_include_directories}
#                COMPILE_OPTIONS
#                  ${list_of_compile_options}
#                WORKING_DIRECTORY
#                  ${ctest_working_directory})
# 
# Function registers tests defined as a list of souce files.  Test
# defined by file source_name.cc in the SOURCES list is registered under
# the name
#   test_namespace.source_name
#
# If provided, test executables will use ${ctest_working_directory}
# as the working directory
#
# Provided list of dependent link targets and include directories are
# used as PRIVATE dependencies during compilation of test executable.
# 
# Function implicitly adds a dependency on the boost unit test framework
# to each generated test executable.
function(register_tests)
  find_package(Boost COMPONENTS unit_test_framework REQUIRED)

  list(APPEND single_parmeter_keywords NAMESPACE WORKING_DIRECTORY)
  list(APPEND multi_parmeter_keywords SOURCES 
                                      LINK_LIBRARIES
                                      INCLUDE_DIRECTORIES 
                                      COMPILE_OPTIONS)
  cmake_parse_arguments("arg" "" "${single_parmeter_keywords}"
                        "${multi_parmeter_keywords}" "${ARGN}")


  register_exe(SOURCES "${arg_SOURCES}" 
               NAMESPACE "${arg_NAMESPACE}"
               WORKING_DIRECTORY "${arg_WORKING_DIRECTORY}")

  get_test_targets(list_of_targets "${arg_SOURCES}")
  
  add_target_includes_private(TARGETS "${list_of_targets}" 
                              LINK_LIBRARIES "${arg_INCLUDE_DIRECTORIES}" 
                              "${Boost_INCLUDE_DIR}")

  add_target_link_libraries_private(TARGETS "${list_of_targets}" 
                                    LINK_LIBRARIES "${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}"
                                            "${arg_LINK_LIBRARIES}")

  add_target_compile_options_private(TARGETS "${list_of_targets}"
                                     COMPILE_OPTIONS "${arg_COMPILE_OPTIONS}")
  
endfunction()

#
# Private functions: Do not use directly
######################################################################
function(add_target_compile_options_private)
  list(APPEND list_of_multivalue_keywords TARGETS COMPILE_OPTIONS)
  cmake_parse_arguments("arg" "" "" "${list_of_multivalue_keywords}" "${ARGN}")
  foreach( target IN LISTS arg_TARGETS)
    target_compile_options(${target}
                           PRIVATE
                             ${arg_COMPILE_OPTIONS})
  endforeach()
endfunction()


function(add_target_includes_private)

  list(APPEND multi_value_keywords TARGETS LINK_LIBRARIES)
  cmake_parse_arguments("arg" "" "" "${multi_value_keywords}" "${ARGN}")
  foreach(target_name IN LISTS arg_TARGETS)
    target_include_directories("${target_name}"
                               PRIVATE
                               "${arg_LINK_LIBRARIES}")
  endforeach()
endfunction()

######################################
function(add_target_link_libraries_private)
  list(APPEND multi_value_keywords TARGETS LINK_LIBRARIES)
  cmake_parse_arguments("arg" "" "" "${multi_value_keywords}" "${ARGN}")
  foreach(target_name IN LISTS arg_TARGETS)
    target_link_libraries("${target_name}"
                               PRIVATE
                               "${arg_LINK_LIBRARIES}")
  endforeach()
endfunction()

######################################
function(register_exe)
  list(APPEND single_value_keywords NAMESPACE WORKING_DIRECTORY)
  list(APPEND multi_value_keywords SOURCES)
  
  cmake_parse_arguments("arg" "" "${single_value_keywords}" 
                         "${multi_value_keywords}" "${ARGN}")
  foreach(source_name IN LISTS arg_SOURCES)
    get_filename_component(target_name ${source_name} NAME_WE)
    add_executable(${target_name} ${source_name})
    # Fixme: The thing below doesnt work for some reason
    #add_test(NAME ${test_namespace}.${target_name} 
    #        COMMAND ${target_name} 
    #        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests)
    #        
    add_test(NAME
               ${arg_NAMESPACE}.${target_name}
             COMMAND
               ${target_name}
             WORKING_DIRECTORY
               ${arg_WORKING_DIRECTORY})
  endforeach()
endfunction()

######################################
function(get_test_targets list_of_targets list_of_source_files )
  foreach(source_name IN LISTS list_of_source_files)
    get_filename_component(target_name ${source_name} NAME_WE)
    list(APPEND list_of_targets_ ${target_name})
  endforeach()
  set(${list_of_targets} ${list_of_targets_} PARENT_SCOPE)
endfunction()
######################################################################
