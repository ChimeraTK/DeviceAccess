#######################################################################################################################
#
# cmake module for finding DOOCS
#
# By default, only the client API is included. If the component "server" is specified, also the
# server library will be used. If the component "zmq" is specified, the DOOCSdzmq library will be used as well.
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

list(APPEND DOOCS_FIND_COMPONENTS doocsapi)

if (";${DOOCS_FIND_COMPONENTS};" MATCHES ";zmq;")
  list(APPEND DOOCS_FIND_COMPONENTS doocsdzmq)
  list(REMOVE_ITEM DOOCS_FIND_COMPONENTS zmq)
endif()

if (";${DOOCS_FIND_COMPONENTS};" MATCHES ";dapi;")
  list(APPEND DOOCS_FIND_COMPONENTS doocsdapi)
  list(REMOVE_ITEM DOOCS_FIND_COMPONENTS dapi)
endif()

if (";${DOOCS_FIND_COMPONENTS};" MATCHES ";server;")
  list(APPEND DOOCS_FIND_COMPONENTS serverlib)
  list(REMOVE_ITEM DOOCS_FIND_COMPONENTS server)
endif()

set(DOOCS_FIND_COMPONENTS_DDAQ false)
if (";${DOOCS_FIND_COMPONENTS};" MATCHES ";ddaq;")
  # This library seems not yet to come with a pkg-config module
  list(REMOVE_ITEM DOOCS_FIND_COMPONENTS ddaq)
  set(DOOCS_FIND_COMPONENTS_DDAQ true)
endif()

if (";${DOOCS_FIND_COMPONENTS};" MATCHES ";daqreader;")
  list(APPEND DOOCS_FIND_COMPONENTS daqreaderlib)
  list(REMOVE_ITEM DOOCS_FIND_COMPONENTS daqreader)
endif()

# For newer cmake versions, the following foreach() can be replaced by this:
# list(TRANSFORM DOOCS_FIND_COMPONENTS PREPEND "doocs-")
foreach(component ${DOOCS_FIND_COMPONENTS})
  list(APPEND DOOCS_FIND_COMPONENTS_TRANSFORMED "doocs-${component}")
endforeach()
set(DOOCS_FIND_COMPONENTS ${DOOCS_FIND_COMPONENTS_TRANSFORMED})

include(FindPkgConfig)
pkg_check_modules(DOOCS REQUIRED ${DOOCS_FIND_COMPONENTS})

string(REPLACE ";" " " DOOCS_CFLAGS "${DOOCS_CFLAGS}")
string(REPLACE ";" " " DOOCS_LDFLAGS "${DOOCS_LDFLAGS}")

set(DOOCS_DIR "${DOOCS_doocs-doocsapi_LIBDIR}")
set(DOOCS_VERSION "${DOOCS_doocs-doocsapi_VERSION}")
set(DOOCS_CXX_FLAGS ${DOOCS_CFLAGS})
set(DOOCS_LIBRARIES ${DOOCS_LDFLAGS})
set(DOOCS_LINKER_FLAGS "-Wl,--no-as-needed")
set(DOOCS_LINK_FLAGS ${DOOCS_LINKER_FLAGS})

set(COMPONENT_DIRS "")
if(DOOCS_FIND_COMPONENTS_DDAQ)
  message("Searching for libDOOCSddaq.so")
  FIND_PATH(DOOCS_DIR_ddaq libDOOCSddaq.so
    ${DOOCS_DIR}
  )
  set(DOOCS_LIBRARIES ${DOOCS_LIBRARIES} DOOCSddaq timinginfo daqevstat DAQFSM TTF2XML xerces-c BM TTF2evutl)
  set(COMPONENT_DIRS ${COMPONENT_DIRS} DOOCS_DIR_ddaq)
endif()

# use a macro provided by CMake to check if all the listed arguments are valid and set DOOCS_FOUND accordingly
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DOOCS REQUIRED_VARS DOOCS_DIR ${COMPONENT_DIRS} VERSION_VAR DOOCS_VERSION )

