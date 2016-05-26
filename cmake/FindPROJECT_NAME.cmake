#
# cmake module for finding mtca4uInstaCoSADev
#
# returns:
#   mtca4uInstaCoSADev_FOUND        : true or false, depending on whether
#                                   the package was found
#   mtca4uInstaCoSADev_VERSION      : the package version
#   mtca4uInstaCoSADev_INCLUDE_DIRS : path to the include directory
#   mtca4uInstaCoSADev_LIBRARY_DIRS : path to the library directory
#   mtca4uInstaCoSADev_LIBRARY      : the provided libraries
#
# @author Martin Hierholzer, DESY
#

SET(mtca4uInstaCoSADev_FOUND 0)

#FIXME: the search path for the device config has to be extended/generalised/improved
FIND_PATH(mtca4uInstaCoSADev_DIR
    mtca4uInstaCoSADevConfig.cmake
    ${CMAKE_CURRENT_LIST_DIR}
    )

#Once we have found the config our job is done. Just load the config which provides the required 
#varaibles.
include(${mtca4uInstaCoSADev_DIR}/mtca4uInstaCoSADevConfig.cmake)

#use a macro provided by CMake to check if all the listed arguments are valid
#and set mtca4uInstaCoSADev_FOUND accordingly
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(mtca4uInstaCoSADev 
        REQUIRED_VARS mtca4uInstaCoSADev_LIBRARIES mtca4uInstaCoSADev_INCLUDE_DIRS
	VERSION_VAR mtca4uInstaCoSADev_VERSION )

