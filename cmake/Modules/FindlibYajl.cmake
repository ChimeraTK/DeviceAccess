#######################################################################################################################
#
# cmake module for finding the yajl library
#
# returns:
#   libYajl_FOUND       : true or false, depending on whether the package was found
#   libYajl_LIBRARY     : path to the library
# @author Patrick Nonn, DESY
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

set(libYajl_FOUND 0)

find_library(libYajl_LIBRARY
    NAMES yajl yajl.so libyajl.so
    PATHS /usr/lib /usr/lib32 /usr/lib64 /usr/local/lib
    HINTS ${CMAKE_INSTALL_LIBDIR}
)

# use a macro provided by CMake to check if all the listed arguments are valid and set Yajl_FOUND accordingly
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libYajl REQUIRED_VARS libYajl_LIBRARY)
