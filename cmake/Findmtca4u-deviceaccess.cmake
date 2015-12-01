#
# cmake module for finding mtca4u-deviceaccess
#
# returns:
#   mtca4u-deviceaccess_FOUND        : true or false, depending on whether
#                                   the package was found
#   mtca4u-deviceaccess_VERSION      : the package version
#   mtca4u-deviceaccess_INCLUDE_DIRS : path to the include directory
#   mtca4u-deviceaccess_LIBRARY_DIRS : path to the library directory
#   mtca4u-deviceaccess_LIBRARY      : the provided libraries
#
#   mtca4u-deviceaccess_CPPFLAGS     : compiler flags for regular Makefiles
#   mtca4u-deviceaccess_LDFLAGS      : linker flags for regular Makefiles
#
# @author Martin Killenberg, DESY
#

SET(mtca4u-deviceaccess_FOUND 0)

#FIXME: the search path for the device config has to be extended/generalised/improved
FIND_PATH(mtca4u-deviceaccess_DIR
    mtca4u-deviceaccessConfig.cmake
    ${CMAKE_CURRENT_LIST_DIR}
    )

#Once we have found the config our job is done. Just load the config which provides the required 
#varaibles.
include(${mtca4u-deviceaccess_DIR}/mtca4u-deviceaccessConfig.cmake)

#use a macro provided by CMake to check if all the listed arguments are valid
#and set mtca4u-deviceaccess_FOUND accordingly
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(mtca4u-deviceaccess 
        REQUIRED_VARS mtca4u-deviceaccess_LIBRARIES mtca4u-deviceaccess_INCLUDE_DIRS
	VERSION_VAR mtca4u-deviceaccess_VERSION )

