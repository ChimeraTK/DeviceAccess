#######################################################################################################################
#
# cmake module for finding the config generator
#
# returns:
#   ConfigGenerator_FOUND   : true or false, depending on whether the package was found
#   ConfigGenerator_VERSION : the package version
#   ConfigGenerator_DIR     : path to the include directory
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

SET(ConfigGenerator_FOUND 0)


file(GLOB ConfigGenerator_SEARCH_PATHS ${CMAKE_CURRENT_LIST_DIR} "${CMAKE_INSTALL_PREFIX}/share/ConfigGenerator*" "/usr/share/ConfigGenerator*")
FIND_PATH(ConfigGenerator_DIR
    NAMES ConfigGeneratorConfigVersion.cmake
    PATHS ${ConfigGenerator_SEARCH_PATHS}
)

include(${ConfigGenerator_DIR}/ConfigGeneratorConfigVersion.cmake)
set(ConfigGenerator_VERSION ${PACKAGE_VERSION})

# use a macro provided by CMake to check if all the listed arguments are valid and set ConfigGenerator_FOUND accordingly
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ConfigGenerator REQUIRED_VARS ConfigGenerator_DIR VERSION_VAR ConfigGenerator_VERSION )

