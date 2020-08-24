#######################################################################################################################
# enable_code_coverage_report.cmake
#
# Enable possibility to generate a code coverage report when compiling in 'Debug' mode.
# Configure with the option -DCMAKE_BUILD_TYPE=Debug.
# It require working tests which can be called with 'make test'. The coverage is created
# over these tests.
#
# You need lcov installed to use the 'make coverage' command.
#
# This script appends to the variable CMAKE_CXX_FLAGS_DEBUG,
# and adds the target 'coverage', only available in 'Debug' mode.
#
#######################################################################################################################

#######################################################################################################################
#
# IMPORTANT NOTE:
#
# DO NOT MODIFY THIS FILE inside a project. Instead update the project-template repository and pull the change from
# there. Make sure to keep the file generic, since it will be used by other projects, too.
#
# If you have modified this file inside a project despite this warning, make sure to cherry-pick all your changes
# into the project-template repository immediately.
#
#######################################################################################################################

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage")
# The make coverage command is only available in debug mode.  Also
# factor in that cmake treats CMAKE_BUILD_TYPE string as case
# insensitive.
string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type_uppercase)
IF(build_type_uppercase STREQUAL "DEBUG")
    configure_file(cmake/make_coverage.sh.in
    ${PROJECT_BINARY_DIR}/make_coverage.sh @ONLY)
    add_custom_target(coverage
    ./make_coverage.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating test coverage documentation" VERBATIM)
ENDIF()
