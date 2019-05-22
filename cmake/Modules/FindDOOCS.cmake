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


FIND_PATH(DOOCS_DIR libDOOCSapi.so
  ${CMAKE_CURRENT_LIST_DIR}
  /export/doocs/lib
)
if (";${DOOCS_FIND_COMPONENTS};" MATCHES ";zmq;")
  FIND_PATH(DOOCS_DIR_ZMQ libDOOCSdzmq.so
    ${DOOCS_DIR}
  )
  set(DOOCS_LIBRARIES ${DOOCS_LIBRARIES} DOOCSdzmq)
endif()

if (";${DOOCS_FIND_COMPONENTS};" MATCHES ";dapi;")
  FIND_PATH(DOOCS_DIR_DAPI libDOOCSdapi.so
    ${DOOCS_DIR}
  )
  set(DOOCS_LIBRARIES ${DOOCS_LIBRARIES} DOOCSdapi)
endif()

if (";${DOOCS_FIND_COMPONENTS};" MATCHES ";server;")
  FIND_PATH(DOOCS_DIR_SERVER libEqServer.so
    ${DOOCS_DIR}
  )
  set(DOOCS_LIBRARIES ${DOOCS_LIBRARIES} EqServer)
endif()

if (";${DOOCS_FIND_COMPONENTS};" MATCHES ";ddaq;")
  FIND_PATH(DOOCS_DIR_SERVER libDOOCSddaq.so
    ${DOOCS_DIR}
  )
  set(DOOCS_LIBRARIES ${DOOCS_LIBRARIES} DOOCSddaq timinginfo daqevstat DAQFSM TTF2XML xerces-c BM TTF2evutl)
endif()

if (";${DOOCS_FIND_COMPONENTS};" MATCHES ";daqreader;")
  FIND_PATH(DOOCS_DIR_SERVER libDOOCSdaqreader.so
    ${DOOCS_DIR}
  )
  set(DOOCS_LIBRARIES ${DOOCS_LIBRARIES} DAQReader TTF2evutl TTF2XML lzo2 DAQsvrutil)
endif()

set(DOOCS_LIBRARIES ${DOOCS_LIBRARIES} DOOCSapi nsl dl pthread m rt ldap gul)

# now set the required variables based on the determined DOOCS_DIR
set(DOOCS_INCLUDE_DIRS ${DOOCS_DIR}/include)
set(DOOCS_LIBRARY_DIRS ${DOOCS_DIR}/)

set(DOOCS_CXX_FLAGS "-Wall -fPIC -D_REENTRANT -DLINUX -D__LINUX__ -DDMSG -DTINE_EXPORT=' '")
set(DOOCS_LINKER_FLAGS "-Wl,--no-as-needed")
set(DOOCS_LINK_FLAGS "${DOOCS_LINKER_FLAGS}")

# extract DOOCS version from librar so symlink. Note: This is platform dependent and only works
# if DOOCS was installed from the Debian pagackes. Find a better version detection scheme!
execute_process(COMMAND bash -c "env LANG=C readelf -d ${DOOCS_DIR}/libDOOCSapi.so | grep SONAME | sed -e 's/^.*Library soname: \\[libDOOCSapi\\.so\\.//' -e 's/\\]$//'"
                OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE DOOCS_VERSION)

# use a macro provided by CMake to check if all the listed arguments are valid and set DOOCS_FOUND accordingly
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DOOCS REQUIRED_VARS DOOCS_DIR VERSION_VAR DOOCS_VERSION )
if (";${DOOCS_FIND_COMPONENTS};" MATCHES ";server;")
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(DOOCS REQUIRED_VARS DOOCS_DIR_SERVER VERSION_VAR DOOCS_VERSION )
endif()
if (";${DOOCS_FIND_COMPONENTS};" MATCHES ";zmq;")
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(DOOCS REQUIRED_VARS DOOCS_DIR_ZMQ VERSION_VAR DOOCS_VERSION )
endif()

