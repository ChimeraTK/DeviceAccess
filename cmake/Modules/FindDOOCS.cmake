#######################################################################################################################
#
# cmake module for finding DOOCS
#
# By default, only the client API is included. If the component "server" is specified, also the
# server library will be used.
#
# returns:
#   DOOCS_FOUND        : true or false, depending on whether the package was found
#   DOOCS_VERSION      : the package version
#   DOOCS_INCLUDE_DIRS : path to the include directory
#   DOOCS_LIBRARY_DIRS : path to the library directory
#   DOOCS_LIBRARIES    : list of libraries to link against
#   DOOCS_CXX_FLAGS    : Flags needed to be passed to the c++ compiler
#   DOOCS_LINK_FLAGS   : Flags needed to be passed to the linker
#
# @author Martin Hierholzer, DESY
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

SET(DOOCS_FOUND 0)

if("${DOOCS_FIND_COMPONENTS}" STREQUAL "server")
  FIND_PATH(DOOCS_DIR libEqServer.so
    ${CMAKE_CURRENT_LIST_DIR}
    /export/doocs/lib
  )
  set(DOOCS_LIBRARIES EqServer DOOCSapi nsl dl pthread m rt ldap)
else()
  FIND_PATH(DOOCS_DIR libDOOCSapi.so
    ${CMAKE_CURRENT_LIST_DIR}
    /export/doocs/lib
  )
  set(DOOCS_LIBRARIES DOOCSapi nsl dl pthread m rt ldap)
endif()

# now set the required variables based on the determined DOOCS_DIR
set(DOOCS_INCLUDE_DIRS ${DOOCS_DIR}/include)
set(DOOCS_LIBRARY_DIRS ${DOOCS_DIR}/)

set(DOOCS_CXX_FLAGS "-Wall -fPIC -D_REENTRANT -DLINUX -D__LINUX__ -DDMSG")
set(DOOCS_LINK_FLAGS "-Wl,--no-as-needed")

# extract DOOCS version from librar so symlink. Note: This is platform dependent and only works
# if DOOCS was installed from the Debian pagackes. Find a better version detection scheme!
execute_process(COMMAND bash -c "readlink ${DOOCS_DIR}/libDOOCSapi.so | sed -e 's/^.*libDOOCSapi.so.//' -e 's/-.*$//'" OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE DOOCS_VERSION)

# use a macro provided by CMake to check if all the listed arguments are valid and set DOOCS_FOUND accordingly
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DOOCS
        REQUIRED_VARS DOOCS_DIR
	    VERSION_VAR DOOCS_VERSION )

