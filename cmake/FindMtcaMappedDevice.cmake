#
# cmake module for finding MtcaMappedDevice
#
# returns:
#   MtcaMappedDevice_FOUND        : true or false, depending on whether
#                                   the package was found
#   MtcaMappedDevice_VERSION      : the package version
#   MtcaMappedDevice_INCLUDE_DIRS : path to the include directory
#   MtcaMappedDevice_LIBRARY_DIRS : path to the library directory
#   MtcaMappedDevice_LIBRARY      : the provided libraries
#
# @author Martin Killenberg, DESY
#

SET(MtcaMappedDevice_FOUND 0)

#FIXME: the search path for the device config has to be extended/generalised/improved
FIND_PATH(MtcaMappedDevice_DIR
    MtcaMappedDeviceConfig.cmake
    ${CMAKE_CURRENT_LIST_DIR}
    )

#Once we have found the config our job is done. Just load the config which provides the required 
#varaibles.
message(" MtcaMappedDevice_DIR  ${MtcaMappedDevice_DIR} ")
include(${MtcaMappedDevice_DIR}/MtcaMappedDeviceConfig.cmake)

message("finder MtcaMappedDevice_INCLUDE_DIRS ${MtcaMappedDevice_INCLUDE_DIRS}")
message("finder MtcaMappedDevice_VERSION ${MtcaMappedDevice_VERSION}")
#message("finder MtcaMappedDevice ${MtcaMappedDevice_VERSION}")

#use a macro provided by CMake to check if all the listed arguments are valid
#and set MtcaMappedDevice_FOUND accordingly
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MtcaMappedDevice 
        REQUIRED_VARS MtcaMappedDevice_LIBRARIES MtcaMappedDevice_INCLUDE_DIRS
	VERSION_VAR MtcaMappedDevice_VERSION )

