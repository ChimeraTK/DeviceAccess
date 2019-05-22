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

#######################################################################################################################
# Set the build type to Release if none is specified
# Force it into Release if "None" is specified (needed to overrule dkpg_buildpackage)
######################################################################################################################

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "None")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "None")

