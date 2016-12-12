#######################################################################################################################
# set_version_numbers.cmake
#
# Set the version numbers for a package in the format needed by other scripts of the project-template
#
# Expects the following input variables:
#   ${PROJECT_NAME}_MAJOR_VERSION - major version number
#   ${PROJECT_NAME}_MINOR_VERSION - minor version number
#   ${PROJECT_NAME}_PATCH_VERSION - patch number
#
# Optional environment variables:
#   $PROJECT_BUILDVERSION - additional build version number for the shared object version number (e.g. "xenial1")
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

set(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})

set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION})
set(${PROJECT_NAME}_BUILDVERSION $ENV{PROJECT_BUILDVERSION})
if( ${PROJECT_NAME}_BUILDVERSION )
  set(${PROJECT_NAME}_SOVERSION "${${PROJECT_NAME}_SOVERSION}${${PROJECT_NAME}_BUILDVERSION}")
endif( ${PROJECT_NAME}_BUILDVERSION )

set(${PROJECT_NAME}_FULL_LIBRARY_VERSION ${${PROJECT_NAME}_SOVERSION}.${${PROJECT_NAME}_PATCH_VERSION})

