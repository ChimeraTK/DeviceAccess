#######################################################################################################################
# create_cmake_config_files.cmake
#
# Create the Find${PROJECT_NAME}.cmake cmake macro and the ${PROJECT_NAME}-config shell script and installs them.
#
# Expects the following input variables:
#   ${PROJECT_NAME}_SOVERSION - version of the .so library file (or just MAJOR.MINOR without the patch level)
#   ${PROJECT_NAME}_INCLUDE_DIRS - list include directories needed when compiling against this project
#   ${PROJECT_NAME}_LIBRARY_DIRS - list of library directories needed when linking against this project
#   ${PROJECT_NAME}_LIBRARIES - list of additional libraries needed when linking against this project. The library
#                               provided by the project will be added automatically
#   ${PROJECT_NAME}_CXX_FLAGS - list of additional C++ compiler flags needed when compiling against this project
#   ${PROJECT_NAME}_LINKER_FLAGS - list of additional linker flags needed when linking against this project
#   ${PROJECT_NAME}_MEXFLAGS - (optional) mex compiler flags
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

# create variables for standard makefiles
set(${PROJECT_NAME}_CXX_FLAGS_MAKEFILE "${${PROJECT_NAME}_CXX_FLAGS}")

string(REPLACE " " ";" LIST ${${PROJECT_NAME}_INCLUDE_DIRS})
foreach(INCLUDE_DIR ${LIST})
  set(${PROJECT_NAME}_CXX_FLAGS_MAKEFILE "${${PROJECT_NAME}_CXX_FLAGS_MAKEFILE} -I${INCLUDE_DIR}")
endforeach()

set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS}")

string(REPLACE " " ";" LIST ${${PROJECT_NAME}_LIBRARY_DIRS})
foreach(LIBRARY_DIR ${LIST})
  set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE} -L${LIBRARY_DIR}")
endforeach()

string(REPLACE " " ";" LIST ${${PROJECT_NAME}_LIBRARIES})
foreach(LIBRARY ${LIST})
  if(LIBRARY MATCHES "/")  # library name contains slashes: link against the a file path name
    set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE} ${LIBRARY}")
  else()                   # library name does not contain slashes: link against library with -l option
    set(${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE "${${PROJECT_NAME}_LINKER_FLAGS_MAKEFILE} -l${LIBRARY}")
  endif()
endforeach()

# we have nested @-statements, so we have to parse twice:

# create the cmake Find package script
configure_file(cmake/FindPROJECT_NAME.cmake.in.in "${PROJECT_BINARY_DIR}/cmake/Find${PROJECT_NAME}.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/Find${PROJECT_NAME}.cmake.in "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake" @ONLY)

# create the shell script for standard make files
configure_file(cmake/PROJECT_NAME-config.in.in "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config" @ONLY)

# install the script
install(FILES "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake"
  DESTINATION share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules COMPONENT dev)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config DESTINATION bin COMPONENT dev)

